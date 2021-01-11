/****************************************************************
**waitable.hpp
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
#include "error.hpp"
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
class waitable;

template<typename>
inline constexpr bool is_waitable_v = false;

template<typename T>
inline constexpr bool is_waitable_v<waitable<T>> = true;

/****************************************************************
** waitable
*****************************************************************/
// Single-threaded "future" object: represents a value that will
// become available in the future by the same thread.
//
// The waitable has three states:
//
//   waiting --> ready --> empty
//
// It starts off in the `waiting` state upon construction (from a
// waitable_promise) then transitions to `ready` when the result
// becomes available. At this point, the value can be retrieved
// using the get or get_and_reset methods (the latter also causes
// a transitions to the `empty` state). Once in the `empty` state
// the waitable is "dead" forever.
//
// Example usage:
//
//   waitable_promise<int> s_promise;
//   waitable<int>  s_future1 = s_promise.get_waitable();
//
//   waitable<int> s_future2 = s_future1.fmap(
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
template<typename T = std::monostate>
class [[nodiscard]] waitable {
  template<typename U>
  using SharedStatePtr =
      std::shared_ptr<internal::sync_shared_state_base<U>>;

public:
  using value_t = T;

  waitable() {}

  waitable( T const& ready_val ) {
    *this = make_waitable<T>( ready_val );
  }

  // This constructor should not be used by client code.
  explicit waitable( SharedStatePtr<T> shared_state )
    : shared_state_{ shared_state }, taken_{ false } {}

  bool operator==( waitable<T> const& rhs ) const {
    // Not comparing `taken_`.
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( waitable<T> const& rhs ) const {
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
  // value, leaving the waitable in the same state.
  T get() {
    CHECK( ready(),
           "attempt to get value from waitable when not in "
           "`ready` state." );
    taken_ = true;
    return shared_state_->get();
  }

  // Gets the value (running any continuations) and resets the
  // waitable to empty state.
  T get_and_reset() {
    CHECK( ready(),
           "attempt to get value from waitable when not in "
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
           "waitable." );
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

    return waitable<NewResult_t>(
        std::make_shared<sync_shared_state_with_continuation>(
            shared_state_, std::forward<Func>( func ) ) );
  }

  template<typename Func>
  waitable<> consume( Func&& func ) {
    return fmap( [func = std::forward<Func>( func )](
                     auto const& value ) {
      func( value );
      return std::monostate{};
    } );
  }

  // We need this so that waitable<T> can access the shared
  // state of waitable<U>. Haven't figured out a way to make
  // them friends yet.
  SharedStatePtr<T> shared_state() { return shared_state_; }

private:
  SharedStatePtr<T> shared_state_;
  bool              taken_ = false;
};

/****************************************************************
** waitable_promise
*****************************************************************/
// Single-threaded "promise" object: allows creating futures and
// sending values to them in the same thread.
//
// Example usage:
//
//   waitable_promise<int> s_promise;
//
//   waitable<int> s_future = s_promise.get_waitable();
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
class waitable_promise {
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

    maybe<T>                               maybe_value;
    std::vector<std::function<NotifyFunc>> callbacks_;
  };

public:
  waitable_promise() : shared_state_( new sync_shared_state ) {}

  bool operator==( waitable_promise<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( waitable_promise<T> const& rhs ) const {
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

  waitable<T> get_waitable() const {
    return waitable<T>( shared_state_ );
  }

private:
  std::shared_ptr<sync_shared_state> shared_state_;
};

/****************************************************************
** Helpers
*****************************************************************/
template<typename Fsm>
void advance_fsm_ui_state( Fsm* fsm, waitable<>* s_future ) {
  CHECK( !s_future->empty() );
  if( s_future->ready() ) {
    fsm->pop();
    s_future->get_and_reset();
  }
}

// Returns a waitable immediately containing the given value.
template<typename T = std::monostate, typename... Args>
waitable<T> make_waitable( Args&&... args ) {
  waitable_promise<T> s_promise;
  s_promise.set_value_emplace( std::forward<Args>( args )... );
  return s_promise.get_waitable();
}

// Returns `false` if the caller needs to wait for completion of
// the step, true if the step is complete.
template<typename T = std::monostate>
bool step_with_future(
    waitable<T>* s_future, function_ref<waitable<T>()> init,
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
bool step_with_future( waitable<T>*                s_future,
                       function_ref<waitable<T>()> init ) {
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
struct formatter<::rn::waitable<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::waitable<T> const& o, FormatContext& ctx ) {
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
struct formatter<::rn::waitable_promise<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::waitable_promise<T> const& o,
               FormatContext&                   ctx ) {
    std::string res;
    if( !o.has_value() )
      res = "<empty>";
    else
      res = fmt::format( "<ready>" );
    return formatter_base::format( res, ctx );
  }
};

} // namespace fmt
