/****************************************************************
**generator.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Coroutine generator type.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"
#include "unique-coro.hpp"

namespace base {

template<typename T>
struct generator;

namespace detail {

template<typename T>
struct generator_promise {
  auto initial_suspend() const { return std::suspend_never{}; }

  maybe<T const&> value() {
    if( std::coroutine_handle<generator_promise>::from_promise(
            *this )
            .done() )
      return nothing;
    CHECK( value_ != nullptr );
    return *value_;
  }

  // This will prevent using co_await in a generator.
  template<typename U>
  std::suspend_never await_transform( U&& value ) = delete;

  auto final_suspend() const noexcept {
    // Always suspend so that the coroutine does not release it-
    // self when it finishes; our unique_coro object will own the
    // coroutine and will release it when it goes out of scope.
    return std::suspend_always{};
  }

  std::suspend_always yield_value( T const& value ) noexcept {
    value_ = std::addressof( value );
    return {};
  }

  std::suspend_always yield_value( T&& value ) noexcept {
    value_ = std::addressof( value );
    return {};
  }

  // Ensure that this is not copyable. See
  // https://devblogs.microsoft.com/oldnewthing/20210504-00/?p=105176
  // for the arcane reason as to why.
  generator_promise()                           = default;
  generator_promise( generator_promise const& ) = delete;

  void operator=( generator_promise const& ) = delete;

  generator<T> get_return_object();

  void unhandled_exception() {
    exception_ = std::current_exception();
  }

  void rethrow_if_exception() const {
    if( exception_ ) std::rethrow_exception( exception_ );
  }

 private:
  T const*           value_     = nullptr;
  std::exception_ptr exception_ = {};
};

} // namespace detail

template<typename T>
struct [[nodiscard]] generator {
  using promise_type = detail::generator_promise<T>;

  generator(
      std::coroutine_handle<detail::generator_promise<T>> h )
    : coro_( h ) {}

  struct generator_sentinel {};

  struct iterator;

  maybe<T const&> value() { return coro_.promise().value(); }

  iterator begin() const { return iterator( coro_.resource() ); }

  generator_sentinel end() const { return {}; }

  unique_coro<promise_type> coro_;
};

template<typename T>
struct generator<T>::iterator {
  using iterator_category = std::input_iterator_tag;
  using difference_type   = int;
  using value_type        = T;
  using pointer           = T const*;
  using reference         = T const&;

  iterator()                  = default;
  iterator( iterator const& ) = default;
  iterator( iterator&& )      = default;
  iterator( std::coroutine_handle<promise_type> coro )
    : coro_( coro ) {}

  iterator& operator=( iterator const& ) = default;
  iterator& operator=( iterator&& )      = default;

  T const& operator*() const {
    maybe<T const&> res = coro_.promise().value();
    CHECK( res.has_value() );
    return *res;
  }

  iterator& operator++() {
    CHECK( !coro_.done() );
    coro_.resume();
    if( coro_.done() ) coro_.promise().rethrow_if_exception();
    return *this;
  }

  // We need to provide this so that this iterator fits the right
  // concepts, but since the value is not stored in the iterator,
  // we can't really return the "original" iterator and have it
  // do the right thing when we do *iter++, so we'll prevent the
  // user from doing that by returning void.
  void operator++( int ) { (void)( ++( *this ) ); }

  bool operator!=( generator_sentinel const& ) const {
    return coro_.promise().value().has_value();
  }

  bool operator==( generator_sentinel const& ) const {
    return !coro_.promise().value().has_value();
  }

  bool operator!=( iterator const& ) const {
    SHOULD_NOT_BE_HERE;
  }

  bool operator==( iterator const& ) const {
    SHOULD_NOT_BE_HERE;
  }

  // Non-owning.
  std::coroutine_handle<promise_type> coro_;
};

namespace detail {

template<typename T>
generator<T> generator_promise<T>::get_return_object() {
  return generator<T>(
      std::coroutine_handle<
          detail::generator_promise<T>>::from_promise( *this ) );
}
}

} // namespace base
