/****************************************************************
**node.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-26.
*
* Description: Base class for node for matcher expression tem-
*              plate trees.
*
*****************************************************************/
#pragma once

// mock
#include "matcher.hpp"

#define MATCHER_NODE_PREAMBLE( name )          \
  using Base = detail::Node<name##Impl<T>, T>; \
  using typename Base::held_type;              \
  using Base::Base;                            \
  using Base::operator==;                      \
  using Base::converting_operator_equal;       \
  using Base::converting_operator_ge

#define MATCHER_NODE_STRUCT( name ) \
  template<MatchableValue T>        \
  struct name##Impl final : detail::Node<name##Impl<T>, T>

#define MATCHER_EQUAL_HOOK( lhs, rhs ) \
  template<typename U>                 \
  static bool equal( held_type const& lhs, U const& rhs )

#define MATCHER_DEFINE_NODE( name, lhs, rhs )                \
  MATCHER_NODE_STRUCT( name ) {                              \
    MATCHER_NODE_PREAMBLE( name );                           \
                                                             \
    template<typename U>                                     \
    static bool equal( held_type const& lhs, U const& rhs ); \
  };                                                         \
  template<MatchableValue T>                                 \
  template<typename U>                                       \
  bool name##Impl<T>::equal( held_type const& lhs,           \
                             U const&         rhs )

namespace mock::matchers {

namespace detail {

// When a structure of nodes is asked to produce a matcher (i.e.,
// something deriving from IMatcher), this is what it uses.
template<typename Target, typename HeldType, typename Parent>
struct NodeMatcher : IMatcher<Target> {
  NodeMatcher( HeldType&& children )
    : children_( std::move( children ) ) {}
  NodeMatcher( HeldType const& children )
    : children_( std::move( children ) ) {}
  bool matches( Target const& val ) const override {
    return Parent::equal( children_, val );
  }
  HeldType children_;
};

// This is the common behavior/interface for a node in the struc-
// ture that describes a matching operation.
template<typename Derived, MatchableValue T>
struct Node {
  using held_type = T;

  constexpr Node( T&& val ) : children_( std::move( val ) ) {}

  template<typename Target>
  operator MatcherWrapper<Target>() && {
    return MatcherWrapper<Target>(
        NodeMatcher<Target, held_type, Derived>(
            std::move( children_ ) ) );
  }

  template<typename Target>
  requires std::is_copy_assignable_v<held_type>
  operator MatcherWrapper<Target>() const& {
    return MatcherWrapper<Target>(
        NodeMatcher<Target, held_type, Derived>( children_ ) );
  }

  bool operator==( Node const& ) const = default;

  template<typename U>
  bool operator==( U const& rhs ) const {
    return Derived::equal( children_, rhs );
  }

 protected:
  // These should always be used instead of the bare operators
  // when comparing values for the actual matching operations
  // since it handles implicit conversions.

  template<typename L, typename R>
  static bool converting_operator_equal( L const& lhs,
                                         R const& rhs ) {
    return lhs == maybe_cast<L>( rhs );
  }

  template<typename L, typename R>
  static bool converting_operator_ge( L const& lhs,
                                      R const& rhs ) {
    return lhs >= maybe_cast<L>( rhs );
  }

 private:
  template<typename To, typename From>
  static auto maybe_cast( From const& rhs )
      -> std::conditional_t<std::is_convertible_v<From, To>, To,
                            From const&> {
    if constexpr( std::is_convertible_v<From, To> )
      // Prevents e.g. signed/unsigned int conversion warnings.
      return static_cast<To>( rhs );
    else
      return rhs;
  }

  held_type children_;
};

} // namespace detail

} // namespace mock::matchers
