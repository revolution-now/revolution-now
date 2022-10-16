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

#include "core-config.hpp"

// base
#include "base/maybe.hpp"
#include "base/unique-coro.hpp"

namespace rn {

namespace detail {

template<typename T>
struct generator_promise {
  base::maybe<T> value;
};

} // namespace detail

template<typename T>
struct [[nodiscard]] generator {
  using promise_type = detail::generator_promise<T>;

  struct generator_sentinel {};

  struct iterator;

  iterator begin() const { return iterator( coro_.resource() ); }

  generator_sentinel end() const { return {}; }

  base::unique_coro<promise_type> coro_;
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

  T const& operator*() const { return coro_.promise().value(); }

  iterator& operator++() {
    coro_.resume();
    return *this;
  }

  iterator operator++( int ) {
    auto res = *this;
    ++( *this );
    return res;
  }

  bool operator!=( generator_sentinel const& ) const {
    return coro_.promise().value.has_value();
  }

  bool operator==( generator_sentinel const& ) const {
    return !coro_.promise().value.has_value();
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

} // namespace rn
