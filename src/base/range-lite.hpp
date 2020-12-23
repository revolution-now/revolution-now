/****************************************************************
**range-lite.hpp
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
#include <tuple>

// rl stands for "range lite".
namespace base::rl {

/****************************************************************
** Macros
*****************************************************************/
#define RL_LAMBDA( name, ... ) \
  name( [&]( auto&& _ ) { return __VA_ARGS__; } )

#define RL_LAMBDA2( name, ... ) \
  name( [&]( auto&& _1, auto&& _2 ) { return __VA_ARGS__; } )

#define keep_if_L( ... ) RL_LAMBDA( keep_if, __VA_ARGS__ )
#define min_by_L( ... ) RL_LAMBDA( min_by, __VA_ARGS__ )
#define max_by_L( ... ) RL_LAMBDA( max_by, __VA_ARGS__ )
#define filter_L( ... ) RL_LAMBDA( filter, __VA_ARGS__ )
#define group_by_L( ... ) RL_LAMBDA2( group_by, __VA_ARGS__ )
#define group_on_L( ... ) RL_LAMBDA( group_on, __VA_ARGS__ )
#define remove_if_L( ... ) RL_LAMBDA( remove_if, __VA_ARGS__ )
#define map_L( ... ) RL_LAMBDA( map, __VA_ARGS__ )
#define map2val_L( ... ) RL_LAMBDA( map2val, __VA_ARGS__ )
#define take_while_L( ... ) RL_LAMBDA( take_while, __VA_ARGS__ )
#define drop_while_L( ... ) RL_LAMBDA( drop_while, __VA_ARGS__ )
#define take_while_incl_L( ... ) \
  RL_LAMBDA( take_while_incl, __VA_ARGS__ )

/****************************************************************
** Forward Declarations
*****************************************************************/
template<typename InputView, typename Cursor, typename ValueType>
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
struct is_chain_view : std::false_type {};

template<typename InputView, typename Cursor, typename ValueType>
struct is_chain_view<ChainView<InputView, Cursor, ValueType>>
  : std::true_type {};

template<typename T>
constexpr bool is_chain_view_v = is_chain_view<T>::value;

template<typename T, typename = void>
struct view_supports_reverse : std::false_type {};

template<typename T>
struct view_supports_reverse<
    T, std::enable_if_t<
           !std::is_same_v<
               decltype( std::declval<T>().rbegin() ), void>,
           void>> : std::true_type {};

template<typename T>
constexpr bool view_supports_reverse_v =
    view_supports_reverse<T>::value;

template<typename T, typename = void>
struct cursor_supports_reverse : std::false_type {};

template<typename T>
struct cursor_supports_reverse<
    T,
    std::void_t<decltype( std::declval<typename T::riterator>().
                          operator++() )>> : std::true_type {};

template<typename T>
constexpr bool cursor_supports_reverse_v =
    cursor_supports_reverse<T>::value;

template<typename It>
struct it_type_to_value_type {
  using type = typename It::value_type;
};

template<typename P>
struct it_type_to_value_type<P*> {
  using type = P;
};

template<typename It>
using it_type_to_value_type_t =
    typename it_type_to_value_type<It>::type;

/****************************************************************
** Identity Cursor
*****************************************************************/
template<typename InputView>
struct IdentityCursor {
  struct Data {};
  using iterator = decltype( std::declval<InputView>().begin() );
  using value_type = it_type_to_value_type_t<iterator>;
  IdentityCursor() = default;
  IdentityCursor( Data const& ) {}
  void init( InputView const& input ) { it_ = input.begin(); }
  decltype( auto ) get( InputView const& ) const { return *it_; }
  void             next( InputView const& ) { ++it_; }
  bool             end( InputView const& input ) const {
    return it_ == input.end();
  }
  iterator pos() const { return it_; }
  iterator it_;
};

template<typename InputView>
struct BidirectionalIdentityCursor {
  struct Data {};
  using iterator = decltype( std::declval<InputView>().begin() );
  using riterator =
      decltype( std::declval<InputView>().rbegin() );
  using value_type = it_type_to_value_type_t<iterator>;
  BidirectionalIdentityCursor() = default;
  BidirectionalIdentityCursor( Data const& ) {}

  // Forward
  void init( InputView const& input ) { it_ = input.begin(); }
  decltype( auto ) get( InputView const& ) const { return *it_; }
  void             next( InputView const& ) { ++it_; }
  bool             end( InputView const& input ) const {
    return it_ == input.end();
  }
  iterator pos() const { return it_; }

  // Backward
  void rinit( InputView const& input ) { rit_ = input.rbegin(); }
  decltype( auto ) rget( InputView const& ) const {
    return *rit_;
  }
  void rnext( InputView const& ) { ++rit_; }
  bool rend( InputView const& input ) const {
    return rit_ == input.rend();
  }
  riterator rpos() const { return rit_; }

  iterator  it_;
  riterator rit_;
};

/****************************************************************
** AllView
*****************************************************************/
// Takes any range and yields a view of all elements in it.
template<typename InputView>
class AllView {
  InputView* view_;

public:
  AllView( InputView* view ) : view_( view ) {}

  using ultimate_view_t = InputView;

  void attach( ultimate_view_t& input ) & { view_ = &input; }

  static auto create( ultimate_view_t& input ) {
    return AllView( &input );
  }

  using iterator = decltype( std::declval<InputView>().begin() );

  auto begin() const { return view_->begin(); }
  auto end() const { return view_->end(); }
  auto cbegin() const { return view_->cbegin(); }
  auto cend() const { return view_->cend(); }
};

// Takes any range and yields a view of all elements in it.
template<typename InputView>
class BidirectionalAllView {
  InputView* view_;

public:
  BidirectionalAllView( InputView* view ) : view_( view ) {}

  using ultimate_view_t = InputView;

  void attach( ultimate_view_t& input ) & { view_ = &input; }

  static auto create( ultimate_view_t& input ) {
    return BidirectionalAllView( &input );
  }

  using iterator = decltype( std::declval<InputView>().begin() );

  auto begin() const { return view_->begin(); }
  auto end() const { return view_->end(); }
  auto cbegin() const { return view_->cbegin(); }
  auto cend() const { return view_->cend(); }

  using riterator =
      decltype( std::declval<InputView>().rbegin() );

  auto rbegin() const { return view_->rbegin(); }
  auto rend() const { return view_->rend(); }
  auto crbegin() const { return view_->crbegin(); }
  auto crend() const { return view_->crend(); }
};

// Takes any range and yields a view of all elements in it
// in reverse.
template<typename InputView>
class ReverseAllView {
  InputView* view_;

public:
  ReverseAllView( InputView* view ) : view_( view ) {
    assert_bt( view_ != nullptr );
  }

  using ultimate_view_t = InputView;

  static auto create( ultimate_view_t& input ) {
    return AllView( &input );
  }

  using iterator =
      decltype( std::declval<InputView>().rbegin() );

  auto begin() const { return view_->rbegin(); }
  auto end() const { return view_->rend(); }
  auto cbegin() const { return view_->crbegin(); }
  auto cend() const { return view_->crend(); }
};

/****************************************************************
** IntsView
*****************************************************************/
// A fundamental view that just produces increasing ints.
class IntsView {
  const int start_;
  const int end_;

public:
  IntsView( int start = 0,
            int end   = std::numeric_limits<int>::max() )
    : start_( start ), end_( end ) {
    assert_bt( start <= end );
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

    decltype( auto ) operator*() const {
      assert_bt( cursor_ < view_->end_ );
      return cursor_;
    }

    iterator& operator++() {
      assert_bt( cursor_ < view_->end_ );
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

  iterator begin() const { return iterator( this ); }
  iterator end() const { return iterator(); }
  iterator cbegin() const { return iterator( this ); }
  iterator cend() const { return iterator(); }
};

/****************************************************************
** GenerateView
*****************************************************************/
// Given  a nullary function and a count, return a range that
// gen- erates  the  requested  number of elements by calling the
// func- tion.
template<typename Func>
class GenerateView {
  Func      func_;
  const int count_;

public:
  template<typename T>
  GenerateView( T&& func, int count )
    : func_( std::forward<T>( func ) ), count_( count ) {}

  class iterator {
    void clear_if_end() {
      if( n_to_go_ <= 0 ) view_ = nullptr;
    }

    void populate() { cache_ = view_->func_(); }

  public:
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = std::invoke_result_t<Func>;
    using pointer           = value_type const*;
    using reference         = value_type const&;

    iterator() = default;
    iterator( GenerateView const* view )
      : view_( view ), n_to_go_( view->count_ ), cache_{} {
      clear_if_end();
      if( view_ == nullptr ) return;
      populate();
    }

    decltype( auto ) operator*() const {
      assert_bt( view_ != nullptr );
      return cache_;
    }

    iterator& operator++() {
      assert_bt( view_ != nullptr );
      assert_bt( n_to_go_ > 0 );
      --n_to_go_;
      clear_if_end();
      if( view_ != nullptr ) populate();
      return *this;
    }

    iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( iterator const& rhs ) const {
      // Cannot compare two iterators that are in the middle of a
      // range, since it would introduce  too  much  complication
      // into this implementation and  it  probably  is not
      // neces- sary anyway.
      assert_bt( view_ == nullptr || rhs.view_ == nullptr );
      return view_ == rhs.view_;
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    GenerateView const* view_    = nullptr;
    int                 n_to_go_ = 0;
    value_type          cache_   = {};
  };

  iterator begin() const { return iterator( this ); }
  iterator end() const { return iterator(); }
  iterator cbegin() const { return iterator( this ); }
  iterator cend() const { return iterator(); }
};

/****************************************************************
** ChildView
*****************************************************************/
// This view is used by views that need to produce other views as
// their value type, e.g. group_by. `Cursor` is the type of the
// Cursor that produces this view as its value type.
template<typename ValueType, typename Cursor>
struct ChildView {
  struct iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = ValueType;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator() = default;
    iterator( Cursor cursor ) : cursor_( std::move( cursor ) ) {
      finished_ = false;
      clear_if_end();
    }

    void clear_if_end() {
      if( cursor_.child_view_done() ) finished_ = true;
    }

    decltype( auto ) operator*() const {
      assert_bt( !finished_ );
      return cursor_.child_view_get();
    }

    auto* operator->() const {
      return std::addressof( this->operator*() );
    }

    iterator& operator++() {
      assert_bt( !finished_ );
      cursor_.child_view_advance();
      clear_if_end();
      return *this;
    }

    iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( iterator const& rhs ) const {
      // Cannot compare two iterators that are in the middle of a
      // range, since it would introduce  too  much  complication
      // into this implementation and  it  probably  is not
      // neces- sary anyway.
      assert_bt( finished_ == true || rhs.finished_ == true );
      return finished_ == rhs.finished_;
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    Cursor cursor_;
    bool   finished_ = true;
  };

  ChildView() = default;
  ChildView( Cursor const* that ) : that_( that ) {}
  auto begin() const { return iterator{ *that_ }; }
  auto end() const { return iterator{}; }

  Cursor const* that_;
};

/****************************************************************
** ChainView
*****************************************************************/
template<typename InputView, typename Cursor,
         // The vast majority of the time you don't have to
         // specify ValueType explicitly -- just let it assume
         // its default value. There are a few strange cases
         // where specifying it is needed in order to break infi-
         // nite (cyclic) template lookups of value type (e.g. in
         // group_by).
         typename ValueType = std::decay_t<std::invoke_result_t<
             decltype( &Cursor::get ), Cursor*, InputView>>>
class ChainView {
  using data_t = typename Cursor::Data;

  InputView input_;
  data_t    data_;

public:
  ChainView( InputView&& input, data_t&& data )
    : input_( std::move( input ) ), data_( std::move( data ) ) {}

  using value_type = ValueType;

  // This gives us the most upstream range/container from which
  // we are ultimately reading data to be fed down the chain.
  using ultimate_view_t = ultimate_view_or_self_t<InputView>;

  /**************************************************************
  ** Attach
  ***************************************************************/
  void attach( ultimate_view_t& input ) & {
    input_.attach( input );
  }

  /**************************************************************
  ** Reconstitution
  ***************************************************************/
  // The machinery in this section will allow creating and/or re-
  // constituting a view pipeline from the type only. This is
  // only possible when none of the views in the pipeline contain
  // state (i.e., data that would not make sense to just default
  // construct). Stateless lambdas, however, are ok.
  static auto create( ultimate_view_t& input ) {
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
      static_assert(
          sizeof( data_t ) != sizeof( data_t ),
          "You cannot use static create here because one of the "
          "views in the chain has state that cannot be "
          "reconstituted through default-construction." );
      struct YouCannotUseStaticCreateHere {};
      return YouCannotUseStaticCreateHere{};
    }
  }

  /**************************************************************
  ** iterator
  ***************************************************************/
  class iterator_sentinel {};

  class iterator {
    ChainView const* view_ = nullptr;
    Cursor           cursor_;

  public:
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = ChainView::value_type;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator() = default;
    iterator( ChainView const* view )
      : view_( view ), cursor_( view->data_ ) {
      cursor_.init( view_->input_ );
    }

    decltype( auto ) operator*() const {
      return cursor_.get( view_->input_ );
    }

    auto* operator->() const {
      return std::addressof( this->operator*() );
    }

    iterator& operator++() {
      cursor_.next( view_->input_ );
      return *this;
    }

    iterator operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( iterator const& rhs ) const {
      return cursor_.pos() == rhs.cursor_.pos();
    }

    bool operator==( iterator_sentinel const& ) const {
      return cursor_.end( view_->input_ );
    }

    bool operator!=( iterator const& rhs ) const {
      return !( *this == rhs );
    }

    bool operator!=( iterator_sentinel const& rhs ) const {
      return !( *this == rhs );
    }
  };

  /**************************************************************
  ** reverse iterator
  ***************************************************************/
  class riterator_sentinel {};

  template<typename Defer = void>
  class riterator_defer {
    ChainView const* view_ = nullptr;
    Cursor           cursor_;

  public:
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = ChainView::value_type;
    using pointer           = value_type*;
    using reference         = value_type&;

    riterator_defer() = default;
    riterator_defer( ChainView const* view )
      : view_( view ), cursor_( view->data_ ) {
      cursor_.rinit( view_->input_ );
    }

    decltype( auto ) operator*() const {
      return cursor_.rget( view_->input_ );
    }

    auto* operator->() const {
      return std::addressof( this->operator*() );
    }

    riterator_defer& operator++() {
      cursor_.rnext( view_->input_ );
      return *this;
    }

    riterator_defer operator++( int ) {
      auto res = *this;
      ++( *this );
      return res;
    }

    bool operator==( riterator_defer const& rhs ) const {
      return cursor_.rpos() == rhs.cursor_.rpos();
    }

    bool operator!=( riterator_defer const& rhs ) const {
      return !( *this == rhs );
    }

    bool operator==( riterator_sentinel const& ) const {
      return cursor_.rend( view_->input_ );
    }

    bool operator!=( riterator_sentinel const& rhs ) const {
      return !( *this == rhs );
    }
  };

  static constexpr auto make_riterator( ChainView const* view ) {
    if constexpr( cursor_supports_reverse_v<Cursor> ) {
      return riterator_defer<>( view );
    }
  }

  using riterator =
      decltype( make_riterator( std::declval<ChainView*>() ) );

  auto begin() const { return iterator( this ); }
  auto end() const { return iterator_sentinel{}; }
  auto cbegin() const { return iterator( this ); }
  auto cend() const { return iterator_sentinel{}; }

  auto rbegin() const { return make_riterator( this ); }
  auto rend() const { return riterator_sentinel{}; }
  auto crbegin() const { return make_riterator( this ); }
  auto crend() const { return riterator_sentinel{}; }

  /**************************************************************
  ** Copy/Move this view
  ***************************************************************/
  auto copy_me() const { return *this; }

  auto&& move_me() { return std::move( *this ); }

  /**************************************************************
  ** Materialization
  ***************************************************************/
  std::vector<value_type> to_vector() const {
    std::vector<value_type> res;

    auto it = this->begin();
    if( it == this->end() ) return res;
    res.reserve( 10 ); // heuristic.
    for( ; it != this->end(); ++it ) res.push_back( *it );
    return res;
  }

  std::string to_string() const
      requires( std::is_convertible_v<value_type, char> ) {
    std::string res;
    for( char c : *this ) res.push_back( c );
    return res;
  }

  // For std::vector or std::string use `to_vector`/`to_string`
  // instead. NOTE: this may cause problems for containers that
  // cannot accept iterators with different types for begin/end.
  template<typename T>
  T to() const {
    return T( this->begin(), this->end() );
  }

  /**************************************************************
  ** Distance (NOT O(1))
  ***************************************************************/
  int distance() const {
    int distance = 0;
    for( auto&& e : *this ) {
      (void)e;
      ++distance;
    }
    return distance;
  }

  /**************************************************************
  ** MinBy/MaxBy
  ***************************************************************/
  // These functions apply `f` to each element in the range to
  // derive keys, then they find the min/max key, and return the
  // range value corresponding to that key.
  template<typename Func>
  auto min_by( Func&& f ) const {
    using KeyType = std::invoke_result_t<Func, ValueType>;
    maybe<ValueType> res{};
    maybe<KeyType>   min_key{};
    for( auto const& elem : *this ) {
      auto key = f( elem );
      if( !min_key.has_value() || key < *min_key ) {
        min_key = key;
        res     = elem;
      }
    }
    return res;
  }

  template<typename Func>
  auto max_by( Func&& f ) const {
    using KeyType = std::invoke_result_t<Func, ValueType>;
    maybe<ValueType> res{};
    maybe<KeyType>   max_key{};
    for( auto const& elem : *this ) {
      auto key = f( elem );
      if( !max_key.has_value() || key > *max_key ) {
        max_key = key;
        res     = elem;
      }
    }
    return res;
  }

  /**************************************************************
  ** Min/Max
  ***************************************************************/
  auto min() const {
    return min_by( []<typename T>( T&& _ ) {
      return std::forward<T>( _ );
    } );
  }

  auto max() const {
    return max_by( []<typename T>( T&& _ ) {
      return std::forward<T>( _ );
    } );
  }

  /**************************************************************
  ** Accumulate
  ***************************************************************/
  template<typename Op    = std::plus<>,
           typename InitT = value_type>
  auto accumulate( Op&& op = {}, InitT init = {} ) {
    value_type res = init;
    for( auto const& e : *this )
      res = std::invoke( std::forward<Op>( op ), res, e );
    return res;
  }

  /**************************************************************
  ** Accumulate Monoid
  ***************************************************************/
  template<typename Op = std::plus<>>
  maybe<value_type> accumulate_monoid( Op&& op = {} ) {
    maybe<value_type> res;
    auto              it = this->begin();
    if( it == this->end() ) return res;
    res = *it;
    ++it;
    for( ; it != this->end(); ++it )
      *res = std::invoke( std::forward<Op>( op ), *res, *it );
    return res;
  }

  /**************************************************************
  ** Head
  ***************************************************************/
  // Will return a maybe. The type inside the maybe depends on
  // the value category of the value being produced by this view.
  // It could be a reference, or a value. It will only be a ref-
  // erence if this view is producing a reference.
  auto head() const {
    auto it     = begin();
    using res_t = maybe<decltype( *it )>;
    if( it == end() ) return res_t{};
    return res_t( *it );
  }

  /**************************************************************
  ** CursorBase
  ***************************************************************/
private:
  // Used to remove some redundancy from Cursors.  Uses CRTP.
  template<typename Derived>
  class CursorBase {
    Derived const* derived() const {
      return static_cast<Derived const*>( this );
    }

  public:
    using iterator = typename ChainView::iterator;

    CursorBase() = default;

    void init( ChainView const& input ) { it_ = input.begin(); }

    decltype( auto ) get( ChainView const& input ) const {
      assert_bt( !derived()->end( input ) );
      return *it_;
    }

    void next( ChainView const& input ) {
      assert_bt( !derived()->end( input ) );
      ++it_;
    }

    bool end( ChainView const& input ) const {
      return it_ == input.end();
    }

    // The type that this returns doesn't really matter, it just
    // has to be a unique value for each iteration of the cursor.
    iterator pos() const { return it_; }

    iterator it_;
  };

  template<typename Derived>
  class ReverseCursorBase {
    Derived const* derived() const {
      return static_cast<Derived const*>( this );
    }

  public:
    using riterator = typename ChainView::riterator;

    ReverseCursorBase() = default;

    void rinit( ChainView const& input ) {
      rit_ = input.rbegin();
    }

    decltype( auto ) rget( ChainView const& input ) const {
      assert_bt( !derived()->rend( input ) );
      return *rit_;
    }

    void rnext( ChainView const& input ) {
      assert_bt( !derived()->rend( input ) );
      ++rit_;
    }

    bool rend( ChainView const& input ) const {
      return rit_ == input.rend();
    }

    // The type that this returns doesn't really matter, it
    // just has to be a unique value for each iteration of
    // the cursor.
    riterator rpos() const { return rit_; }

    // If you get an error on this line about riterator being an
    // incomplete type then that means that this view (likely one
    // in your pipelien) does not support reverse iteration.
    riterator rit_;
  };

  /**************************************************************
  ** Chain Maker
  ***************************************************************/
private:
  // Add another element to the chain.
  template<typename NewCursor, typename... Args>
  auto make_chain( Args&&... args ) {
    using NewChainView =
        ChainView<ChainView<InputView, Cursor>, NewCursor>;
    using Data = typename NewCursor::Data;
    return NewChainView( std::move( *this ),
                         Data( std::forward<Args>( args )... ) );
  }

  template<typename T>
  constexpr static auto check_fn_ptr( T&& f ) {
    if constexpr( std::is_function_v<
                      std::remove_reference_t<T>> )
      return &f;
    else
      return std::forward<T>( f );
  }

  template<typename T>
  using func_storage_t =
      decltype( check_fn_ptr( std::declval<T>() ) );

public:
  /**************************************************************
  ** KeepIf
  ***************************************************************/
  template<typename Func>
  auto keep_if( Func&& func ) && {
    struct KeepIfCursor : public CursorBase<KeepIfCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
        Data( func_t&& f )
          : func_( std::move( check_fn_ptr( f ) ) ) {}
        func_storage_t<Func> func_;
      };
      using Base = CursorBase<KeepIfCursor>;
      using Base::it_;
      KeepIfCursor() = default;
      KeepIfCursor( Data const& data ) : func_( &data.func_ ) {}

      void find( ChainView const& input ) {
        do {
          if( this->end( input ) ) break;
          if( ( *func_ )( *it_ ) ) break;
          ++it_;
        } while( true );
      }

      void init( ChainView const& input ) {
        it_ = input.begin();
        find( input );
      }

      void next( ChainView const& input ) {
        assert_bt( !this->end( input ) );
        ++it_;
        find( input );
      }

      func_storage_t<Func> const* func_;
    };
    return make_chain<KeepIfCursor>(
        std::forward<Func>( func ) );
  }

  /**************************************************************
  ** Map
  ***************************************************************/
  template<typename Func>
  struct MapCursor : public CursorBase<MapCursor<Func>> {
    using func_t = std::remove_reference_t<Func>;
    struct Data {
      Data() = default;
      Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
      Data( func_t&& f )
        : func_( std::move( check_fn_ptr( f ) ) ) {}
      func_storage_t<Func> func_;
    };
    using Base = CursorBase<MapCursor>;
    using Base::it_;
    using typename Base::iterator;
    MapCursor() = default;
    MapCursor( Data const& data ) : func_( &data.func_ ) {
      constexpr bool func_returns_ref =
          std::is_reference_v<decltype( ( *func_ )( *it_ ) )>;
      if constexpr( func_returns_ref ) {
        // Normally, if the function returns a reference, we re-
        // quire that the input value is an lvalue reference (to
        // avoid producing a dangling reference). However, there
        // is one exception to that, if the input as a pointer
        // temporary and the function returns a reference to its
        // pointed-to type. That likely means that the function
        // func_ is simply dereferencing a pointer, and in that
        // case it is ok if the pointer itself is an rvalue.
        constexpr bool is_ptr_dereference =
            std::is_pointer_v<std::decay_t<decltype( *it_ )>> &&
            std::is_same_v<
                std::decay_t<decltype( ( *func_ )( *it_ ) )>,
                std::decay_t<std::remove_pointer_t<
                    std::decay_t<decltype( *it_ )>>>>;
        // If you fail here then you are probably trying to re-
        // turn a reference to a temporary (or to something in-
        // side a temporary) that was produced by an earlier view
        // in the pipeline.
        static_assert(
            is_ptr_dereference ||
            std::is_lvalue_reference_v<decltype( *it_ )> );
      }
    }

    decltype( auto ) get( ChainView const& input ) const {
      assert_bt( !this->end( input ) );
      return ( *func_ )( *it_ );
    }

    func_storage_t<Func> const* func_;
  };

  template<typename Func>
  struct BidirectionalMapCursor
    : public MapCursor<Func>,
      public ReverseCursorBase<BidirectionalMapCursor<Func>> {
    using MapCursor<Func>::MapCursor;
    decltype( auto ) rget( ChainView const& input ) const {
      assert_bt( !this->rend( input ) );
      return ( *this->func_ )( *this->rit_ );
    }
  };

  template<typename Func>
  auto map( Func&& func ) && {
    if constexpr( cursor_supports_reverse_v<Cursor> ) {
      return make_chain<BidirectionalMapCursor<Func>>(
          std::forward<Func>( func ) );
    } else {
      return make_chain<MapCursor<Func>>(
          std::forward<Func>( func ) );
    }
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
        Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
        Data( func_t&& f )
          : func_( std::move( check_fn_ptr( f ) ) ) {}
        func_storage_t<Func> func_;
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

      void next( ChainView const& input ) {
        assert_bt( !finished_ );
        assert_bt( it_ != input.end() );
        ++it_;
        if( it_ == input.end() || !( *func_ )( *it_ ) )
          finished_ = true;
      }

      bool end( ChainView const& ) const { return finished_; }

      iterator pos() const { return it_; }

      func_storage_t<Func> const* func_;
      bool                        finished_;
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
        Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
        Data( func_t&& f )
          : func_( std::move( check_fn_ptr( f ) ) ) {}
        func_storage_t<Func> func_;
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

      func_storage_t<Func> const* func_;
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
        Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
        Data( func_t&& f )
          : func_( std::move( check_fn_ptr( f ) ) ) {}
        func_storage_t<Func> func_;
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

      void next( ChainView const& input ) {
        assert_bt( !end( input ) );
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

      iterator pos() const { return it_; }

      func_storage_t<Func> const* func_;
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
      using iterator2 =
          decltype( std::declval<SndView>().begin() );
      ZipCursor() = default;
      ZipCursor( Data const& data )
        : snd_view_( &data.snd_view_ ) {}

      void init( ChainView const& input ) {
        it_  = input.begin();
        it2_ = snd_view_->begin();
      }

      decltype( auto ) get( ChainView const& input ) const {
        assert_bt( !end( input ) );
        return std::pair<decltype( *it_ ), decltype( *it2_ )>{
            *it_, *it2_ };
      }

      void next( ChainView const& input ) {
        assert_bt( !end( input ) );
        ++it_;
        ++it2_;
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() ) ||
               ( it2_ == snd_view_->end() );
      }

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

      void next( ChainView const& input ) {
        assert_bt( !end( input ) );
        ++it_;
        --n_;
        assert_bt( n_ >= 0 );
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() ) || n_ == 0;
      }

      iterator pos() const { return it_; }

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

      int n_ = 0;
    };
    return make_chain<DropCursor>( n );
  }

  /**************************************************************
  ** Intersperse
  ***************************************************************/
  template<typename T = ValueType>
  auto intersperse( T&& val ) && {
    struct IntersperseCursor
      : public CursorBase<IntersperseCursor> {
      struct Data {
        Data( ValueType val ) : val_( std::move( val ) ) {}
        ValueType val_;
      };
      using Base = CursorBase<IntersperseCursor>;
      using Base::it_;
      using typename Base::iterator;
      IntersperseCursor() = default;
      IntersperseCursor( Data const& data )
        : val_( data.val_ ), should_give_val_{ false } {}

      // Note: return type of get() should always be decltype-
      // (auto) unless it always returns by value.
      ValueType get( ChainView const& input ) const {
        assert_bt( !this->end( input ) );
        if( should_give_val_ ) return val_;
        return *it_;
      }

      void next( ChainView const& input ) {
        assert_bt( !this->end( input ) );
        if( should_give_val_ ) {
          should_give_val_ = false;
          return;
        }
        ++it_;
        if( it_ == input.end() ) return;
        should_give_val_ = true;
      }

      ValueType val_;
      bool      should_give_val_;
    };
    return make_chain<IntersperseCursor>(
        std::forward<T>( val ) );
  }

  /**************************************************************
  ** Reverse
  ***************************************************************/
  auto reverse() && {
    struct ReverseCursor : public CursorBase<ReverseCursor>,
                           ReverseCursorBase<ReverseCursor> {
      struct Data {};
      using FBase = CursorBase<ReverseCursor>;
      using RBase = ReverseCursorBase<ReverseCursor>;
      using FBase::it_;
      using RBase::rit_;

      ReverseCursor() = default;
      ReverseCursor( Data const& ) {}

      using iterator  = typename RBase::riterator;
      using riterator = typename FBase::iterator;

      void init( ChainView const& input ) {
        this->rit_ = input.rbegin();
      }

      decltype( auto ) get( ChainView const& input ) const {
        assert_bt( !this->end( input ) );
        return *this->rit_;
      }

      void next( ChainView const& input ) {
        assert_bt( !this->end( input ) );
        ++this->rit_;
      }

      bool end( ChainView const& input ) const {
        return this->rit_ == input.rend();
      }

      iterator pos() const { return this->rit_; }

      // === Reverse ===

      void rinit( ChainView const& input ) {
        it_ = input.begin();
      }

      decltype( auto ) rget( ChainView const& input ) const {
        assert_bt( !this->rend( input ) );
        return *it_;
      }

      void rnext( ChainView const& input ) {
        assert_bt( !this->rend( input ) );
        ++it_;
      }

      bool rend( ChainView const& input ) const {
        return it_ == input.end();
      }

      riterator rpos() const { return it_; }
    };
    return make_chain<ReverseCursor>();
  }

  /**************************************************************
  ** Cache1
  ***************************************************************/
  // Caches the single most recent value so that multiple reads
  // from the iterator won't cause redundant work.
  auto cache1() && {
    struct CacheCursor : public CursorBase<CacheCursor> {
      struct Data {};
      using Base = CursorBase<CacheCursor>;
      using Base::it_;
      using typename Base::iterator;
      CacheCursor() = default;
      CacheCursor( Data const& ) {}

      void load( ChainView const& input ) {
        if( it_ == input.end() ) return;
        cache_ = *it_;
      }

      void init( ChainView const& input ) {
        it_ = input.begin();
        load( input );
      }

      void next( ChainView const& input ) {
        assert_bt( !this->end( input ) );
        ++it_;
        load( input );
      }

      value_type get( ChainView const& input ) const {
        assert_bt( !this->end( input ) );
        return cache_;
      }

      value_type cache_;
    };
    return make_chain<CacheCursor>();
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
        assert_bt( it_ != input.end() );
      }

      void next( ChainView const& input ) {
        ++it_;
        if( it_ == input.end() ) {
          ++cycles_;
          it_ = input.begin();
        }
        assert_bt( it_ != input.end() );
      }

      bool end( ChainView const& ) const { return false; }
      auto pos() const { return std::pair{ cycles_, it_ }; }

      std::intmax_t cycles_ = 0;
    };
    return make_chain<CycleCursor>();
  }

  /**************************************************************
  ** GroupBy
  ***************************************************************/
  template<typename Func>
  auto group_by( Func&& func ) && {
    struct GroupByCursor : public CursorBase<GroupByCursor> {
      using func_t = std::remove_reference_t<Func>;
      struct Data {
        Data() = default;
        Data( func_t const& f ) : func_( check_fn_ptr( f ) ) {}
        Data( func_t&& f )
          : func_( std::move( check_fn_ptr( f ) ) ) {}
        func_storage_t<Func> func_;
      };
      using Base = CursorBase<GroupByCursor>;
      using Base::it_;
      using typename Base::iterator;
      using IncomingValueType = typename ChainView::value_type;
      using ChainGroupView =
          ChainView<ChildView<IncomingValueType, GroupByCursor>,
                    IdentityCursor<ChildView<IncomingValueType,
                                             GroupByCursor>>,
                    IncomingValueType>;
      using ChainGroupViewCursor = IdentityCursor<
          ChildView<IncomingValueType, GroupByCursor>>;
      // using value_type = ChainGroupView;
      GroupByCursor() = default;
      GroupByCursor( Data const& data )
        : func_( &data.func_ ), input_{}, cache_{} {}

      void init( ChainView const& input ) {
        input_ = &input;
        it_    = input.begin();
        if( it_ == input.end() ) return;
        cache_ = *it_;
      }

      decltype( auto ) get( ChainView const& input ) const {
        assert_bt( !end( input ) );
        return ChainGroupView(
            ChildView<IncomingValueType, GroupByCursor>( this ),
            typename ChainGroupViewCursor::Data{} );
      }

      void next( ChainView const& input ) {
        assert_bt( !end( input ) );
        do {
          ++it_;
          if( it_ == input.end() ) return;
        } while( ( *func_ )( cache_, *it_ ) );
        cache_ = *it_;
      }

      bool end( ChainView const& input ) const {
        return ( it_ == input.end() );
      }

      iterator pos() const { return it_; }

      // ======== ChildView hooks ========

      bool child_view_done() const { return finished_group_; }

      auto& child_view_get() const {
        assert_bt( !finished_group_ );
        return *it_;
      }

      void child_view_advance() {
        assert_bt( !finished_group_ );
        ++it_;
        if( it_ == input_->end() || !( *func_ )( cache_, *it_ ) )
          finished_group_ = true;
      }

      func_storage_t<Func> const*    func_;
      ChainView const*               input_;
      bool                           finished_group_ = false;
      typename ChainView::value_type cache_;
    };
    return make_chain<GroupByCursor>(
        std::forward<Func>( func ) );
  }

  /**************************************************************
  ** Combinator Creations
  ***************************************************************/
  // The views below are implemented in terms of the ones above.
  // It should be easier to verify their correctness by in-
  // specting their implementation, but they might be slower than
  // if they have been custom written like the ones above.

  /**************************************************************
  ** Map2Val
  ***************************************************************/
  // Takes a range and a function and returns a range of pairs
  // where the second field of the pair is generated by applying
  // the function to each input value. [a] -> [(a, f(a))].
  template<typename Func>
  auto map2val( Func&& func ) && {
    return std::move( *this ).map( [func = std::forward<Func>(
                                        func )]( auto&& arg ) {
      return std::pair<decltype( arg ), decltype( func( arg ) )>{
          arg, func( arg ) };
    } );
  }

  /**************************************************************
  ** RemoveIf
  ***************************************************************/
  template<typename Func>
  auto remove_if( Func&& func ) && {
    return std::move( *this ).keep_if(
        [func = std::forward<Func>( func )]( auto&& arg ) {
          return !func( arg );
        } );
  }

  /**************************************************************
  ** GroupOn (a.k.a. group-by-key)
  ***************************************************************/
  template<typename Func>
  auto group_on( Func&& func ) && {
    return std::move( *this ).group_by(
        [func = std::forward<Func>(
             func )]<typename L, typename R>( L&& l, R&& r ) {
          return func( std::forward<L>( l ) ) ==
                 func( std::forward<R>( r ) );
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
          return std::pair<decltype( p.second ),
                           decltype( p.first )>{ p.second,
                                                 p.first };
        } );
  }

  /**************************************************************
  ** Dereference
  ***************************************************************/
  auto dereference() && {
    return std::move( *this ).map(
        []<typename T>( T&& arg ) -> decltype( auto ) {
          using deref_t = decltype( *std::forward<T>( arg ) );
          // The reasoning here is that if we dereference some-
          // thing and get an rvalue reference then that probably
          // (?) means that it is an rvalue reference pointing
          // inside a temporary from earlier in the pipeline.
          // However, when `map` computes this temporary by
          // dereferencing the previous iterator, that temporary
          // will disappear before map returns, causing the re-
          // sulting rvalue referenced produced by the derefer-
          // ence operation to be dangling. So for this reason,
          // in that case, we just copy (actually, move) the ob-
          // ject and yield a temporary by value from the `map`.
          // As a concrete example, this was proven to be neces-
          // sary to fix ASan exceptions that were triggered by
          // calling .dereference() on a previous view that was
          // generating temporary maybe<> objects (maybe<> ob-
          // jects, when dereferenced as rvalues, produce rvalue
          // references).
          if constexpr( std::is_rvalue_reference_v<deref_t> )
            return std::remove_reference_t<deref_t>(
                *std::forward<T>( arg ) );
          // Otherwise, it is safe return whatever the derefer-
          // ence operator returns, be it a value, or an lvalue
          // reference.
          else
            return *std::forward<T>( arg );
        } );
  }

  /**************************************************************
  ** CatMaybes
  ***************************************************************/
  auto cat_maybes() && {
    return std::move( *this )
        .remove_if( []( auto&& arg ) { return arg == nothing; } )
        .dereference();
  }

  /**************************************************************
  ** Keys
  ***************************************************************/
  auto keys() && {
    return std::move( *this ).map(
        // Need auto&& return type so that it will deduce a ref-
        // erence for p.first (decltype(auto) will not).
        []( auto&& p ) -> auto&& { return p.first; } );
  }

  /**************************************************************
  ** Tail
  ***************************************************************/
  auto tail() && { return std::move( *this ).drop( 1 ); }

  /**************************************************************
  ** Zip3
  ***************************************************************/
  // FIXME: Make this more efficient, probably needs a dedicated
  // cursor. Also it should ideally yield references into the un-
  // derlying views where appropriate.
  template<typename View2, typename View3>
  auto zip3( View2&& view2, View3&& view3 ) && {
    using value_t_1 = value_type;
    using value_t_2 =
        it_type_to_value_type_t<typename View2::iterator>;
    using value_t_3 =
        it_type_to_value_type_t<typename View3::iterator>;
    using first_pair  = std::pair<value_t_1, value_t_2>;
    using outter_pair = std::pair<first_pair, value_t_3>;
    return std::move( *this )
        .zip( std::forward<View2>( view2 ) )
        .zip( std::forward<View3>( view3 ) )
        .map( []( outter_pair&& p ) {
          return std::tuple{ std::move( p.first.first ),
                             std::move( p.first.second ),
                             std::move( p.second ) };
        } );
  }

  /**************************************************************
  ** Filter
  ***************************************************************/
  // Just for compatibility with range-v3.
  template<typename Func>
  auto filter( Func&& func ) && {
    return std::move( *this ).keep_if(
        std::forward<Func>( func ) );
  }
};

/****************************************************************
** ChainView Makers
*****************************************************************/
// Contrary to the name, this doesn't actually make an AllView,
// since that on its own would not be of much use, since it is
// not chainable. So this will actually wrap the input in an Al-
// lView (which just holds a pointer) and then will wrap the Al-
// lView in a ChainView. However, if the input is already a Chain
// view then it will just forward it.
template<
    typename InputView,
    typename T = it_type_to_value_type_t<
        typename std::remove_reference_t<InputView>::iterator>>
auto all( InputView&& input ) {
  if constexpr( is_chain_view_v<std::decay_t<InputView>> ) {
    return std::forward<InputView>( input );
  } else {
    if constexpr( view_supports_reverse_v<InputView> ) {
      using initial_view_t = BidirectionalAllView<
          std::remove_reference_t<InputView>>;
      using Data = typename BidirectionalIdentityCursor<
          initial_view_t>::Data;
      return ChainView<
          initial_view_t,
          BidirectionalIdentityCursor<initial_view_t>>(
          initial_view_t( &input ), Data{} );
    } else {
      using initial_view_t =
          AllView<std::remove_reference_t<InputView>>;
      using Data = typename IdentityCursor<initial_view_t>::Data;
      return ChainView<initial_view_t,
                       IdentityCursor<initial_view_t>>(
          initial_view_t( &input ), Data{} );
    }
  }
}

// Same above but creates a reverse view.
template<
    typename InputView,
    typename T = it_type_to_value_type_t<
        typename std::remove_reference_t<InputView>::iterator>>
auto rall( InputView&& input ) {
  using initial_view_t =
      ReverseAllView<std::remove_reference_t<InputView>>;
  using Data = typename IdentityCursor<initial_view_t>::Data;
  return ChainView<initial_view_t,
                   IdentityCursor<initial_view_t>>(
      initial_view_t( &input ), Data{} );
}

// This one is kind of like `all` except it doesn't take a con-
// tainer, it only takes the type of a container (which must be
// specified explicitly as the first template parameter). So this
// function is meant to represent/produce a placeholder that says
// "this is the beginning of a chain that takes `InputView` as
// input." This is useful for building view chains for later use
// on multiple containers.
template<
    typename InputView,
    typename T = it_type_to_value_type_t<
        typename std::remove_reference_t<InputView>::iterator>>
auto input() {
  static_assert( !is_chain_view_v<std::decay_t<InputView>> );
  if constexpr( view_supports_reverse_v<InputView> ) {
    using initial_view_t =
        BidirectionalAllView<std::remove_reference_t<InputView>>;
    using Data = typename BidirectionalIdentityCursor<
        initial_view_t>::Data;
    return ChainView<initial_view_t, BidirectionalIdentityCursor<
                                         initial_view_t>>(
        initial_view_t( nullptr ), Data{} );
  } else {
    using initial_view_t =
        AllView<std::remove_reference_t<InputView>>;
    using Data = typename IdentityCursor<initial_view_t>::Data;
    return ChainView<initial_view_t,
                     IdentityCursor<initial_view_t>>(
        initial_view_t( nullptr ), Data{} );
  }
}

/****************************************************************
** ints
*****************************************************************/
inline auto ints( int start = 0,
                  int end   = std::numeric_limits<int>::max() ) {
  using Cursor = IdentityCursor<IntsView>;
  using Data   = typename Cursor::Data;
  return ChainView<IntsView, Cursor>( IntsView( start, end ),
                                      Data{} );
}

/****************************************************************
** generate_n
*****************************************************************/
template<typename Func>
auto generate_n( Func&& func, int count ) {
  using Cursor = IdentityCursor<GenerateView<Func>>;
  using Data   = typename Cursor::Data;
  return ChainView<GenerateView<Func>, Cursor>(
      GenerateView<Func>( std::forward<Func>( func ), count ),
      Data{} );
}

/****************************************************************
** zip
*****************************************************************/
template<typename LeftView, typename RightView>
auto zip( LeftView&& left_view, RightView&& right_view ) {
  return all( std::forward<LeftView>( left_view ) )
      .zip( all( std::forward<RightView>( right_view ) ) );
}

template<typename FirstView, typename SecondView,
         typename ThirdView>
auto zip( FirstView&& first, SecondView&& second,
          ThirdView&& third ) {
  return all( std::forward<FirstView>( first ) )
      .zip3( all( std::forward<SecondView>( second ) ),
             all( std::forward<ThirdView>( third ) ) );
}

} // namespace base::rl