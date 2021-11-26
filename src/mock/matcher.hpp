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
#include "base/macros.hpp"

// C++ standard library
#include <concepts>
#include <memory>
#include <type_traits>

namespace mock {

/****************************************************************
** Matcher Concepts
*****************************************************************/
template<typename T>
concept HasMatchedType = requires {
  typename T::matched_type;
};

template<typename T>
concept MatchableValue = std::equality_comparable<T>;

template<MatchableValue T>
struct IMatcher;

template<typename T>
concept Matcher = HasMatchedType<T> &&
    std::is_base_of_v<IMatcher<typename T::matched_type>, T>;

// Without this clang format indents stuff below... strange.
struct FixClangFormat {};

template<typename T>
struct matched_type_for {
  using type = T;
};

template<HasMatchedType T>
struct matched_type_for<T> {
  using type = typename T::matched_type;
};

/****************************************************************
** IMatcher
*****************************************************************/
template<MatchableValue T>
struct IMatcher {
  using matched_type = T;

  IMatcher() = default;

  MOVABLE_ONLY( IMatcher );

  virtual ~IMatcher() = default;

  virtual bool matches( T const& val ) const = 0;
};

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

// Matches anything of type T.
template<typename T>
struct Any : IMatcher<T> {
  bool matches( T const& ) const override { return true; }
};

namespace detail {
struct AnyTag {};
} // namespace detail

// Matches any value of any type.
inline constexpr auto _ = detail::AnyTag{};

} // namespace matchers

/****************************************************************
** MatcherWrapper
*****************************************************************/
// When matchers need to be stored in mock objects (as a result
// of EXPECT_CALL) they are stored in these wrappers. The purpose
// of this is to provide a concrete non-pointer type to act as a
// target for implicit conversion/construction by whatever
// (value, matcher, or other) object the user writes in the
// EXPECT_CALL as the arguments to expect. An explicit value, an
// AnyTag, or an IMatcher derivative will be handled by the con-
// structors below, while an arbitrary object will provide an im-
// plicit conversion operator to convert to a MatcherWrapper of a
// requested T. The latter is required to handle subtle type type
// conversions between desired matchers and actual matchers,
// e.g., when we need a matcher for `unsigned const int*` but
// we're provided one for `int const*`.
template<MatchableValue T>
struct MatcherWrapper {
  // This is for the wildcard matcher, `_`.
  MatcherWrapper( decltype( matchers::_ ) )
    : matcher_( std::make_unique<matchers::Any<T>>() ) {}

  // This is for explicit values that are not IMatcher derived.
  template<typename U>
  requires( MatchableValue<std::remove_cvref_t<U>> &&
            !Matcher<std::remove_cvref_t<U>> &&
            std::is_constructible_v<T, U> )
      MatcherWrapper( U&& val )
    : matcher_( std::make_unique<matchers::Value<T>>(
          std::forward<U>( val ) ) ) {}

  // This is for IMatcher-derived objects.
  template<Matcher U>
  requires std::is_same_v<typename U::matched_type, T>
  MatcherWrapper( U&& val )
    : matcher_( std::make_unique<U>( std::forward<U>( val ) ) ) {
  }

  IMatcher<T> const& matcher() const { return *matcher_; }

 private:
  MatcherWrapper() = delete;

  std::unique_ptr<IMatcher<T>> matcher_;
};

} // namespace mock
