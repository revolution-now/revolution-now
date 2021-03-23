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
#include "base/unique-func.hpp"

// C++ standard library
#include <memory>

namespace rn {

namespace internal {

template<typename T>
class sync_shared_state_base {
  // This is a reference count that counts the number of promise
  // objects that are referring to this shared state object.
  // Promise objects are copyable, so in general this will be
  // larger than one, although in most cases it will be 1 most of
  // the time, because typically there is only one client respon-
  // sible for fulfilling the promise, and and that client typi-
  // cally ends up holding the only copy of that promise. How-
  // ever, there are some cases where multiple clients could po-
  // tentially fulfill a promise (though only one can do so), and
  // so we make the promise copyable.
  //
  // However, when all promises that refer to a shared state
  // perish then we need to release the callbacks 1) because they
  // can no longer be called, and 2) we need to do so in order to
  // break a reference cycle that would otherwise cause memory
  // leaks:
  //
  //   shared_state -> callbacks --> unique_coro -->
  //   coroutine --> temp. waitables --> shared_state
  //
  // and this reference count will tell us when we can do that.
  // We can't use the reference count in the shared_ptr that is
  // used to hold this shared state because its reference count
  // will include contributions from the waitable's.
  int promise_ref_count_ = 1;

public:
  sync_shared_state_base()          = default;
  virtual ~sync_shared_state_base() = default;

  virtual bool has_value() const = 0;
  virtual T    get() const       = 0;

  using NotifyFunc = void( T const& );

protected:
  // Accumulates callbacks in a list, then when the value eventu-
  // ally becomes ready, it will call them all in order. Any
  // callbacks added after the value is ready will be called im-
  // mediately.
  virtual void add_copyable_callback(
      std::function<NotifyFunc> callback ) = 0;

  virtual void add_movable_callback(
      base::unique_func<NotifyFunc> callback ) = 0;

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

  // Removes callbacks, so that when a value is set, nothing hap-
  // pens. This probably has no effect if the value has already
  // been set.
  virtual void clear_callbacks() = 0;

  void inc_promise_count() {
    // Since the ref count starts out at 1, that means that once
    // it hits zero (and the callbacks have been cleared) we
    // should not be incrementing it anymore.
    CHECK( promise_ref_count_ > 0 );
    ++promise_ref_count_;
  }

  void dec_promise_count() {
    CHECK( promise_ref_count_ > 0 );
    if( --promise_ref_count_ == 0 ) clear_callbacks();
  }
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
template<typename T = std::monostate>
class [[nodiscard]] waitable {
  template<typename U>
  using SharedStatePtr =
      std::shared_ptr<internal::sync_shared_state_base<U>>;

public:
  using value_type = T;

  // FIXME: remove after land-view is fixed.
  waitable();

  waitable( T const& ready_val );

  // This constructor should not be used by client code.
  explicit waitable( SharedStatePtr<T> shared_state )
    : shared_state_{ shared_state } {}

  bool operator==( waitable<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( waitable<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool ready() const {
    return shared_state_ && shared_state_->has_value();
  }

  operator bool() const { return ready(); }

  void clear_callbacks() { shared_state_->clear_callbacks(); }

  // Gets the value (running any continuations) and returns the
  // value, leaving the waitable in the same state.
  T get() {
    CHECK( ready(),
           "attempt to get value from waitable when not in "
           "`ready` state." );
    return shared_state_->get();
  }

  // FIXME: remove and replace with coroutines.
  // FIXME
  template<typename Func>
  auto fmap( Func&& func ) {
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

      void add_copyable_callback(
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

      void add_movable_callback(
          base::unique_func<
              typename internal::sync_shared_state_base<
                  NewResult_t>::NotifyFunc>
              callback ) override {
        old_shared_state_->add_callback(
            [this, callback = std::move( callback )](
                T const& val ) mutable {
              callback( this->continuation_( val ) );
            } );
      }

      void clear_callbacks() override {
        old_shared_state_->clear_callbacks();
      }

      SharedStatePtr<T> old_shared_state_;
      // continuation :: T -> NewResult_t
      Func continuation_;
    };

    return waitable<NewResult_t>(
        std::make_shared<sync_shared_state_with_continuation>(
            shared_state_, std::forward<Func>( func ) ) );
  }

  // FIXME: remove and replace with coroutines.
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
};

/****************************************************************
** waitable_promise
*****************************************************************/
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

    void add_copyable_callback(
        std::function<NotifyFunc> callback ) override {
      if( has_value() )
        callback( *maybe_value );
      else
        callbacks_.push_back( std::move( callback ) );
    }

    void add_movable_callback(
        base::unique_func<NotifyFunc> callback ) override {
      if( has_value() )
        callback( *maybe_value );
      else
        ucallbacks_.push_back( std::move( callback ) );
    }

    void clear_callbacks() override {
      callbacks_.clear();
      ucallbacks_.clear();
    }

    void do_callbacks() {
      CHECK( has_value() );
      for( auto const& callback : callbacks_ )
        callback( *maybe_value );
      for( auto& callback : ucallbacks_ )
        callback( *maybe_value );
      // The callbacks can/should only be called once (at least
      // when the program is behaving correctly) and so now that
      // we've called them, we can take the opportunity to delete
      // them which helps to break memory cycles, since the call-
      // backs can own other objects through their captures that
      // can in turn reference this shared state.
      clear_callbacks();
    }

    maybe<T> maybe_value;
    // Currently we have two separate vectors for unique and
    // non-unique function callbacks. This may cause callbacks to
    // get invoked in a different order than they were inserted.
    // Don't think this should a problem...
    std::vector<std::function<NotifyFunc>>     callbacks_;
    std::vector<base::unique_func<NotifyFunc>> ucallbacks_;
  };

public:
  waitable_promise() : shared_state_( new sync_shared_state ) {
    // shared state ref count should initialize to 1.
  }

  ~waitable_promise() noexcept {
    // shared_state_ could be nullptr if we've been moved from.
    if( shared_state_ ) shared_state_->dec_promise_count();
  }

  waitable_promise( waitable_promise const& rhs )
    : shared_state_( rhs.shared_state_ ) {
    CHECK( shared_state_ );
    shared_state_->inc_promise_count();
  }

  waitable_promise& operator=( waitable_promise const& rhs ) {
    if( *this == rhs ) return *this;
    CHECK( shared_state_ )
    CHECK( rhs.shared_state_ )
    shared_state_->dec_promise_count();
    shared_state_ = rhs.shared_state_;
    shared_state_->inc_promise_count();
    return *this;
  }

  // The promise ref count should take care of itself in this
  // one.
  waitable_promise( waitable_promise&& rhs ) = default;

  waitable_promise& operator=( waitable_promise&& r ) noexcept {
    if( *this == r ) return *this;
    shared_state_->dec_promise_count();
    shared_state_ = std::move( r.shared_state_ );
    return *this;
  }

  bool operator==( waitable_promise<T> const& rhs ) const {
    return shared_state_.get() == rhs.shared_state_.get();
  }

  bool operator!=( waitable_promise<T> const& rhs ) const {
    return !( *this == rhs );
  }

  bool has_value() const { return shared_state_->has_value(); }

  waitable<T> get_waitable() const {
    return waitable<T>( shared_state_ );
  }

  /**************************************************************
  ** set_value
  ***************************************************************/
  void set_value( T const& value ) const {
    CHECK( !has_value() );
    mutable_state()->maybe_value = value;
    mutable_state()->do_callbacks();
  }

  void set_value( T&& value ) const {
    CHECK( !has_value() );
    mutable_state()->maybe_value = std::move( value );
    mutable_state()->do_callbacks();
  }

  template<typename... Args>
  void set_value_emplace( Args&&... args ) const {
    CHECK( !has_value() );
    mutable_state()->maybe_value.emplace(
        std::forward<Args>( args )... );
    mutable_state()->do_callbacks();
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
  sync_shared_state* mutable_state() const {
    return const_cast<sync_shared_state*>( shared_state_.get() );
  }

  std::shared_ptr<sync_shared_state> shared_state_;
};

/****************************************************************
** Helpers
*****************************************************************/
template<typename Fsm>
void advance_fsm_ui_state( Fsm* fsm, waitable<>* s_future ) {
  if( s_future->ready() ) {
    fsm->pop();
    s_future->get();
  }
}

// Returns a waitable immediately containing the given value.
template<typename T = std::monostate, typename... Args>
waitable<T> make_waitable( Args&&... args ) {
  waitable_promise<T> s_promise;
  s_promise.set_value_emplace( std::forward<Args>( args )... );
  return s_promise.get_waitable();
}

// FIXME: remove when land-view is fixed.
template<typename T>
waitable<T>::waitable() : waitable( T{} ) {}

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
