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

  // Removes callbacks, so that when a value is set, nothing hap-
  // pens. This probably has no effect if the value has already
  // been set.
  virtual void cancel() = 0;
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

  void cancel() { shared_state_->cancel(); }

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

      void cancel() override { old_shared_state_->cancel(); }

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

    void add_callback(
        std::function<NotifyFunc> callback ) override {
      if( has_value() )
        callback( *maybe_value );
      else
        callbacks_.push_back( std::move( callback ) );
    }

    void cancel() override { callbacks_.clear(); }

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

  waitable<T> get_waitable() const {
    return waitable<T>( shared_state_ );
  }

  /**************************************************************
  ** set_value
  ***************************************************************/
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

  /**************************************************************
  ** set_value_if_not_set
  ***************************************************************/
  void set_value_if_not_set( T const& value ) {
    if( has_value() ) return;
    set_value( value );
  }

  void set_value_if_not_set( T&& value ) {
    if( has_value() ) return;
    set_value( std::move( value ) );
  }

  template<typename... Args>
  void set_value_emplace_if_not_set( Args&&... args ) {
    if( has_value() ) return;
    set_value_emplace( std::forward<Args>( args )... );
  }

private:
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
