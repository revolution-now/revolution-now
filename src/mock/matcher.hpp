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
#include "base/to-str.hpp"

// C++ standard library
#include <concepts>
#include <memory>
#include <type_traits>

namespace mock {

/****************************************************************
** Matcher Concepts
*****************************************************************/
template<typename T>
struct IMatcher;

template<typename T>
concept Matcher =
    std::is_base_of_v<IMatcher<typename T::matched_type>, T>;

/****************************************************************
** IMatcher
*****************************************************************/
// This is the interface for matchers.
template<typename T>
struct IMatcher {
  using matched_type = T;

  IMatcher() = default;

  MOVABLE_ONLY( IMatcher );

  virtual ~IMatcher() = default;

  virtual bool matches( T const& val ) const = 0;

  virtual std::string format_expected() const {
    return "<unformatted>";
  }
};

/****************************************************************
** Special Matchers
*****************************************************************/
namespace matchers::detail {

// This is a special matcher that we define in this file because
// it is needed by the Matcher wrapper itself. All of the other
// matchers should go into the dedicated matchers files. This
// matcher should not be referred to directly. Matches an ex-
// plicit value that doesn't have an automatic conversion to a
// MatcherWrapper already defined, as normal matchers do. This
// will allow passing e.g. integer literals into the expected
// mock call.
template<typename T>
struct Value : IMatcher<T> {
  template<typename U>
  requires std::is_constructible_v<std::remove_cvref_t<T>, U>
  Value( U&& val ) : val_( std::forward<U>( val ) ) {}

  bool matches( T const& val ) const override {
    static_assert(
        std::equality_comparable<std::remove_cvref_t<T>>,
        "One of the argument types of this mocked function is "
        "not equality comparable; either make it so, or use the "
        "Any (_) matcher for that argument." );
    return val == val_;
  }

  std::string format_expected() const override {
    if constexpr( base::Show<T> )
      return base::to_str( val_ );
    else
      return this->IMatcher<T>::format_expected();
  }

 private:
  // It is not safe to mock a method that takes a string_view as
  // an argument, since otherwise this framework will store the
  // string_view as the matcher, and that can cause lifetime is-
  // sues if the string_view was constructed from a temporary
  // during the expect-call statement.
  static_assert( !std::is_same_v<std::string_view,
                                 std::remove_cvref_t<T>> );
  std::remove_cvref_t<T> val_;
};

} // namespace matchers::detail

/****************************************************************
** MatcherWrapper
*****************************************************************/
// When matchers need to be stored in mock objects (as a result
// of expect-call statement) they are stored in these wrappers.
// The purpose of this is to provide a concrete non-pointer type
// to act as a target for implicit conversion/construction by
// whatever (value, matcher, or other) object the user writes in
// the expect-call statement as the arguments to expect. An ex-
// plicit value, an AnyTag, or an IMatcher derivative will be
// handled by the constructors below, while an arbitrary object
// will provide an implicit conversion operator to convert to a
// MatcherWrapper of a requested T. The latter is required to
// handle subtle type type conversions between desired matchers
// and actual matchers, e.g., when we need a matcher for `un-
// signed const int*` but we're provided one for `int const*`.
template<typename T>
struct MatcherWrapper {
  // This is for values that are not IMatcher derived.
  template<typename U>
  requires( !Matcher<std::remove_cvref_t<U>> &&
            std::is_constructible_v<std::remove_cvref_t<T>, U> )
  MatcherWrapper( U&& val )
    : matcher_( std::make_unique<matchers::detail::Value<T>>(
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
