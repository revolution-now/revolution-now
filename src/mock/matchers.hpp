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

// base
#include "base/meta.hpp"

namespace mock::matchers {

/****************************************************************
** Helpers
*****************************************************************/
namespace detail {

// When a structure of nodes is asked to produce a matcher (i.e.,
// something deriving from IMatcher), this is what it uses.
template<typename Target, typename HeldType, typename Parent>
struct TargetMatcher : IMatcher<Target> {
  TargetMatcher( HeldType&& children )
    : children_( std::move( children ) ) {}
  bool matches( Target const& val ) const override {
    return Parent::compare( children_, val );
  }
  HeldType children_;
};

// This is the common behavior/interface for a node in the struc-
// ture that describes a matching operation.
template<typename Derived, MatchableValue T>
struct Node {
  using held_type = T;

  Node( T&& val ) : children_( std::move( val ) ) {}

  template<typename Target>
  operator MatcherWrapper<Target>() && {
    return MatcherWrapper<Target>(
        TargetMatcher<Target, held_type, Derived>(
            std::move( children_ ) ) );
  }

  bool operator==( Node const& ) const = default;

  template<typename U>
  bool operator==( U const& rhs ) const {
    return Derived::compare( children_, rhs );
  }

 private:
  held_type children_;
};

// Order might matter here because there is an assymmetry in how
// the conversion is done between the two parameters.
template<typename L, typename R>
bool converting_operator_equal( L const& lhs, R const& rhs ) {
  if constexpr( std::is_convertible_v<R, L> )
    // Prevents e.g. signed/unsigned integer conversion warn-
    // ings.
    return lhs == static_cast<L>( rhs );
  else
    return lhs == rhs;
}

} // namespace detail

#define MATCHER_NODE_PREAMBLE( name )          \
  using Base = detail::Node<name##Impl<T>, T>; \
  using typename Base::held_type;              \
  using Base::Base;                            \
  using Base::operator==

/****************************************************************
** Pointee
*****************************************************************/
namespace detail {

template<MatchableValue T>
struct PointeeImpl final : detail::Node<PointeeImpl<T>, T> {
  MATCHER_NODE_PREAMBLE( Pointee );

  template<typename U>
  static bool compare( held_type const& lhs, U const& rhs ) {
    return converting_operator_equal( lhs, *rhs );
  }
};

} // namespace detail

// We need these little functions because if Pointee is the
// struct itself then we can't seem to properly construct nested
// Pointees, i.e. Pointee( Pointee( 8 ) ) seems to get collapsed
// into a single Pointee<int> instead of Pointee<int*> which is
// what we want, and there doesn't seem to be any way to prevent
// it (caused by guaranteed copy elision?), even with deduction
// guides.
template<typename T>
auto Pointee( T&& arg ) {
  return detail::PointeeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** IterableContains
*****************************************************************/
namespace detail {

template<MatchableValue T>
struct IterableContainsImpl final
  : detail::Node<IterableContainsImpl<T>, T> {
  MATCHER_NODE_PREAMBLE( IterableContains );

  template<typename U>
  static bool compare( held_type const& lhs, U const& rhs ) {
    bool stop = false;
    auto it   = std::begin( rhs );
    FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
      if( it == std::end( rhs ) ||
          !converting_operator_equal( std::get<Idx>( lhs ),
                                      *it ) )
        stop = true;
      else
        ++it;
      return stop;
    };
    bool equal = ( it == std::end( rhs ) ) && !stop;
    return equal;
  }
};

} // namespace detail

template<typename... M>
auto IterableContains( M&&... to_match ) {
  using child_t = std::tuple<std::remove_reference_t<M>...>;
  return detail::IterableContainsImpl<child_t>(
      child_t{ std::forward<M>( to_match )... } );
}

} // namespace mock::matchers
