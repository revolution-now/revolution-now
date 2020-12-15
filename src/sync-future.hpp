/****************************************************************
**sync-future.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-08.
*
* Description: Synchronous promise & future.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <memory>

namespace rn {

namespace internal {

template<typename T>
class sync_shared_state_base {
public:
  sync_shared_state_base()          = default;
  virtual ~sync_shared_state_base() = default;

  virtual bool has_value() const = 0;
  virtual T    get() const       = 0;

  using NotifyFunc = void( T const& );

  // Accumulates callbacks in a list, then when the value eventu-
  // ally becomes ready, it will call them all in order. Any
  // callbacks added after the value is ready will be called im-
  // mediately.
  virtual void add_callback(
      std::function<NotifyFunc> callback ) = 0;
};

} // namespace internal

template<typename T>
class sync_future;

template<typename>
inline constexpr bool is_sync_future_v = false;

template<typename T>
inline constexpr bool is_sync_future_v<sync_future<T>> = true;

/****************************************************************
** sync_future
*****************************************************************/
// Single-threaded "future" object: represents a value that will
// become available in the future by the same thread.
//
// The sync_future has three states:
//
//   waiting --> ready --> empty
//
// It starts off in the `waiting` state upon construction (from a
// sync_promise) then transitions to `ready` when the result be-
// comes available. At this point, the value can be retrieved
// using the get or get_and_reset methods (the latter also causes
// a transitions to the `empty` state). Once in the `empty` state
// the sync_future is "dead" forever.
//
// Example usage:
//
//   sync_promise<int> s_promise;
//   sync_future<int>  s_future1 = s_promise.get_future();
//
//   sync_future<int> s_future2 = s_future1.fmap(
//       []( int n ){ return n+1; } );
//
//   s_promise.set_value( 3 );
//
//   assert( s_future1.get() == 3 );
//   assert( s_future1.get_and_reset() == 3 );
//   assert( s_future2.get_and_reset() == 4 );
//
//   assert( s_future1.empty() );
//   assert( s_future2.empty() );
//
// FIXME: Should be [[nodiscard]] but that has to wait until an
// issue is resolved when compiling with gcc where a function_ref
// that returns a [[nodiscard]] type generates a warning.
template<typename T = std::monostate>
class /*ND*/ sync_future {
  template<typename U>
  using SharedStatePtr =
      std::shared_ptr<internal::sync_shared_state_base<U>>;

public:
  using value_t = T;

  sync_future() {}

  sync_future( T const& ready_val ) {
    *this = make_sync_future<T>( ready_val );
  }

  // This constructor should not be used by client code.
  explicit sync_future( SharedStatePtr<T> shared_state )
    : shared_state_{ shared_state }, taken_{ false } {}

  bool operator==( sync_future<T> const& rhs ) const {
    // Not comparing `taken_`.
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( sync_future<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool empty() const { return shared_state_ == nullptr; }

  bool waiting() const {
    if( empty() ) return false;
    return !shared_state_->has_value();
  }

  bool ready() const {
    if( empty() ) return false;
    return shared_state_->has_value();
  }

  bool taken() const { return taken_; }

  // Gets the value (running any continuations) and returns the
  // value, leaving the sync_future in the same state.
  T get() {
    CHECK( ready(),
           "attempt to get value from sync_future when not in "
           "`ready` state." );
    taken_ = true;
    return shared_state_->get();
  }

  // Gets the value (running any continuations) and resets the
  // sync_future to empty state.
  T get_and_reset() {
    CHECK( ready(),
           "attempt to get value from sync_future when not in "
           "`ready` state." );
    T res = shared_state_->get();
    shared_state_.reset();
    taken_ = true;
    return res;
  }

  template<typename Func>
  auto fmap( Func&& func ) {
    CHECK( !empty(),
           "attempting to attach a continuation to an empty "
           "sync_future." );
    using NewResult_t =
        std::decay_t<std::invoke_result_t<Func, T>>;

    struct sync_shared_state_with_continuation
      : public internal::sync_shared_state_base<NewResult_t> {
      ~sync_shared_state_with_continuation() override = default;

      sync_shared_state_with_continuation(
          SharedStatePtr<T> old_shared_state,
          Func&&            continuation )
        : old_shared_state_( old_shared_state ),
          continuation_( std::forward<Func>( continuation ) ) {}

      bool has_value() const override {
        return old_shared_state_->has_value();
      }

      NewResult_t get() const override {
        CHECK( has_value() );
        return continuation_( old_shared_state_->get() );
      }

      void add_callback(
          std::function<
              typename internal::sync_shared_state_base<
                  NewResult_t>::NotifyFunc>
              callback ) override {
        old_shared_state_->add_callback(
            [this,
             callback = std::move( callback )]( T const& val ) {
              callback( this->continuation_( val ) );
            } );
      }

      SharedStatePtr<T> old_shared_state_;
      // continuation :: T -> NewResult_t
      Func continuation_;
    };

    return sync_future<NewResult_t>(
        std::make_shared<sync_shared_state_with_continuation>(
            shared_state_, std::forward<Func>( func ) ) );
  }

  template<typename Func>
  sync_future<> consume( Func&& func ) {
    return fmap( [func = std::forward<Func>( func )](
                     auto const& value ) {
      func( value );
      return std::monostate{};
    } );
  }

  // Returns a future object whose result is a monostate. When
  // the monostate is retrieved with get_and_reset then a side
  // effect will be performed, namely to store the result into
  // the location given by the destination pointer.
  auto stored( T* destination ) {
    CHECK( !empty(),
           "attempting to attach a continuation to an empty "
           "sync_future." );
    return consume( [destination]( T const& value ) {
      *destination = value;
    } );
  }

  template<typename Func>
  auto bind( Func&& func ) -> std::invoke_result_t<Func, T> {
    using new_future_t = std::invoke_result_t<Func, T>;
    static_assert( is_sync_future_v<new_future_t>,
                   "The function passed to `bind` must return a "
                   "sync_future." );
    CHECK( !empty(),
           "attempting to bind to an empty sync_future." );
    using new_value_t = typename new_future_t::value_t;

    struct sync_shared_state_with_monadic_continuation
      : public internal::sync_shared_state_base<new_value_t> {
      using Base_t =
          internal::sync_shared_state_base<new_value_t>;
      ~sync_shared_state_with_monadic_continuation() override =
          default;

      sync_shared_state_with_monadic_continuation(
          SharedStatePtr<T> old_shared_state,
          Func&&            continuation )
        : old_shared_state_( old_shared_state ),
          new_shared_state_(),
          continuation_( std::forward<Func>( continuation ) ) {
        old_shared_state_->add_callback( [this]( T const& val ) {
          this->new_shared_state_ =
              this->continuation_( val ).shared_state();
          for( auto& f : this->callbacks_ )
            this->new_shared_state_->add_callback(
                std::move( f ) );
          this->callbacks_.clear();
        } );
      }

      bool has_value() const override {
        if( new_shared_state_ == nullptr ) return false;
        return new_shared_state_->has_value();
      }

      new_value_t get() const override {
        CHECK( has_value() );
        return new_shared_state_->get();
      }

      using typename Base_t::NotifyFunc;

      void add_callback(
          std::function<NotifyFunc> callback ) override {
        if( new_shared_state_ != nullptr ) {
          new_shared_state_->add_callback(
              std::move( callback ) );
        } else {
          callbacks_.push_back( std::move( callback ) );
        }
      }

      SharedStatePtr<T>           old_shared_state_;
      SharedStatePtr<new_value_t> new_shared_state_;
      // This is to accumulate callbacks that are installed be-
      // fore the continuation is called. Once the continuation
      // is called then we are ready to properly install them.
      Vec<std::function<NotifyFunc>> callbacks_;
      // *continuation_ :: T -> sync_future<new_value_t>
      Func continuation_;
    };

    return sync_future<new_value_t>(
        std::make_shared<
            sync_shared_state_with_monadic_continuation>(
            shared_state_, std::forward<Func>( func ) ) );
  }

  // func takes no arguments.
  template<typename Func>
  auto next( Func&& func ) -> std::invoke_result_t<Func> {
    return bind( [func = std::forward<Func>( func )](
                     auto const& ) { return func(); } );
  }

  // Need to use >> instead of >>= because >>= is right associa-
  // tive.
  template<typename Func>
  auto operator>>( Func&& func ) {
    return bind( std::forward<Func>( func ) );
  }

  // We need this so that sync_future<T> can access the shared
  // state of sync_future<U>. Haven't figured out a way to make
  // them friends yet.
  SharedStatePtr<T> shared_state() { return shared_state_; }

private:
  SharedStatePtr<T> shared_state_;
  bool              taken_ = false;
};

/****************************************************************
** sync_promise
*****************************************************************/
// Single-threaded "promise" object: allows creating futures and
// sending values to them in the same thread.
//
// Example usage:
//
//   sync_promise<int> s_promise;
//
//   sync_future<int> s_future = s_promise.get_future();
//   assert( s_future.waiting() );
//
//   s_promise.set_value( 3 );
//
//   assert( s_future.get() == 3 );
//
//   assert( s_future.get_and_reset() == 3 );
//   assert( s_future.empty() );
//
template<typename T = std::monostate>
class sync_promise {
  struct sync_shared_state
    : public internal::sync_shared_state_base<T> {
    using Base_t = internal::sync_shared_state_base<T>;
    ~sync_shared_state() override = default;

    sync_shared_state() = default;

    bool has_value() const override {
      return maybe_value.has_value();
    }

    T get() const override {
      CHECK( has_value() );
      return *maybe_value;
    }

    using typename Base_t::NotifyFunc;

    void add_callback(
        std::function<NotifyFunc> callback ) override {
      if( has_value() )
        callback( *maybe_value );
      else
        callbacks_.push_back( std::move( callback ) );
    }

    void do_callbacks() const {
      CHECK( has_value() );
      for( auto const& callback : callbacks_ )
        callback( *maybe_value );
    }

    maybe<T>                                 maybe_value;
    std::vector<std::function<NotifyFunc>> callbacks_;
  };

public:
  sync_promise() : shared_state_( new sync_shared_state ) {}

  bool operator==( sync_promise<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( sync_promise<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool has_value() const { return shared_state_->has_value(); }

  void set_value( T const& value ) {
    CHECK( !has_value() );
    shared_state_->maybe_value = value;
    shared_state_->do_callbacks();
  }

  void set_value( T&& value ) {
    CHECK( !has_value() );
    shared_state_->maybe_value = std::move( value );
    shared_state_->do_callbacks();
  }

  template<typename... Args>
  void set_value_emplace( Args&&... args ) {
    CHECK( !has_value() );
    shared_state_->maybe_value.emplace(
        std::forward<Args>( args )... );
    shared_state_->do_callbacks();
  }

  sync_future<T> get_future() const {
    return sync_future<T>( shared_state_ );
  }

private:
  std::shared_ptr<sync_shared_state> shared_state_;
};

/****************************************************************
** Helpers
*****************************************************************/
template<typename Fsm>
void advance_fsm_ui_state( Fsm* fsm, sync_future<>* s_future ) {
  CHECK( !s_future->empty() );
  if( s_future->ready() ) {
    fsm->pop();
    s_future->get_and_reset();
  }
}

// Returns a sync_future immediately containing the given value.
template<typename T = std::monostate, typename... Args>
sync_future<T> make_sync_future( Args&&... args ) {
  sync_promise<T> s_promise;
  s_promise.set_value_emplace( std::forward<Args>( args )... );
  return s_promise.get_future();
}

// Returns `false` if the caller needs to wait for completion of
// the step, true if the step is complete.
template<typename T = std::monostate>
bool step_with_future(
    sync_future<T>*                s_future,
    function_ref<sync_future<T>()> init,
    function_ref<bool( T const& )> when_ready ) {
  if( s_future->empty() ) {
    *s_future = init();
    // !! should fall through.
  }
  if( !s_future->ready() ) return false;
  if( !s_future->taken() ) return when_ready( s_future->get() );
  return true;
}

// Same as above, but the `when_ready` function always returns
// true.
template<typename T = std::monostate>
bool step_with_future( sync_future<T>*                s_future,
                       function_ref<sync_future<T>()> init ) {
  if( s_future->empty() ) {
    *s_future = init();
    // !! should fall through.
  }
  if( !s_future->ready() ) return false;
  if( !s_future->taken() ) {
    // Still need to run this in case it has side effects.
    (void)s_future->get();
    return true;
  }
  return true;
}

} // namespace rn

/****************************************************************
** {fmt}
*****************************************************************/
namespace fmt {
// {fmt} formatters.

template<typename T>
struct formatter<::rn::sync_future<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::sync_future<T> const& o,
               FormatContext&              ctx ) {
    std::string res;
    if( o.empty() )
      res = "<empty>";
    else if( o.waiting() )
      res = "<waiting>";
    else if( o.ready() && !o.taken() )
      res = fmt::format( "<ready>" );
    else if( o.taken() )
      res = fmt::format( "<taken>" );
    return formatter_base::format( res, ctx );
  }
};

template<typename T>
struct formatter<::rn::sync_promise<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::sync_promise<T> const& o,
               FormatContext&               ctx ) {
    std::string res;
    if( !o.has_value() )
      res = "<empty>";
    else
      res = fmt::format( "<ready>" );
    return formatter_base::format( res, ctx );
  }
};

} // namespace fmt
