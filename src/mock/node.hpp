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

#define MATCHER_NODE_PREAMBLE( name )                       \
  using Base =                                              \
      detail::Node<name##Impl<T, MustMatch>, T, MustMatch>; \
  using typename Base::held_type;                           \
  using Base::Base;                                         \
  using Base::operator==;                                   \
  using Base::converting_operator_equal;                    \
  using Base::converting_operator_greater

#define MATCHER_NODE_STRUCT( name )        \
  template<typename T, typename MustMatch> \
  struct name##Impl final                  \
    : detail::Node<name##Impl<T, MustMatch>, T, MustMatch>

#define MATCHER_EQUAL_HOOK( lhs, rhs ) \
  template<typename U>                 \
  static bool equal( held_type const& lhs, U const& rhs )

#define MATCHER_DEFINE_NODE( name, lhs, rhs )                  \
  namespace detail {                                           \
  MATCHER_NODE_STRUCT( name ) {                                \
    MATCHER_NODE_PREAMBLE( name );                             \
                                                               \
    friend void to_str( name##Impl const& o, std::string& out, \
                        base::tag<name##Impl> ) {              \
      to_str( o, out, base::tag<Base>{} );                     \
    }                                                          \
                                                               \
    template<typename U>                                       \
    static bool equal( held_type const& lhs, U const& rhs );   \
  };                                                           \
  }                                                            \
  template<typename T, typename MustMatch>                     \
  template<typename U>                                         \
  bool detail::name##Impl<T, MustMatch>::equal(                \
      held_type const& lhs, U const& rhs )

namespace mock::matchers {

namespace detail {

// When a structure of nodes is asked to produce a matcher (i.e.,
// something deriving from IMatcher), this is what it uses.
template<typename Target, typename HeldType, typename Parent>
struct NodeMatcher : IMatcher<Target> {
  NodeMatcher( std::string_view name, HeldType&& children )
    : name_( name ), children_( std::move( children ) ) {}

  NodeMatcher( std::string_view name, HeldType const& children )
    : name_( name ), children_( std::move( children ) ) {}

  bool matches( Target const& val ) const override {
    return Parent::equal( children_, val );
  }

  std::string format_expected() const override {
    return fmt::format(
        "{}( {} )", name_,
        stringify( children_, "<unformattable>" ) );
  }

  friend void to_str( NodeMatcher const& o, std::string& out,
                      base::tag<NodeMatcher> ) {
    // Defer to base class implementation.
    to_str( o, out, base::tag<IMatcher<Target>>{} );
  }

  std::string_view name_;
  HeldType children_;
};

// Common Node stuff not dependent on the template parameters of
// Node.
struct NodeBase {
  constexpr NodeBase( std::string_view name ) : name_( name ) {}

  constexpr std::string_view name() const { return name_; }

 private:
  std::string_view name_ = {};

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
  static bool converting_operator_greater( L const& lhs,
                                           R const& rhs ) {
    return lhs > maybe_cast<L>( rhs );
  }

 private:
  template<typename To, typename From>
  static auto
  maybe_cast( From const& rhs ) -> std::conditional_t<
      std::is_convertible_v<From const&, To>, To, From const&> {
    if constexpr( std::is_convertible_v<From const&, To> )
      // Prevents e.g. signed/unsigned int conversion warnings.
      return static_cast<To>( rhs );
    else
      return rhs;
  }
};

// The purpose of this it to allow us to (conditionally) enforce
// that we will only convert to a MatcherWrapper of a certain
// type. Typically we don't want to do this (MustMatch=void), in
// which case this concept just returns true and doesn't affect
// anything. But for certain matchers we want to enforce that it
// can only match a certain type (for the purpose of disam-
// biguating overloaded functions) in which case this will en-
// force that.
template<typename Target, typename MustMatch>
concept MaybeEnforceType = ( std::is_same_v<MustMatch, void> ||
                             std::is_same_v<MustMatch, Target> );

// This is the common behavior/interface for a node in the struc-
// ture that describes a matching operation.
template<typename Derived, typename T, typename MustMatch>
struct Node : public NodeBase {
  using held_type = T;

  using NodeBase::converting_operator_equal;
  using NodeBase::converting_operator_greater;

  explicit constexpr Node( std::string_view name, T&& val )
    : NodeBase( name ), children_( std::move( val ) ) {}

  template<MaybeEnforceType<MustMatch> Target>
  operator MatcherWrapper<Target>() && {
    return MatcherWrapper<Target>(
        NodeMatcher<Target, held_type, Derived>(
            name(), std::move( children_ ) ) );
  }

  template<MaybeEnforceType<MustMatch> Target>
  requires std::is_copy_assignable_v<held_type>
  operator MatcherWrapper<Target>() const& {
    return MatcherWrapper<Target>(
        NodeMatcher<Target, held_type, Derived>( name(),
                                                 children_ ) );
  }

  bool operator==( Node const& ) const = default;

  template<typename U>
  bool operator==( U const& rhs ) const {
    return Derived::equal( children_, rhs );
  }

  friend void to_str( Node const& o, std::string& out,
                      base::tag<Node> ) {
    out += fmt::format(
        "{}( {} )", o.name(),
        stringify( o.children_, "<unformattable>" ) );
  }

 private:
  held_type children_;
};

} // namespace detail

} // namespace mock::matchers
