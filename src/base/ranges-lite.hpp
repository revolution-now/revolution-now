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
//      rv::take_while_incl
//
//      3 rv::zip
//      3 rv::take
//      3 rv::drop
//      3 rv::ints
//      3 rv::cycle
//
//      2 rv::sliding
//      2 rv::intersperse
//      2 rv::enumerate
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

/****************************************************************
** Macros
*****************************************************************/
#define RL_LAMBDA( name, ... ) name( L( __VA_ARGS__ ) )

#define rl_keep( ... ) RL_LAMBDA( keep, __VA_ARGS__ )
#define rl_remove( ... ) RL_LAMBDA( remove, __VA_ARGS__ )
#define rl_map( ... ) RL_LAMBDA( map, __VA_ARGS__ )
#define rl_take_while( ... ) RL_LAMBDA( take_while, __VA_ARGS__ )

/****************************************************************
** View
*****************************************************************/
template<typename InputView, typename Cursor>
class View {
  InputView input_;
  Cursor    op_;

public:
  View( InputView&& input, Cursor&& op )
    : input_( std::move( input ) ), op_( std::move( op ) ) {}

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
      view_->op_.init( view_->input_ );
      clear_if_end();
    }

    void clear_if_end() {
      if( view_->op_.end( view_->input_ ) ) view_ = nullptr;
    }

    auto const& operator*() const {
      rl_assert( view_ != nullptr );
      return view_->op_.get( view_->input_ );
    }

    Iterator& operator++() {
      rl_assert( view_ != nullptr );
      view_->op_.next( view_->input_ );
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
        return view_->op_.pos( view_->input_ ) ==
               rhs.view_->op_.pos( rhs.view_->input_ );
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
  struct CursorBase {
    using iterator   = typename View::iterator;
    using value_type = typename View::value_type;

    CursorBase() = default;

    void init( View& input ) { it_ = input.begin(); }

    value_type const& get( View const& ) const { return *it_; }

    void next( View const& input ) {
      rl_assert( !end( input ) );
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
    struct KeepCursor : public CursorBase {
      using CursorBase::it_;
#ifdef __clang__ // C++20
      KeepCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      KeepCursor( Func&& f ) : func_( std::move( f ) ) {}

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

      using CursorBase::end;
      using CursorBase::get;
      using CursorBase::pos;

      Func func_;
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
  ** Map
  ***************************************************************/
  template<typename Func>
  auto map( Func&& func ) && {
    struct MapCursor : public CursorBase {
      using CursorBase::it_;
      using typename CursorBase::iterator;
      using value_type =
          std::invoke_result_t<Func,
                               typename iterator::value_type>;
#ifdef __clang__ // C++20
      MapCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      MapCursor( Func&& f ) : func_( std::move( f ) ) {}

      void init( View& input ) {
        it_ = input.begin();
        if( !end( input ) ) cache = func_( *it_ );
      }

      value_type const& get( View const& ) const {
        return cache;
      }

      void next( View const& input ) {
        rl_assert( !end( input ) );
        ++it_;
        if( !end( input ) ) cache = func_( *it_ );
      }

      using CursorBase::end;
      using CursorBase::pos;

      Func       func_;
      value_type cache;
    };
    return make_chain<MapCursor>( std::forward<Func>( func ) );
  }

  /**************************************************************
  ** TakeWhile
  ***************************************************************/
  template<typename Func>
  auto take_while( Func&& func ) && {
    struct TakeWhileCursor : public CursorBase {
      using CursorBase::it_;
      using typename CursorBase::iterator;
#ifdef __clang__ // C++20
      TakeWhileCursor() requires(
          std::is_default_constructible_v<Func> ) = default;
#endif
      TakeWhileCursor( Func&& f )
        : func_( std::move( f ) ), finished_{ false } {}

      void init( View& input ) {
        it_ = input.begin();
        if( it_ == input.end() || !func_( *it_ ) )
          finished_ = true;
      }

      using CursorBase::get;

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

      Func func_;
      bool finished_;
    };
    return make_chain<TakeWhileCursor>(
        std::forward<Func>( func ) );
  }
};

/****************************************************************
** Free-standing view factories.
*****************************************************************/
template<typename InputView,
         typename T = typename InputView::value_type>
auto view( InputView const& input ) {
  using initial_view_t = std::span<T const>;
  struct DoNothingCursor {
    using iterator   = typename initial_view_t::iterator;
    using value_type = typename iterator::value_type;
    void init( initial_view_t& input ) { it = input.begin(); }
    value_type const& get( initial_view_t const& ) const {
      return *it;
    }
    void next( initial_view_t const& ) { ++it; }
    bool end( initial_view_t const& input ) const {
      return it == input.end();
    }
    iterator pos( initial_view_t const& ) const { return it; }
    iterator it;
  };
  return View( initial_view_t( input ), DoNothingCursor{} );
}

// Reverse view.
template<typename InputView,
         typename T = typename InputView::value_type>
auto rview( InputView const& input ) {
  using initial_view_t = std::span<T const>;
  struct ReverseCursor {
    using iterator   = typename initial_view_t::reverse_iterator;
    using value_type = typename iterator::value_type;
    void init( initial_view_t& input ) { it = input.rbegin(); }
    value_type const& get( initial_view_t const& ) const {
      return *it;
    }
    void next( initial_view_t const& ) { ++it; }
    bool end( initial_view_t const& input ) const {
      return it == input.rend();
    }
    iterator pos( initial_view_t const& ) const { return it; }
    iterator it;
  };
  return View( initial_view_t( input ), ReverseCursor{} );
}

/****************************************************************
** Views from Temporaries
*****************************************************************/
template<typename InputView>
auto view( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    void> = delete;

template<typename InputView>
auto view_temporary( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    decltype( view( input ) )> {
  return view( input );
}

template<typename InputView>
auto rview( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    void> = delete;

template<typename InputView>
auto rview_temporary( InputView&& input ) -> std::enable_if_t<
    std::is_rvalue_reference_v<
        decltype( std::forward<InputView>( input ) )>,
    decltype( rview( input ) )> {
  return rview( input );
}

} // namespace base::rl