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
#include "maybe.hpp"
#include "unique-coro.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <memory>

namespace rn {

namespace detail {

template<typename T>
class waitable_shared_state {
public:
  waitable_shared_state()  = default;
  ~waitable_shared_state() = default;

  waitable_shared_state( waitable_shared_state&& ) = delete;
  waitable_shared_state& operator=( waitable_shared_state&& ) =
      delete;

  using NotifyFunc = void( T const& );

public:
  // Prefer the move-only unique function to resolve the ambi-
  // guity when a callable is copyable and movable.
  template<typename Func>
  void add_callback( Func&& func ) {
    using func_t = decltype( std::forward<Func>( func ) );
    if constexpr( std::is_move_constructible_v<func_t> &&
                  std::is_rvalue_reference_v<func_t> )
      add_movable_callback( std::forward<Func>( func ) );
    else
      add_copyable_callback( std::forward<Func>( func ) );
  }

  void set_coro( unique_coro coro ) {
    CHECK( !coro_ );
    coro_ = std::move( coro );
  }

public:
  void cancel() {
    coro_.reset();
    ucallbacks_.clear();
    callbacks_.clear();
  }

  bool has_value() const { return maybe_value_.has_value(); }

  T get() const {
    CHECK( has_value() );
    return *maybe_value_;
  }

  template<typename U>
  void set( U&& val ) {
    maybe_value_ = std::forward<U>( val );
  }

  template<typename... Args>
  void set_emplace( Args&&... args ) {
    maybe_value_.emplace( std::forward<Args>( args )... );
  }

private:
  // Accumulates callbacks in a list, then when the value eventu-
  // ally becomes ready, it will call them all in order. Any
  // callbacks added after the value is ready will be called im-
  // mediately.
  void add_copyable_callback(
      std::function<NotifyFunc> callback ) {
    if( has_value() )
      callback( *maybe_value_ );
    else
      callbacks_.push_back( std::move( callback ) );
  }

  void add_movable_callback(
      base::unique_func<NotifyFunc> callback ) {
    if( has_value() )
      callback( *maybe_value_ );
    else
      ucallbacks_.push_back( std::move( callback ) );
  }

public:
  void do_callbacks() {
    CHECK( has_value() );
    for( auto const& callback : callbacks_ )
      callback( *maybe_value_ );
    for( auto& callback : ucallbacks_ )
      callback( *maybe_value_ );
  }

private:
  maybe<T> maybe_value_;
  // Currently we have two separate vectors for unique and
  // non-unique function callbacks. This may cause callbacks to
  // get invoked in a different order than they were inserted.
  // Don't think this should a problem...
  std::vector<std::function<NotifyFunc>>     callbacks_;
  std::vector<base::unique_func<NotifyFunc>> ucallbacks_;
  // Will be populated if this shared state is created by a
  // coroutine.
  maybe<unique_coro> coro_;
};

} // namespace detail

/****************************************************************
** waitable
*****************************************************************/
template<typename T = std::monostate>
class [[nodiscard]] waitable {
  template<typename U>
  using SharedStatePtr =
      std::shared_ptr<detail::waitable_shared_state<U>>;

public:
  using value_type = T;

  waitable( T const& ready_val );

  // This constructor should not be used by client code.
  explicit waitable( SharedStatePtr<T> shared_state )
    : shared_state_{ shared_state } {}

  // We need to cancel in this destructor if we want the corou-
  // tine to be freed as a result (if there is a coroutine asso-
  // ciated with this waitable). This is because there would oth-
  // erwise be a memory cycle: shared_state owns coroutine which
  // owns promise which owns shared_state.
  ~waitable() noexcept { cancel(); }

  waitable( waitable const& ) = delete;
  waitable& operator=( waitable const& ) = delete;
  waitable( waitable&& )                 = default;

  waitable& operator=( waitable&& rhs ) noexcept {
    if( shared_state_ == rhs.shared_state_ ) {
      rhs.shared_state_ = nullptr;
      return *this;
    }
    cancel();
    shared_state_ = std::move( rhs.shared_state_ );
    return *this;
  }

  bool ready() const {
    return shared_state_ && shared_state_->has_value();
  }

  operator bool() const { return ready(); }

  // Gets the value (running any continuations) and returns the
  // value, leaving the waitable in the same state.
  T get() {
    CHECK( ready(),
           "attempt to get value from waitable when not in "
           "`ready` state." );
    return shared_state_->get();
  }

  // We need this so that waitable<T> can access the shared
  // state of waitable<U>. Haven't figured out a way to make
  // them friends yet.
  SharedStatePtr<T>& shared_state() { return shared_state_; }

  void cancel() {
    if( shared_state_ ) shared_state_->cancel();
  }

private:
  SharedStatePtr<T> shared_state_;
};

/****************************************************************
** waitable_promise
*****************************************************************/
template<typename T = std::monostate>
class waitable_promise {
public:
  waitable_promise()
    : shared_state_( new detail::waitable_shared_state<T> ) {
    // shared state ref count should initialize to 1.
  }

  bool operator==( waitable_promise<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( waitable_promise<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool has_value() const { return shared_state_->has_value(); }

  ::rn::waitable<T> waitable() const {
    return ::rn::waitable<T>( shared_state_ );
  }

  std::shared_ptr<detail::waitable_shared_state<T>>&
  shared_state() {
    return shared_state_;
  }

  /**************************************************************
  ** set_value
  ***************************************************************/
  void set_value( T const& value ) const {
    CHECK( !has_value() );
    mutable_state()->set( value );
    mutable_state()->do_callbacks();
  }

  void set_value( T&& value ) const {
    CHECK( !has_value() );
    mutable_state()->set( std::move( value ) );
    mutable_state()->do_callbacks();
  }

  template<typename... Args>
  void set_value_emplace( Args&&... args ) const {
    CHECK( !has_value() );
    mutable_state()->set_emplace(
        std::forward<Args>( args )... );
    mutable_state()->do_callbacks();
  }

  // For convenience.
  void finish() const
      requires( std::is_same_v<T, std::monostate> ) {
    set_value_emplace();
  }

  /**************************************************************
  ** set_value_if_not_set
  ***************************************************************/
  void set_value_if_not_set( T const& value ) const {
    if( has_value() ) return;
    set_value( value );
  }

  void set_value_if_not_set( T&& value ) const {
    if( has_value() ) return;
    set_value( std::move( value ) );
  }

  template<typename... Args>
  void set_value_emplace_if_not_set( Args&&... args ) const {
    if( has_value() ) return;
    set_value_emplace( std::forward<Args>( args )... );
  }

private:
  // This is because it is often that we have to capture promises
  // in lambdas, which this allows us to set the value on the
  // promise without making the lambda mutable, which tends to be
  // viral and is painful to deal with.
  detail::waitable_shared_state<T>* mutable_state() const {
    return const_cast<detail::waitable_shared_state<T>*>(
        shared_state_.get() );
  }

  std::shared_ptr<detail::waitable_shared_state<T>>
      shared_state_;
};

/****************************************************************
** Helpers
*****************************************************************/
// Returns a waitable immediately containing the given value.
template<typename T = std::monostate, typename... Args>
waitable<T> make_waitable( Args&&... args ) {
  waitable_promise<T> s_promise;
  s_promise.set_value_emplace( std::forward<Args>( args )... );
  return s_promise.waitable();
}

template<typename T = std::monostate>
waitable<T> empty_waitable() {
  return waitable_promise<T>{}.waitable();
}

template<typename T>
waitable<T>::waitable( T const& ready_val ) {
  *this = make_waitable<T>( ready_val );
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
    if( !o.ready() )
      res = "<waiting>";
    else if( o.ready() )
      res = fmt::format( "<ready>" );
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
