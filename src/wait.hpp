/****************************************************************
**wait.hpp
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
#include "maybe.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/unique-coro.hpp"
#include "base/unique-func.hpp"

// C++ standard library
#include <exception>
#include <memory>

namespace rn {

template<typename T = std::monostate>
class wait_promise;

namespace detail {

template<typename T>
struct promise_type;

template<typename T>
class wait_shared_state {
 public:
  wait_shared_state()  = default;
  ~wait_shared_state() = default;

  wait_shared_state( wait_shared_state const& ) = delete;
  wait_shared_state& operator=( wait_shared_state const& ) =
      delete;
  // Note that this object is in general self-referential, there-
  // fore it cannot be moved.
  wait_shared_state( wait_shared_state&& )            = delete;
  wait_shared_state& operator=( wait_shared_state&& ) = delete;

  using NotifyFunc = void( T const& );
  using ExceptFunc = void( std::exception_ptr );

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

  template<typename Func>
  void set_exception_callback( Func&& func ) {
    using func_t = decltype( std::forward<Func>( func ) );
    if constexpr( std::is_move_constructible_v<func_t> &&
                  std::is_rvalue_reference_v<func_t> )
      set_movable_exception_callback(
          std::forward<Func>( func ) );
    else
      set_copyable_exception_callback(
          std::forward<Func>( func ) );
  }

  void set_coro( base::unique_coro<promise_type<T>> coro ) {
    CHECK( !coro_ );
    coro_ = std::move( coro );
  }

  void cancel() {
    eptr_ = {};
    coro_.reset();
    ucallbacks_.clear();
    callbacks_.clear();
    exception_callback_.reset();
    exception_ucallback_.reset();
  }

  // So that we can access private members of this class when it
  // is templated on types other than our T.
  template<typename U>
  friend class wait_shared_state;

  bool has_value() const { return maybe_value_.has_value(); }
  std::exception_ptr exception() const { return eptr_; }

  bool has_exception() const { return bool( exception() ); }

  T const& get() const noexcept {
    CHECK( has_value() );
    return *maybe_value_;
  }

  T& get() noexcept {
    CHECK( has_value() );
    return *maybe_value_;
  }

  template<typename U>
  void set( U&& val ) {
    CHECK( !eptr_ );
    maybe_value_ = std::forward<U>( val );
  }

  template<typename... Args>
  void set_emplace( Args&&... args ) {
    CHECK( !eptr_ );
    maybe_value_.emplace( std::forward<Args>( args )... );
  }

  void set_exception( std::exception_ptr eptr ) {
    // Keep the first exception encountered.
    if( eptr_ ) return;
    eptr_ = eptr;
    do_exception_callback();
    // No need to call cancel() here. An exception should propa-
    // gate outword and cause the coroutine call stack to unwind,
    // and in doing so the wait's destructors will be called
    // and things will be cancelled naturally.
  }

  void do_callbacks() {
    CHECK( has_value() );
    CHECK( !eptr_ );
    for( auto const& callback : callbacks_ )
      callback( *maybe_value_ );
    for( auto& callback : ucallbacks_ )
      callback( *maybe_value_ );
  }

  void do_exception_callback() {
    CHECK( eptr_ );
    if( exception_callback_ ) ( *exception_callback_ )( eptr_ );
    if( exception_ucallback_ )
      ( *exception_ucallback_ )( eptr_ );
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

  void set_copyable_exception_callback(
      std::function<ExceptFunc> callback ) {
    if( exception() )
      callback( exception() );
    else
      exception_callback_ = std::move( callback );
  }

  void set_movable_exception_callback(
      base::unique_func<ExceptFunc> callback ) {
    if( exception() )
      callback( exception() );
    else
      exception_ucallback_ = std::move( callback );
  }

  maybe<T>           maybe_value_;
  std::exception_ptr eptr_ = {}; // this is nullable.

  // Currently we have two separate vectors for unique and
  // non-unique function callbacks. This may cause callbacks to
  // get invoked in a different order than they were inserted.
  // Don't think this should a problem...
  std::vector<std::function<NotifyFunc>>     callbacks_;
  std::vector<base::unique_func<NotifyFunc>> ucallbacks_;

  maybe<std::function<ExceptFunc>>     exception_callback_;
  maybe<base::unique_func<ExceptFunc>> exception_ucallback_;

  // Will be populated if this shared state is created by a
  // coroutine.
  maybe<base::unique_coro<promise_type<T>>> coro_;
};

} // namespace detail

/****************************************************************
** wait
*****************************************************************/
template<typename T = std::monostate>
class [[nodiscard]] wait {
  template<typename U>
  using SharedStatePtr =
      std::shared_ptr<detail::wait_shared_state<U>>;

 public:
  using value_type = T;

  wait( T const& ready_val );

  // This constructor should not be used by client code.
  explicit wait( SharedStatePtr<T> shared_state )
    : shared_state_{ shared_state } {}

  // We need to cancel in this destructor if we want the corou-
  // tine to be freed as a result (if there is a coroutine asso-
  // ciated with this wait). This is because there would oth-
  // erwise be a memory cycle: shared_state owns coroutine which
  // owns promise which owns shared_state.
  ~wait() noexcept { cancel(); }

  wait( wait const& )            = delete;
  wait& operator=( wait const& ) = delete;
  wait( wait&& )                 = default;

  wait& operator=( wait&& rhs ) noexcept {
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

  std::exception_ptr exception() const {
    return shared_state_->exception();
  }

  bool has_exception() const {
    CHECK( shared_state_ != nullptr );
    return shared_state_->has_exception();
  }

  operator bool() const { return ready(); }

  T& get() noexcept { return shared_state_->get(); }

  T const& get() const noexcept { return shared_state_->get(); }

  T& operator*() noexcept { return get(); }

  T const& operator*() const noexcept { return get(); }

  T*       operator->() noexcept { return &get(); }
  T const* operator->() const noexcept { return &get(); }

  SharedStatePtr<T>& shared_state() { return shared_state_; }

  void cancel() {
    if( shared_state_ ) shared_state_->cancel();
  }

  friend void to_str( wait const& o, std::string& out,
                      base::ADL_t ) {
    out += o.ready() ? "<ready>" : "<waiting>";
  }

 private:
  SharedStatePtr<T> shared_state_;
};

/****************************************************************
** wait_promise
*****************************************************************/
template<typename T>
class wait_promise {
 public:
  // This type is immobile in order to enforce that its lifetime
  // be tied to a lexical scope in order to enforce a structured
  // concurrency model where all promises are owned by a running
  // coroutine that is owned by a waitable whose lifetime exceeds
  // that of the promise. This allows us to avoid using shared
  // pointers to represent the shared state.
  wait_promise( wait_promise const& )            = delete;
  wait_promise& operator=( wait_promise const& ) = delete;
  wait_promise( wait_promise&& )                 = delete;
  wait_promise& operator=( wait_promise&& )      = delete;

  wait_promise() { reset(); }

  void reset() {
    shared_state_.reset( new detail::wait_shared_state<T> );
  }

  bool operator==( wait_promise<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( wait_promise<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool has_value() const { return shared_state_->has_value(); }

  ::rn::wait<T> wait() const {
    return ::rn::wait<T>( shared_state_ );
  }

  std::shared_ptr<detail::wait_shared_state<T>>& shared_state() {
    return shared_state_;
  }

  /**************************************************************
  ** set_exception
  ***************************************************************/
  void set_exception( std::exception_ptr eptr ) const {
    mutable_state()->set_exception( eptr );
  }

  template<typename Exception>
  void set_exception( Exception&& e ) const {
    mutable_state()->set_exception( std::make_exception_ptr(
        std::forward<Exception>( e ) ) );
  }

  template<typename Exception, typename... Args>
  void set_exception_emplace( Args&&... args ) const {
    mutable_state()->set_exception( std::make_exception_ptr(
        Exception( std::forward<Args>( args )... ) ) );
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
  requires( std::is_same_v<T, std::monostate> )
  {
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

  friend void to_str( wait_promise const& o, std::string& out,
                      base::ADL_t ) {
    out += o.has_value() ? "<ready>" : "<empty>";
  }

 private:
  // This is because it is often that we have to capture promises
  // in lambdas, which this allows us to set the value on the
  // promise without making the lambda mutable, which tends to be
  // viral and is painful to deal with.
  detail::wait_shared_state<T>* mutable_state() const {
    return const_cast<detail::wait_shared_state<T>*>(
        shared_state_.get() );
  }

  std::shared_ptr<detail::wait_shared_state<T>> shared_state_;
};

/****************************************************************
** Helpers
*****************************************************************/
// Returns a wait immediately containing the given value.
template<typename T = std::monostate, typename... Args>
wait<T> make_wait( Args&&... args ) {
  wait_promise<T> s_promise;
  s_promise.set_value_emplace( std::forward<Args>( args )... );
  // It's ok not to keep the promise alive here because it is al-
  // ready fulfilled and won't be referenced again.
  return s_promise.wait();
}

template<typename T = std::monostate>
wait<T> empty_wait() {
  return wait_promise<T>{}.wait();
}

template<typename T>
wait<T>::wait( T const& ready_val ) {
  *this = make_wait<T>( ready_val );
}

} // namespace rn
