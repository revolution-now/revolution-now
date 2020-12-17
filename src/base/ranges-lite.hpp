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
#include "lambda.hpp"
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
#  define rl_assert( ... )
#endif

/****************************************************************
** TODO list
*****************************************************************/
//
//      2 rv::sliding
//      2 rv::intersperse
//
//      1 rv::keys
//      1 rv::join
//      1 rv::iota
//      1 rv::group_by
//      1 rv::drop_while
//      1 rv::drop_last
//      1 rv::concat
//      1 rg::distance
//      1 rg::any_of

/****************************************************************
** Forward Declarations
*****************************************************************/
template<typename InputView, typename Cursor>
class View;

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
struct is_rl_view<View<InputView, Cursor>> : std::true_type {};

template<typename T>
constexpr bool is_rl_view_v = is_rl_view<T>::value;

/****************************************************************
** Macros
*****************************************************************/
#define RL_LAMBDA( name, ... ) name( LC( __VA_ARGS__ ) )

// FIXME: rename this with a suffix instead of prefix.
#define rl_keep( ... ) RL_LAMBDA( keep, __VA_ARGS__ )
#define rl_remove( ... ) RL_LAMBDA( remove, __VA_ARGS__ )
#define rl_map( ... ) RL_LAMBDA( map, __VA_ARGS__ )
#define rl_take_while( ... ) RL_LAMBDA( take_while, __VA_ARGS__ )
#define rl_take_while_incl( ... ) \
  RL_LAMBDA( take_while_incl, __VA_ARGS__ )

/****************************************************************
** Identity Cursor
*****************************************************************/
template<typename InputView>
struct IdentityCursor {
  using iterator   = typename InputView::iterator;
  using value_type = typename iterator::value_type;
  void init( InputView& input ) { it = input.begin(); }
  value_type const& get( InputView const& ) const { return *it; }
  void              next( InputView const& ) { ++it; }
  bool              end( InputView const& input ) const {
    return it == input.end();
  }
  iterator pos( InputView const& ) const { return it; }
  iterator it;
};

template<typename InputView>
struct ReverseIdentityCursor {
  using iterator   = typename InputView::reverse_iterator;
  using value_type = typename iterator::value_type;
  void init( InputView& input ) { it = input.rbegin(); }
  value_type const& get( InputView const& ) const { return *it; }
  void              next( InputView const& ) { ++it; }
  bool              end( InputView const& input ) const {
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
  int       cursor_;

public:
  IntsView( int start = 0,
            int end   = std::numeric_limits<int>::max() )
    : start_( start ), end_( end ), cursor_( start ) {
    rl_assert( start <= end );
  }

  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = int;
    using pointer           = value_type const*;
    using reference         = value_type const&;

    iterator() = default;
    iterator( IntsView* view ) : view_( view ) {
      view_->cursor_ = view_->start_;
      clear_if_end();
    }

    void clear_if_end() {
      if( view_->cursor_ >= view_->end_ ) view_ = nullptr;
    }

    auto const& operator*() const {
      rl_assert( view_->cursor_ < view_->end_ );
      return view_->cursor_;
    }

    iterator& operator++() {
      rl_assert( view_->cursor_ < view_->end_ );
      view_->cursor_++;
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
        return view_->cursor_ == rhs.view_->cursor_;
      return view_ == rhs.view_;
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    IntsView* view_ = nullptr;
  };

  using const_iterator = iterator;
  iterator begin() { return iterator( this ); }
  iterator end() const { return iterator(); }
};

/****************************************************************
** View
*****************************************************************/
template<typename InputView, typename Cursor>
class View {
  InputView input_;
  Cursor    cursor_;

public:
  View( InputView&& input, Cursor&& cursor )
    : input_( std::move( input ) ),
      cursor_( std::move( cursor ) ) {}

  using value_type =
      std::decay_t<decltype( std::declval<Cursor>().get(
          std::declval<InputView>() ) )>;

  using ultimate_view_t = ultimate_view_or_self_t<InputView>;

  static auto create( ultimate_view_t const& input ) {
    if constexpr( std::is_default_constructible_v<Cursor> ) {
      if constexpr( std::is_same_v<ultimate_view_t,
                                   InputView> ) {
        return View{ InputView( input ), Cursor{} };
      } else {
        return View{ InputView::create( input ), Cursor{} };
      }
    } else {
      // After C++20 stateless lambdas should be default con-
      // structible, which will make this feature usable more of-
      // ten. Until then, you must use regular functions (or
      // function pointers) instead of lambdas.
      struct CanOnlyUseThisWithDefaultConstructibleLambdas {};
      return CanOnlyUseThisWithDefaultConstructibleLambdas{};
    }
  }

  /**************************************************************
  ** Iterator
  ***************************************************************/
  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = View::value_type;
    using pointer           = value_type const*;
    using reference         = value_type const&;

    Iterator() = default;
    Iterator( View* view ) : view_( view ) {
      view_->cursor_.init( view_->input_ );
      clear_if_end();
    }

    void clear_if_end() {
      if( view_->cursor_.end( view_->input_ ) ) view_ = nullptr;
    }

    auto const& operator*() const {
      rl_assert( view_ != nullptr );
      return view_->cursor_.get( view_->input_ );
    }

    Iterator& operator++() {
      rl_assert( view_ != nullptr );
      view_->cursor_.next( view_->input_ );
      clear_if_end();
      return *this;
    }

    Iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( Iterator const& rhs ) const {
      if( view_ != nullptr && rhs.view_ != nullptr )
        return view_->cursor_.pos( view_->input_ ) ==
               rhs.view_->cursor_.pos( rhs.view_->input_ );
      return view_ == rhs.view_;
    }

    bool operator!=( Iterator const& rhs ) const {
      return !( *this == rhs );
    }

    View* view_ = nullptr;
  };

  using const_iterator = Iterator;
  using iterator       = Iterator;
  Iterator begin() { return Iterator( this ); }
  Iterator end() const { return Iterator(); }

  View copy() const { return *this; }

  /**************************************************************
  ** Materialization
  ***************************************************************/
  std::vector<value_type> to_vector() {
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
  maybe<value_type> head() {
    maybe<value_type> res;

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
    using iterator   = typename View::iterator;
    using value_type = typename View::value_type;

    CursorBase() = default;

    Derived const* derived() const {
      return static_cast<Derived const*>( this );
    }

    void init( View& input ) { it_ = input.begin(); }

    value_type const& get( View const& input ) const {
      rl_assert( !derived()->end( input ) );
      return *it_;
    }

    void next( View const& input ) {
      rl_assert( !derived()->end( input ) );
      ++it_;
    }

    bool end( View const& input ) const {
      return it_ == input.end();
    }

    // The type that this returns doesn't really matter, it just
    // has to be a unique value for each iteration of the cursor.
    iterator pos( View const& ) const { return it_; }

    iterator it_;
  };

  /**************************************************************
  ** Chain Maker
  ***************************************************************/
private:
  template<typename NewCursor, typename... Args>
  auto make_chain( Args&&... args ) {
    using NewView = View<View<InputView, Cursor>, NewCursor>;
    return NewView( std::move( *this ),
                    NewCursor( std::forward<Args>( args )... ) );
  }

public:
  /**************************************************************
  ** Keep
  ***************************************************************/
  template<typename Func>
  auto keep( Func&& func ) && {
    struct KeepCursor : public CursorBase<KeepCursor> {
      using Base = CursorBase<KeepCursor>;
      using Base::it_;
#ifdef __clang__ // C++20
      KeepCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      KeepCursor( std::remove_reference_t<Func> const& f )
        : func_( f ) {}
      KeepCursor( std::remove_reference_t<Func>&& f )
        : func_( std::move( f ) ) {}

      void init( View& input ) {
        it_ = input.begin();
        do {
          if( end( input ) ) break;
          if( func_( *it_ ) ) break;
          ++it_;
        } while( true );
      }

      void next( View const& input ) {
        rl_assert( !end( input ) );
        do {
          ++it_;
          if( end( input ) ) break;
          if( func_( *it_ ) ) break;
        } while( true );
      }

      using Base::end;
      using Base::get;
      using Base::pos;

      std::remove_reference_t<Func> func_;
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
    return std::move( *this )
        .zip( View<IntsView, IntsCursor>( IntsView( start ),
                                          IntsCursor{} ) )
        .map( []( auto&& p ) {
          return std::pair{ p.second, p.first };
        } );
  }

  /**************************************************************
  ** Map
  ***************************************************************/
  template<typename Func>
  auto map( Func&& func ) && {
    struct MapCursor : public CursorBase<MapCursor> {
      using Base = CursorBase<MapCursor>;
      using Base::it_;
      using typename Base::iterator;
      using value_type =
          std::invoke_result_t<Func,
                               typename iterator::value_type>;
#ifdef __clang__ // C++20
      MapCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      MapCursor( std::remove_reference_t<Func> const& f )
        : func_( f ) {}
      MapCursor( std::remove_reference_t<Func>&& f )
        : func_( std::move( f ) ) {}

      void init( View& input ) {
        it_ = input.begin();
        if( !end( input ) ) cache = func_( *it_ );
      }

      value_type const& get( View const& input ) const {
        rl_assert( !end( input ) );
        return cache;
      }

      void next( View const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        if( !end( input ) ) cache = func_( *it_ );
      }

      using Base::end;
      using Base::pos;

      std::remove_reference_t<Func> func_;
      value_type                    cache;
    };
    return make_chain<MapCursor>( std::forward<Func>( func ) );
  }

  /**************************************************************
  ** TakeWhile
  ***************************************************************/
  template<typename Func>
  auto take_while( Func&& func ) && {
    struct TakeWhileCursor : public CursorBase<TakeWhileCursor> {
      using Base = CursorBase<TakeWhileCursor>;
      using Base::it_;
      using typename Base::iterator;
#ifdef __clang__ // C++20
      TakeWhileCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      TakeWhileCursor( std::remove_reference_t<Func> const& f )
        : func_( f ), finished_{ false } {}
      TakeWhileCursor( std::remove_reference_t<Func>&& f )
        : func_( std::move( f ) ), finished_{ false } {}

      void init( View& input ) {
        it_ = input.begin();
        if( it_ == input.end() || !func_( *it_ ) )
          finished_ = true;
      }

      using Base::get;

      void next( View const& input ) {
        rl_assert( !finished_ );
        rl_assert( it_ != input.end() );
        ++it_;
        if( it_ == input.end() || !func_( *it_ ) )
          finished_ = true;
      }

      bool end( View const& ) const { return finished_; }

      iterator pos( View const& input ) const {
        if( finished_ ) return input.end();
        return it_;
      }

      std::remove_reference_t<Func> func_;
      bool                          finished_;
    };
    return make_chain<TakeWhileCursor>(
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
      using Base = CursorBase<TakeWhileInclCursor>;
      using Base::it_;
      using typename Base::iterator;
#ifdef __clang__ // C++20
      TakeWhileInclCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      TakeWhileInclCursor(
          std::remove_reference_t<Func> const& f )
        : func_( f ), state_{ e_state::taking } {}
      TakeWhileInclCursor( std::remove_reference_t<Func>&& f )
        : func_( std::move( f ) ), state_{ e_state::taking } {}

      void init( View& input ) {
        it_ = input.begin();
        if( it_ == input.end() ) {
          state_ = e_state::finished;
          return;
        }
        if( !func_( *it_ ) ) state_ = e_state::last;
      }

      using Base::get;

      void next( View const& input ) {
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
        if( !func_( *it_ ) ) {
          state_ = e_state::last;
          return;
        }
      }

      bool end( View const& ) const {
        return ( state_ == e_state::finished );
      }

      iterator pos( View const& input ) const {
        if( state_ == e_state::finished ) return input.end();
        return it_;
      }

      std::remove_reference_t<Func> func_;
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
      using Base = CursorBase<ZipCursor>;
      using Base::it_;
      using iterator2 = typename std::decay_t<SndView>::Iterator;
      using value_type1 = typename View::value_type;
      using value_type2 =
          typename std::decay_t<SndView>::value_type;
      using value_type = std::pair<value_type1, value_type2>;
#ifdef __clang__ // C++20
      ZipCursor() /* requires? */ = default;
#endif
      ZipCursor( SndView&& snd_view )
        : snd_view_( std::move( snd_view ) ) {}

      void update_cache( View const& input ) {
        if( !end( input ) ) cache_ = { *it_, *it2_ };
      }

      void init( View& input ) {
        it_  = input.begin();
        it2_ = snd_view_.begin();
        update_cache( input );
      }

      value_type const& get( View const& input ) const {
        rl_assert( !end( input ) );
        return cache_;
      }

      void next( View const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        ++it2_;
        update_cache( input );
      }

      bool end( View const& input ) const {
        return ( it_ == input.end() ) ||
               ( it2_ == snd_view_.end() );
      }

      using Base::pos;

      iterator2             it2_;
      std::decay_t<SndView> snd_view_;
      value_type            cache_;
    };
    return make_chain<ZipCursor>(
        std::forward<SndView>( snd_view ) );
  }

  /**************************************************************
  ** Take
  ***************************************************************/
  auto take( int n ) && {
    struct TakeCursor : public CursorBase<TakeCursor> {
      using Base = CursorBase<TakeCursor>;
      using Base::it_;
      using typename Base::iterator;
      TakeCursor() = default;
      TakeCursor( int n ) : n_( n ) {}

      using Base::get;
      using Base::init;

      void next( View const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        --n_;
        rl_assert( n_ >= 0 );
      }

      bool end( View const& input ) const {
        return ( it_ == input.end() ) || n_ == 0;
      }

      iterator pos( View const& input ) const {
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
      using Base = CursorBase<DropCursor>;
      using Base::it_;
      using typename Base::iterator;
      DropCursor() = default;
      DropCursor( int n ) : n_( n ) {}

      void init( View& input ) {
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
      using Base = CursorBase<CycleCursor>;
      using Base::it_;
      using typename Base::iterator;
#ifdef __clang__ // C++20
      CycleCursor() /* requires? */ = default;
#endif
      CycleCursor( View* input_view )
        : view_pristine_( *input_view ) {}

      void init( View& input ) {
        it_ = input.begin();
        // We need to have at least one element!
        rl_assert( it_ != input.end() );
      }

      using Base::get;

      void next( View& input ) {
        ++it_;
        if( it_ == input.end() ) {
          ++cycles_;
          input = view_pristine_;
          init( input );
        }
        rl_assert( it_ != input.end() );
      }

      bool end( View const& ) const { return false; }
      auto pos( View const& ) const {
        return std::pair{ cycles_, it_ };
      }

      View          view_pristine_;
      std::intmax_t cycles_ = 0;
    };
    // Careful here... this function will move *this, but the
    // `this` pointer being passed to it should be used to con-
    // struct the CycleCursor before that happens, and the Cycle-
    // Cursor will not retain the pointer itself but will make a
    // copy of the View, so everything should be fine.
    return make_chain<CycleCursor>( this );
  }
};

/****************************************************************
** Free-standing view factories.
*****************************************************************/
auto ints( int start = 0,
           int end   = std::numeric_limits<int>::max() ) {
  using Cursor = IdentityCursor<IntsView>;
  return View<IntsView, Cursor>( IntsView( start, end ),
                                 Cursor{} );
}

template<typename InputView,
         typename T = typename std::remove_reference_t<
             InputView>::value_type>
auto view( InputView&& input ) {
  if constexpr( is_rl_view_v<std::decay_t<InputView>> ) {
    return std::forward<InputView>( input );
  } else {
    using initial_view_t = std::span<T const>;
    return View( initial_view_t( input ),
                 IdentityCursor<initial_view_t>{} );
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
auto rview( InputView const& input ) {
  if constexpr( is_rl_view_v<std::decay_t<InputView>> ) {
    static_assert(
        sizeof( InputView ) != sizeof( InputView ),
        "rl::View does not support reverse iteration." );
  } else {
    using initial_view_t = std::span<T const>;
    return View( initial_view_t( input ),
                 ReverseIdentityCursor<initial_view_t>{} );
  }
}

template<typename InputView>
auto rview( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    void> = delete;

} // namespace base::rl