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
#  define rl_assert( ... )
#endif

template<typename InputView, typename Op>
class View {
  InputView input_;
  Op        op_;

public:
  View( InputView&& input, Op&& op )
    : input_( std::move( input ) ), op_( std::move( op ) ) {}

  using value_type =
      std::decay_t<decltype( std::declval<Op>().get(
          std::declval<InputView>() ) )>;

  /**************************************************************
  ** Iterator
  ***************************************************************/
  struct Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = value_type;
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

  /**************************************************************
  ** Materialize
  ***************************************************************/
  std::vector<value_type> materialize() {
    // FIXME: pre-size
    return std::vector<value_type>( this->begin(), this->end() );
  }

  /**************************************************************
  ** Keep
  ***************************************************************/
  template<typename Func>
  auto keep( Func&& func ) {
    struct KeepOp {
      using input_view_t = View;
      using iterator     = typename input_view_t::iterator;
      using value_type   = typename input_view_t::value_type;
      KeepOp( Func&& f ) : func_( std::move( f ) ) {}
      void init( input_view_t& input ) { it = input.begin(); }
      value_type const& get( input_view_t const& input ) const {
        return *it;
      }
      void next( input_view_t const& input ) {
        rl_assert( !end( input ) );
        do {
          ++it;
          if( end( input ) ) break;
          if( func_( *it ) ) break;
        } while( true );
      }
      bool end( input_view_t const& input ) const {
        return it == input.end();
      }
      iterator pos( input_view_t const& input ) const {
        return it;
      }
      iterator it;
      Func     func_;
    };
    using NewView = View<View<InputView, Op>, KeepOp>;
    return NewView( std::move( *this ),
                    KeepOp( std::move( func ) ) );
  }

  /**************************************************************
  ** Map
  ***************************************************************/
  template<typename Func>
  auto map( Func&& func ) {
    struct MapOp {
      using input_view_t = View;
      using iterator     = typename input_view_t::iterator;
      using value_type   = typename input_view_t::value_type;
      MapOp( Func&& f ) : func_( std::move( f ) ) {}
      void init( input_view_t& input ) {
        it = input.begin();
        if( !end( input ) ) cache = func_( *it );
      }
      value_type const& get( input_view_t const& input ) const {
        return cache;
      }
      void next( input_view_t const& input ) {
        rl_assert( !end( input ) );
        ++it;
        if( !end( input ) ) cache = func_( *it );
      }
      bool end( input_view_t const& input ) const {
        return it == input.end();
      }
      iterator pos( input_view_t const& input ) const {
        return it;
      }
      iterator   it;
      Func       func_;
      value_type cache;
    };
    using NewView = View<View<InputView, Op>, MapOp>;
    return NewView( std::move( *this ),
                    MapOp( std::move( func ) ) );
  }

  /**************************************************************
  ** TakeWhile
  ***************************************************************/
  template<typename Func>
  auto take_while( Func&& func ) {
    struct TakeWhileOp {
      using input_view_t = View;
      using iterator     = typename input_view_t::iterator;
      using value_type   = typename input_view_t::value_type;
      TakeWhileOp( Func&& f )
        : func_( std::move( f ) ), finished_{ false } {}
      void init( input_view_t& input ) {
        it = input.begin();
        if( it == input.end() || !func_( *it ) )
          finished_ = true;
      }
      value_type const& get( input_view_t const& input ) const {
        rl_assert( !finished_ );
        return *it;
      }
      void next( input_view_t const& input ) {
        rl_assert( !finished_ );
        rl_assert( it != input.end() );
        ++it;
        if( it == input.end() || !func_( *it ) )
          finished_ = true;
      }
      bool end( input_view_t const& input ) const {
        return finished_;
      }
      iterator pos( input_view_t const& input ) const {
        if( finished_ ) return input.end();
        return it;
      }
      iterator it;
      Func     func_;
      bool     finished_;
    };
    using NewView = View<View<InputView, Op>, TakeWhileOp>;
    return NewView( std::move( *this ),
                    TakeWhileOp( std::move( func ) ) );
  }
};

// template<typename InputView>
// auto view( InputView&& ) = delete;

template<typename InputView,
         typename T = typename InputView::value_type>
auto view( InputView const& input ) {
  using initial_view_t = std::span<T const>;
  struct DoNothingOp {
    using iterator   = typename initial_view_t::iterator;
    using value_type = typename iterator::value_type;
    void init( initial_view_t& input ) { it = input.begin(); }
    value_type const& get( initial_view_t const& input ) const {
      return *it;
    }
    void next( initial_view_t const& ) { ++it; }
    bool end( initial_view_t const& input ) const {
      return it == input.end();
    }
    iterator pos( initial_view_t const& input ) const {
      return it;
    }
    iterator it;
  };
  return View( initial_view_t( input ), DoNothingOp{} );
}

// Reverse view.
template<typename InputView,
         typename T = typename InputView::value_type>
auto rview( InputView const& input ) {
  using initial_view_t = std::span<T const>;
  struct ReverseOp {
    using iterator   = typename initial_view_t::reverse_iterator;
    using value_type = typename iterator::value_type;
    void init( initial_view_t& input ) { it = input.rbegin(); }
    value_type const& get( initial_view_t const& input ) const {
      return *it;
    }
    void next( initial_view_t const& ) { ++it; }
    bool end( initial_view_t const& input ) const {
      return it == input.rend();
    }
    iterator pos( initial_view_t const& input ) const {
      return it;
    }
    iterator it;
  };
  return View( initial_view_t( input ), ReverseOp{} );
}

} // namespace base::rl