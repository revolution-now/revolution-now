/****************************************************************
**matchers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-14.
*
* Description: Matchers.
*
*****************************************************************/
#pragma once

// mock
#include "matcher.hpp"

namespace mock::matchers {

/****************************************************************
** Pointee
*****************************************************************/
namespace detail {

template<typename T>
struct PointeeMatcher : IMatcher<T const*> {
  using Base = IMatcher<T const*>;

  template<typename U>
  PointeeMatcher( U&& val ) : child_( std::forward<U>( val ) ) {}

  bool matches( T const* const& val ) const override {
    return child_.matcher().matches( *val );
  }

  template<typename Target>
  operator MatcherWrapper<Target>() && {
    static_assert( std::is_convertible_v<
                   decltype( *std::declval<Target>() ),
                   decltype( *std::declval<
                             typename Base::matched_type>() )> );
    struct TargetPointeeMatcher : IMatcher<Target> {
      TargetPointeeMatcher( MatcherWrapper<T>&& child )
        : child_( std::move( child ) ) {}
      bool matches( Target const& val ) const override {
        return child_.matcher().matches(
            static_cast<T const&>( *val ) );
      }
      MatcherWrapper<T> child_;
    };
    return MatcherWrapper<Target>(
        TargetPointeeMatcher{ std::move( child_ ) } );
  }

 private:
  MatcherWrapper<T> child_;
};

} // namespace detail

// We need this function because if Pointee were the matcher
// struct itself then we can't seem to properly constructed
// nested Pointees, i.e. Pointee( Pointee( 8 ) ) seems to get
// collapsed into a single Pointee<int> instead of Pointee<int*>
// which is what we want, and there doesn't seem to be any way to
// prevent it (caused by guaranteed copy elision?), even with de-
// duction guides.
template<typename T>
auto Pointee( T&& arg ) {
  using base_t = std::remove_reference_t<T>;
  if constexpr( Matcher<base_t> ) {
    return detail::PointeeMatcher<typename base_t::matched_type>(
        std::forward<T>( arg ) );
  } else {
    return detail::PointeeMatcher<base_t>(
        std::forward<T>( arg ) );
  }
}

} // namespace mock::matchers
