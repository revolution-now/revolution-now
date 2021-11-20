/****************************************************************
**matcher.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-14.
*
* Description: Interface and type-erased wrapper for Matchers.
*
*****************************************************************/
#pragma once

// base
#include "base/error.hpp"

// C++ standard library
#include <concepts>
#include <memory>
#include <type_traits>

namespace mock {

/****************************************************************
** IMatcher
*****************************************************************/
template<typename T>
concept MatchableValue = std::equality_comparable<T>;

template<MatchableValue T>
struct IMatcher {
  using matched_type = T;

  virtual ~IMatcher() = default;

  virtual bool matches( T const& val ) const = 0;
};

template<typename T>
concept IsMatcher =
    std::is_base_of_v<IMatcher<typename T::matched_type>, T>;

// Without this clang format indents stuff below... strange.
struct FixClangFormat {};

/****************************************************************
** Special Matchers
*****************************************************************/
// These are some special matchers that we define in this file
// because they are needed by the Matcher wrapper itself. All of
// the other matchers should go into the dedicated matcher file.
namespace matchers {

// Matches an explicit value. This matcher is created in response
// to just receiving a plain explicit value to match and should
// not need to be created manually.
template<MatchableValue T>
struct Value : IMatcher<T> {
  /* clang-format off */
  template<typename U>
  requires std::is_constructible_v<T, U>
  /* clang-format on */
  Value( U&& val ) : val_( std::forward<U>( val ) ) {}

  bool matches( T const& val ) const override {
    return val == val_;
  }

private:
  T val_;
};

// Matches anything.
template<typename T>
struct Any : IMatcher<T> {
  bool matches( T const& ) const override { return true; }
};

namespace detail {
struct AnyTag {};
} // namespace detail

inline constexpr auto _ = detail::AnyTag{};

} // namespace matchers

/****************************************************************
** MatcherWrapper
*****************************************************************/
template<MatchableValue T>
struct MatcherWrapper {
  MatcherWrapper() = delete;

  MatcherWrapper( matchers::detail::AnyTag )
    : matcher_( std::make_unique<matchers::Any<T>>() ) {}

  template<typename U>
  requires( MatchableValue<std::remove_cvref_t<U>> &&
            !IsMatcher<std::remove_cvref_t<U>> )
      MatcherWrapper( U&& val )
    : matcher_( std::make_unique<matchers::Value<T>>(
          std::forward<U>( val ) ) ) {}

  template<IsMatcher U>
  requires std::is_same_v<typename U::matched_type, T>
  MatcherWrapper( U&& val )
    : matcher_( std::make_unique<U>( std::forward<U>( val ) ) ) {
  }

  bool matches( T const& val ) const {
    DCHECK( matcher_ );
    return matcher_->matches( val );
  }

  bool operator==( T const& val ) const {
    return matches( val );
  }

private:
  std::unique_ptr<IMatcher<T>> matcher_;
};

} // namespace mock
