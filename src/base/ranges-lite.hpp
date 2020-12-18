/****************************************************************
**ranges-lite.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Light-weight ranges library to replace range-v3.
*
*****************************************************************/
#pragma once

// base
#include "maybe.hpp"
#include "stack-trace.hpp"

// C++ standard library
#include <cassert>
#include <iterator>
#include <span>

// rl stands for "ranges lite".
namespace base::rl {

// FIXME: move this out of here.
#ifndef NDEBUG
#  define rl_assert( ... ) \
    if( !( __VA_ARGS__ ) ) abort_with_backtrace_here();
#else
// Use the argument in case there are any variables in there that
// would otherwise be unused in a release build and trigger warn-
// ings.
#  define rl_assert( ... ) \
    (void)( ( decltype( __VA_ARGS__ )* ){} );
#endif

/****************************************************************
** TODO list
*****************************************************************/
//
//      rv::sliding
//
//      rv::drop_last
//
//      rv::intersperse
//
//      rv::join

/****************************************************************
** Forward Declarations
*****************************************************************/
class IntsView;

template<typename InputView, typename Cursor>
class ChainView;

/****************************************************************
** Metaprogramming helpers.
*****************************************************************/
template<typename T, typename = void>
struct ultimate_view_or_self {
  using type = T;
};

template<typename T>
struct ultimate_view_or_self<
    T, std::void_t<typename T::ultimate_view_t>> {
  using type = typename T::ultimate_view_t;
};

template<typename T>
using ultimate_view_or_self_t =
    typename ultimate_view_or_self<T>::type;

template<typename T>
struct is_rl_view : std::false_type {};

template<typename InputView, typename Cursor>
struct is_rl_view<ChainView<InputView, Cursor>>
  : std::true_type {};

template<typename T>
constexpr bool is_rl_view_v = is_rl_view<T>::value;

/****************************************************************
** Macros
*****************************************************************/
#define RL_LAMBDA( name, ... ) \
  name( [&]( auto&& _ ) { return __VA_ARGS__; } )

#define keep_rl( ... ) RL_LAMBDA( keep, __VA_ARGS__ )
#define remove_rl( ... ) RL_LAMBDA( remove, __VA_ARGS__ )
#define map_rl( ... ) RL_LAMBDA( map, __VA_ARGS__ )
#define take_while_rl( ... ) RL_LAMBDA( take_while, __VA_ARGS__ )
#define drop_while_rl( ... ) RL_LAMBDA( drop_while, __VA_ARGS__ )
#define take_while_incl_rl( ... ) \
  RL_LAMBDA( take_while_incl, __VA_ARGS__ )

/****************************************************************
** Identity Cursor
*****************************************************************/
template<typename InputView>
struct IdentityCursor {
  struct Data {};
  using iterator   = typename InputView::iterator;
  using value_type = typename iterator::value_type;
  IdentityCursor() = default;
  IdentityCursor( Data const& ) {}
  void init( InputView const& input ) { it = input.begin(); }
  value_type& get( InputView const& ) const { return *it; }
  void        next( InputView const& ) { ++it; }
  bool        end( InputView const& input ) const {
    return it == input.end();
  }
  iterator pos( InputView const& ) const { return it; }
  iterator it;
};

template<typename InputView>
struct ReverseIdentityCursor {
  struct Data {};
  using iterator          = typename InputView::reverse_iterator;
  using value_type        = typename iterator::value_type;
  ReverseIdentityCursor() = default;
  ReverseIdentityCursor( Data const& ) {}
  void init( InputView const& input ) { it = input.rbegin(); }
  value_type& get( InputView const& ) const { return *it; }
  void        next( InputView const& ) { ++it; }
  bool        end( InputView const& input ) const {
    return it == input.rend();
  }
  iterator pos( InputView const& ) const { return it; }
  iterator it;
};

/****************************************************************
** IntsView
*****************************************************************/
class IntsView {
  const int start_;
  const int end_;

public:
  IntsView( int start = 0,
            int end   = std::numeric_limits<int>::max() )
    : start_( start ), end_( end ) {
    rl_assert( start <= end );
  }

  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = int const;
    using pointer           = value_type const*;
    using reference         = value_type const&;

    iterator() = default;
    iterator( IntsView const* view )
      : view_( view ), cursor_( view_->start_ ) {
      clear_if_end();
    }

    void clear_if_end() {
      if( cursor_ >= view_->end_ ) view_ = nullptr;
    }

    auto const& operator*() const {
      rl_assert( cursor_ < view_->end_ );
      return cursor_;
    }

    auto const* operator->() const {
      return std::addressof( this->operator*() );
    }

    iterator& operator++() {
      rl_assert( cursor_ < view_->end_ );
      cursor_++;
      clear_if_end();
      return *this;
    }

    iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( iterator const& rhs ) const {
      if( view_ != nullptr && rhs.view_ != nullptr )
        return cursor_ == rhs.cursor_;
      return view_ == rhs.view_;
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    IntsView const* view_ = nullptr;
    int             cursor_;
  };

  using const_iterator = iterator;
  iterator begin() const { return iterator( this ); }
  iterator end() const { return iterator(); }
};

/****************************************************************
** ChainView
*****************************************************************/
template<typename InputView, typename Cursor>
class ChainView {
  using data_t = typename Cursor::Data;

  InputView input_;
  data_t    data_;

public:
  ChainView( InputView&& input, data_t&& data )
    : input_( std::move( input ) ), data_( std::move( data ) ) {}

  using value_type =
      std::decay_t<std::invoke_result_t<decltype( &Cursor::get ),
                                        Cursor*, InputView>>;

  using ultimate_view_t = ultimate_view_or_self_t<InputView>;

  static auto create( ultimate_view_t const& input ) {
    if constexpr( std::is_default_constructible_v<data_t> ) {
      if constexpr( std::is_same_v<ultimate_view_t,
                                   InputView> ) {
        return ChainView{ InputView( input ), data_t{} };
      } else {
        return ChainView{ InputView::create( input ), data_t{} };
      }
    } else {
      // If you get an error here it is because one of the views
      // in the pipeline has a Data member struct that is not de-
      // fault constructible. Cursor Data structs should only be
      // default constructible if it would make sense to use the
      // view/cursor with those values default constructed. For
      // example, the view `take( 5 )` needs to hold the number 5
      // in its Data, and thus its Data should not be default
      // constructible, otherwise that number would just default
      // to zero, and thus would not reconstruct whatever view
      // was used to get the type that is trying to be con-
      // structed here. On the other hand, a Data that only takes
      // a stateless lambda can be default constructed and so
      // that shouldn't be a problem.
      struct YouCannotUseStaticCreateHere {};
      return YouCannotUseStaticCreateHere{};
    }
  }

  /**************************************************************
  ** iterator
  ***************************************************************/
  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = ChainView::value_type;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator() = default;
    iterator( ChainView const* view )
      : view_( view ), cursor_( view->data_ ) {
      cursor_.init( view_->input_ );
      clear_if_end();
    }

    void clear_if_end() {
      if( cursor_.end( view_->input_ ) ) view_ = nullptr;
    }

    decltype( auto ) operator*() const {
      rl_assert( view_ != nullptr );
      return cursor_.get( view_->input_ );
    }

    auto* operator->() const {
      return std::addressof( this->operator*() );
    }

    iterator& operator++() {
      rl_assert( view_ != nullptr );
      cursor_.next( view_->input_ );
      clear_if_end();
      return *this;
    }

    iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( iterator const& rhs ) const {
      if( view_ != nullptr && rhs.view_ != nullptr )
        return cursor_.pos( view_->input_ ) ==
               rhs.cursor_.pos( rhs.view_->input_ );
      return view_ == rhs.view_;
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    ChainView const* view_ = nullptr;
    Cursor           cursor_;
  };

  using const_iterator = iterator;
  iterator begin() const { return iterator( this ); }
  iterator end() const { return iterator(); }

  /**************************************************************
  ** Materialization
  ***************************************************************/
  std::vector<value_type> to_vector() const {
    return std::vector<value_type>( this->begin(), this->end() );
  }

  template<typename T>
  T to() {
    return T( this->begin(), this->end() );
  }

  /**************************************************************
  ** Accumulate
  ***************************************************************/
  template<typename Op = std::plus<>>
  value_type accumulate( Op&& op = {}, value_type init = {} ) {
    value_type res = init;
    for( auto const& e : *this )
      res = std::invoke( std::forward<Op>( op ), res, e );
    return res;
  }

  /**************************************************************
  ** Head
  ***************************************************************/
  auto head() {
    maybe<std::decay_t<value_type>> res;

    auto it = begin();
    if( it != end() ) res = *it;
    return res;
  }

  /**************************************************************
  ** CursorBase
  ***************************************************************/
private:
  // Used to remove some redundancy from Cursors.
  template<typename Derived>
  struct CursorBase {
    using iterator = typename ChainView::iterator;

    CursorBase() = default;

    Derived const* derived() const {
      return static_cast<Derived const*>( this );
    }

    void init( ChainView const& input ) { it_ = input.begin(); }

    decltype( auto ) get( ChainView const& input ) const {
      rl_assert( !derived()->end( input ) );
      return *it_;
    }

    void next( ChainView const& input ) {
      rl_assert( !derived()->end( input ) );
      ++it_;
    }

    bool end( ChainView const& input ) const {
      return it_ == input.end();
    }

    // The type that this returns doesn't really matter, it just
    // has to be a unique value for each iteration of the cursor.
    iterator pos( ChainView const& ) const { return it_; }

    iterator it_;
  };

  /**************************************************************
  ** Chain Maker
  ***************************************************************/
private:
  template<typename NewCursor, typename... Args>
  auto make_chain( Args&&... args ) {
    using NewChainView =
        ChainView<ChainView<InputView, Cursor>, NewCursor>;
    using Data = typename NewCursor::Data;
    return NewChainView( std::move( *this ),
                         Data( std::forward<Args>( args )... ) );
  }

public:
  /**************************************************************
  ** Keep
  ***************************************************************/
  template<typename Func>
  auto keep( Func&& func ) && {
    struct KeepCursor : public CursorBase<KeepCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<KeepCursor>;
      using Base::it_;
      KeepCursor() = default;
      KeepCursor( Data const& data ) : func_( &data.func_ ) {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        do {
          if( end( input ) ) break;
          if( ( *func_ )( *it_ ) ) break;
          ++it_;
        } while( true );
      }

      void next( ChainView const& input ) {
        rl_assert( !end( input ) );
        do {
          ++it_;
          if( end( input ) ) break;
          if( ( *func_ )( *it_ ) ) break;
        } while( true );
      }

      using Base::end;
      using Base::get;
      using Base::pos;

      func_t const* func_;
    };
    return make_chain<KeepCursor>( std::forward<Func>( func ) );
  }

  /**************************************************************
  ** Remove
  ***************************************************************/
  template<typename Func>
  auto remove( Func&& func ) && {
    return std::move( *this ).keep(
        [func = std::forward<Func>( func )]( auto&& arg ) {
          return !func( arg );
        } );
  }

  /**************************************************************
  ** Enumerate
  ***************************************************************/
  // FIXME: make this more efficient with a specialized Cursor.
  auto enumerate( int start = 0 ) && {
    using IntsCursor = IdentityCursor<IntsView>;
    using Data       = typename IntsCursor::Data;
    return std::move( *this )
        .zip( ChainView<IntsView, IntsCursor>( IntsView( start ),
                                               Data{} ) )
        .map( []( auto&& p ) {
          return std::pair{ p.second, p.first };
        } );
  }

  /**************************************************************
  ** Lense
  ***************************************************************/
  template<typename Func>
  auto lense( Func&& func ) && {
    struct KeysCursor : public CursorBase<KeysCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<KeysCursor>;
      using Base::it_;
      using typename Base::iterator;
      KeysCursor() = default;
      KeysCursor( Data const& data ) : func_( &data.func_ ) {}

      auto& get( ChainView const& input ) const {
        rl_assert( !end( input ) );
        // These checks are an effort to ensure that we don't
        // call a lense on a temporary, and that the lense re-
        // turns a reference. The idea is that lenses are used to
        // produce references into the original data being
        // viewed, and not to intermediate values produced
        // through the pipeline.
        static_assert(
            std::is_reference_v<std::invoke_result_t<
                Func, std::add_lvalue_reference_t<
                          ChainView::value_type>>>,
            "In order to use a lense, the callable must return "
            "a reference (into existing data). If you are using "
            "a lambda that is supposed to return a reference, "
            "then you might need to use -> decltype( auto )." );
        // If you fail here then you are probably trying to lense
        // something that is producing a temporary.
        static_assert(
            std::is_lvalue_reference_v<decltype( *it_ )> );
        return ( *func_ )( *it_ );
      }

      using Base::end;
      using Base::init;
      using Base::next;
      using Base::pos;

      func_t const* func_;
    };
    return make_chain<KeysCursor>( std::forward<Func>( func ) );
  }

  /**************************************************************
  ** Dereference
  ***************************************************************/
  auto dereference() && {
    return std::move( *this ).lense(
        []( auto&& arg ) -> decltype( auto ) { return *arg; } );
  }

  /**************************************************************
  ** CatMaybes
  ***************************************************************/
  auto cat_maybes() && {
    return std::move( *this )
        .remove( []( auto&& arg ) { return arg == nothing; } )
        .dereference();
  }

  /**************************************************************
  ** Keys
  ***************************************************************/
  auto keys() && {
    return std::move( *this ).lense(
        // Need auto&& return type so that it will deduce a ref-
        // erence for p.first (decltype(auto) will not).
        []( auto&& p ) -> auto&& { return p.first; } );
  }

  /**************************************************************
  ** Tail
  ***************************************************************/
  auto tail() && { return std::move( *this ).drop( 1 ); }

  /**************************************************************
  ** Map
  ***************************************************************/
  template<typename Func>
  auto map( Func&& func ) && {
    struct MapCursor : public CursorBase<MapCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<MapCursor>;
      using Base::it_;
      using typename Base::iterator;
      MapCursor() = default;
      MapCursor( Data const& data ) : func_( &data.func_ ) {}

      auto get( ChainView const& input ) const {
        rl_assert( !end( input ) );
        return ( *func_ )( *it_ );
      }

      using Base::end;
      using Base::init;
      using Base::next;
      using Base::pos;

      func_t const* func_;
    };
    return make_chain<MapCursor>( std::forward<Func>( func ) );
  }

  /**************************************************************
  ** TakeWhile
  ***************************************************************/
  template<typename Func>
  auto take_while( Func&& func ) && {
    struct TakeWhileCursor : public CursorBase<TakeWhileCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<TakeWhileCursor>;
      using Base::it_;
      using typename Base::iterator;
      TakeWhileCursor() = default;
      TakeWhileCursor( Data const& data )
        : func_( &data.func_ ), finished_{ false } {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        if( it_ == input.end() || !( *func_ )( *it_ ) )
          finished_ = true;
      }

      using Base::get;

      void next( ChainView const& input ) {
        rl_assert( !finished_ );
        rl_assert( it_ != input.end() );
        ++it_;
        if( it_ == input.end() || !( *func_ )( *it_ ) )
          finished_ = true;
      }

      bool end( ChainView const& ) const { return finished_; }

      iterator pos( ChainView const& input ) const {
        if( finished_ ) return input.end();
        return it_;
      }

      func_t const* func_;
      bool          finished_;
    };
    return make_chain<TakeWhileCursor>(
        std::forward<Func>( func ) );
  }

  /**************************************************************
  ** DropWhile
  ***************************************************************/
  template<typename Func>
  auto drop_while( Func&& func ) && {
    struct DropWhileCursor : public CursorBase<DropWhileCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<DropWhileCursor>;
      using Base::it_;
      using typename Base::iterator;
      DropWhileCursor() = default;
      DropWhileCursor( Data const& data )
        : func_( &data.func_ ) {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        while( it_ != input.end() && ( *func_ )( *it_ ) ) ++it_;
      }

      using Base::end;
      using Base::get;
      using Base::next;
      using Base::pos;

      func_t const* func_;
    };
    return make_chain<DropWhileCursor>(
        std::forward<Func>( func ) );
  }

  /**************************************************************
  ** TakeWhile Inclusive
  ***************************************************************/
  // Just like take_while, except it also includes (if present)
  // the element that caused it to stop iterating.
  template<typename Func>
  auto take_while_incl( Func&& func ) && {
    struct TakeWhileInclCursor
      : public CursorBase<TakeWhileInclCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<TakeWhileInclCursor>;
      using Base::it_;
      using typename Base::iterator;
      TakeWhileInclCursor() = default;
      TakeWhileInclCursor( Data const& data )
        : func_( &data.func_ ), state_{ e_state::taking } {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        if( it_ == input.end() ) {
          state_ = e_state::finished;
          return;
        }
        if( !( *func_ )( *it_ ) ) state_ = e_state::last;
      }

      using Base::get;

      void next( ChainView const& input ) {
        rl_assert( !end( input ) );
        if( state_ == e_state::last ) {
          state_ = e_state::finished;
          return;
        }
        ++it_;
        if( it_ == input.end() ) {
          state_ = e_state::finished;
          return;
        }
        if( !( *func_ )( *it_ ) ) {
          state_ = e_state::last;
          return;
        }
      }

      bool end( ChainView const& ) const {
        return ( state_ == e_state::finished );
      }

      iterator pos( ChainView const& input ) const {
        if( state_ == e_state::finished ) return input.end();
        return it_;
      }

      func_t const* func_;
      enum class e_state { taking, last, finished };
      e_state state_;
    };
    return make_chain<TakeWhileInclCursor>(
        std::forward<Func>( func ) );
  }

  /**************************************************************
  ** Zip
  ***************************************************************/
  template<typename SndView>
  auto zip( SndView&& snd_view ) && {
    struct ZipCursor : public CursorBase<ZipCursor> {
      struct Data {
        Data( SndView&& snd_view ) // not happy with this sig.
          : snd_view_( std::move( snd_view ) ) {}
        std::decay_t<SndView> snd_view_;
      };
      using Base = CursorBase<ZipCursor>;
      using Base::it_;
      using iterator2 = typename std::decay_t<SndView>::iterator;
      ZipCursor()     = default;
      ZipCursor( Data const& data )
        : snd_view_( &data.snd_view_ ) {}

      void init( ChainView const& input ) {
        it_  = input.begin();
        it2_ = snd_view_->begin();
      }

      auto get( ChainView const& input ) const {
        rl_assert( !end( input ) );
        return std::pair{ *it_, *it2_ };
      }

      void next( ChainView const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        ++it2_;
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() ) ||
               ( it2_ == snd_view_->end() );
      }

      using Base::pos;

      std::decay_t<SndView> const* snd_view_;
      iterator2                    it2_ = {};
    };
    return make_chain<ZipCursor>(
        std::forward<SndView>( snd_view ) );
  }

  /**************************************************************
  ** Take
  ***************************************************************/
  auto take( int n ) && {
    struct TakeCursor : public CursorBase<TakeCursor> {
      struct Data {
        Data( int m ) : n( m ) {}
        int n;
      };
      using Base = CursorBase<TakeCursor>;
      using Base::it_;
      using typename Base::iterator;
      TakeCursor() = default;
      TakeCursor( Data const& data ) : n_( data.n ) {}

      using Base::get;
      using Base::init;

      void next( ChainView const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        --n_;
        rl_assert( n_ >= 0 );
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() ) || n_ == 0;
      }

      iterator pos( ChainView const& input ) const {
        if( n_ == 0 ) return input.end();
        return it_;
      }

      int n_ = 0;
    };
    return make_chain<TakeCursor>( n );
  }

  /**************************************************************
  ** Drop
  ***************************************************************/
  auto drop( int n ) && {
    struct DropCursor : public CursorBase<DropCursor> {
      struct Data {
        Data( int m ) : n( m ) {}
        int n;
      };
      using Base = CursorBase<DropCursor>;
      using Base::it_;
      using typename Base::iterator;
      DropCursor() = default;
      DropCursor( Data const& data ) : n_( data.n ) {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        while( it_ != input.end() && n_ > 0 ) {
          ++it_;
          --n_;
        }
      }

      using Base::end;
      using Base::get;
      using Base::next;
      using Base::pos;

      int n_ = 0;
    };
    return make_chain<DropCursor>( n );
  }

  /**************************************************************
  ** Cycle
  ***************************************************************/
  auto cycle() && {
    struct CycleCursor : public CursorBase<CycleCursor> {
      struct Data {};
      using Base = CursorBase<CycleCursor>;
      using Base::it_;
      using typename Base::iterator;
      CycleCursor() = default;
      CycleCursor( Data const& ) {}

      void init( ChainView const& input ) {
        it_ = input.begin();
        // We need to have at least one element!
        rl_assert( it_ != input.end() );
      }

      using Base::get;

      void next( ChainView const& input ) {
        ++it_;
        if( it_ == input.end() ) {
          ++cycles_;
          it_ = input.begin();
        }
        rl_assert( it_ != input.end() );
      }

      bool end( ChainView const& ) const { return false; }
      auto pos( ChainView const& ) const {
        return std::pair{ cycles_, it_ };
      }

      std::intmax_t cycles_ = 0;
    };
    return make_chain<CycleCursor>();
  }

  /**************************************************************
  ** GroupBy
  ***************************************************************/
  template<typename ValueType, typename GroupByCursor>
  struct GroupByView {
    struct iterator {
      using iterator_category = std::input_iterator_tag;
      using difference_type   = int;
      using value_type        = ValueType;
      using pointer           = value_type*;
      using reference         = value_type&;

      iterator() = default;
      iterator( GroupByView const* view ) : view_( view ) {
        clear_if_end();
      }

      void clear_if_end() {
        if( view_->that_->is_group_done() ) view_ = nullptr;
      }

      auto& operator*() const {
        return view_->that_->group_get();
      }

      auto* operator->() const {
        return std::addressof( this->operator*() );
      }

      iterator& operator++() {
        view_->that_->group_advance();
        clear_if_end();
        return *this;
      }

      iterator operator++( int ) {
        auto res = *this;
        ++( *this );
        return res;
      }

      bool operator==( iterator const& rhs ) const {
        // Cannot compare two iterators that are in the middle of
        // a range, since it would introduce too much complica-
        // tion into this implementation and it probably is not
        // necessary anyway.
        rl_assert( view_ == nullptr || rhs.view_ == nullptr );
        return view_ == rhs.view_;
      }

      bool operator!=( iterator const& rhs ) const {
        return !( *this == rhs );
      }

      GroupByView const* view_ = nullptr;
    };

    GroupByView() = default;
    GroupByView( GroupByCursor* that ) : that_( that ) {}
    auto begin() const { return iterator{ this }; }
    auto end() const { return iterator{}; }

    GroupByCursor* that_;
  };

  template<typename Func>
  auto group_by( Func&& func ) && {
    struct GroupByCursor : public CursorBase<GroupByCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( f ) {}
        Data( func_t&& f ) : func_( std::move( f ) ) {}
        func_t func_;
      };
      using Base = CursorBase<GroupByCursor>;
      using Base::it_;
      using typename Base::iterator;
      using IncomingValueType = typename ChainView::value_type;
      using ChainGroupView    = ChainView<
          GroupByView<IncomingValueType, GroupByCursor>,
          IdentityCursor<
              GroupByView<IncomingValueType, GroupByCursor>>>;
      using ChainGroupViewCursor = IdentityCursor<
          GroupByView<IncomingValueType, GroupByCursor>>;
      // using value_type = ChainGroupView;
      GroupByCursor() = default;
      GroupByCursor( Data const& data )
        : func_( &data.func_ ),
          input_{},
          finished_group_{ true },
          cache_{} {}

      void init( ChainView const& input ) {
        input_ = &input;
        it_    = input.begin();
        if( it_ == input.end() ) return;
        finished_group_ = false;
        cache_          = *it_;
      }

      auto get( ChainView const& input ) const {
        rl_assert( !end( input ) );
        return ChainGroupView(
            GroupByView<IncomingValueType, GroupByCursor>{
                const_cast<GroupByCursor*>( this ) },
            typename ChainGroupViewCursor::Data{} );
      }

      void next( ChainView const& input ) {
        rl_assert( finished_group_ );
        if( it_ == input.end() ) return;
        finished_group_ = false;
        cache_          = *it_;
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() );
      }

      iterator pos( ChainView const& ) const { return it_; }

      // === Functions for Working with Single Group View ===

      bool is_group_done() const { return finished_group_; }

      auto& group_get() const {
        rl_assert( !finished_group_ );
        return *it_;
      }

      void group_advance() {
        rl_assert( !finished_group_ );
        ++it_;
        if( it_ == input_->end() || !( *func_ )( cache_, *it_ ) )
          finished_group_ = true;
      }

      func_t const*                  func_;
      ChainView const*               input_;
      bool                           finished_group_;
      typename ChainView::value_type cache_;
    };
    return make_chain<GroupByCursor>(
        std::forward<Func>( func ) );
  }
};

/****************************************************************
** Free-standing view factories.
*****************************************************************/
auto ints( int start = 0,
           int end   = std::numeric_limits<int>::max() ) {
  using Cursor = IdentityCursor<IntsView>;
  using Data   = typename Cursor::Data;
  return ChainView<IntsView, Cursor>( IntsView( start, end ),
                                      Data{} );
}

template<typename InputView,
         typename T = typename std::remove_reference_t<
             InputView>::value_type>
auto view( InputView&& input ) {
  if constexpr( is_rl_view_v<std::decay_t<InputView>> ) {
    return std::forward<InputView>( input );
  } else {
    using initial_view_t = std::span<T>;
    using Data = typename IdentityCursor<initial_view_t>::Data;
    return ChainView<initial_view_t,
                     IdentityCursor<initial_view_t>>(
        initial_view_t( input ), Data{} );
  }
}

template<typename LeftView, typename RightView>
auto zip( LeftView&& left_view, RightView&& right_view ) {
  return view( std::forward<LeftView>( left_view ) )
      .zip( view( std::forward<RightView>( right_view ) ) );
}

// Reverse view.
template<typename InputView,
         typename T = typename std::remove_reference_t<
             InputView>::value_type>
auto rview( InputView&& input ) {
  if constexpr( is_rl_view_v<std::decay_t<InputView>> ) {
    static_assert(
        sizeof( InputView ) != sizeof( InputView ),
        "rl::ChainView does not support reverse iteration." );
  } else {
    using initial_view_t = std::span<T>;
    using Data =
        typename ReverseIdentityCursor<initial_view_t>::Data;
    return ChainView<initial_view_t,
                     ReverseIdentityCursor<initial_view_t>>(
        initial_view_t( input ), Data{} );
  }
}

template<typename InputView>
auto rview( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    void> = delete;

} // namespace base::rl