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

// Rds
#include "matchers.rds.hpp"

// mock
#include "matcher.hpp"
#include "node.hpp"

// base
#include "base/meta.hpp"

// C++ standard library
#include <algorithm>
#include <regex>

#define GENERIC_SINGLE_ARG_MATCHER( name )                   \
  template<typename T>                                       \
  auto name( T&& arg ) {                                     \
    return detail::name##Impl<std::remove_cvref_t<T>, void>( \
        #name, std::forward<T>( arg ) );                     \
  }

#define GENERIC_ZERO_ARG_MATCHER( name )                        \
  inline constexpr auto name() {                                \
    struct Unused {                                             \
      bool operator==( Unused const& ) const = default;         \
    };                                                          \
    return detail::name##Impl<Unused, void>( #name, Unused{} ); \
  }

#define GENERIC_TUPLE_ARG_MATCHER( name )                      \
  template<typename... M>                                      \
  auto name( M&&... to_match ) {                               \
    using child_t = std::tuple<std::remove_reference_t<M>...>; \
    return detail::name##Impl<child_t, void>(                  \
        #name, child_t{ std::forward<M>( to_match )... } );    \
  }

#define CONCRETE_SINGLE_ARG_MATCHER( name, type )              \
  inline auto name( type arg ) {                               \
    return detail::name##Impl<type, void>( #name,              \
                                           std::move( arg ) ); \
  }

// This one is a bit specific in that it makes use of the Must-
// Match and defaults its parameter to Any(). It is for imple-
// menting the `Type` matcher; not sure if it will have other
// uses.
#define TYPE_SINGLE_ARG_MATCHER_MUSTMATCH( name )              \
  template<typename MustMatch, typename T = decltype( Any() )> \
  auto name( T&& arg = Any() ) {                               \
    return detail::name##Impl<std::remove_cvref_t<T>,          \
                              MustMatch>(                      \
        #name, std::forward<T>( arg ) );                       \
  }

namespace mock::matchers {

/****************************************************************
** Any
*****************************************************************/
MATCHER_DEFINE_NODE( Any, /*held*/, /*actual*/ ) {
  return true;
};

GENERIC_ZERO_ARG_MATCHER( Any );

inline constexpr auto _ = Any();

/****************************************************************
** Type
*****************************************************************/
// This matcher enforces that it must match only a certain type,
// and therefore is used to disambiguate overloaded functions. It
// takes an arbitrary matcher as an argument to which it will
// forward matching, though this argument is optional and de-
// faults to "Any". Note that the argument can also be a value.
// See the unit tests for examples on how to use it.
MATCHER_DEFINE_NODE( Type, held, actual ) {
  return converting_operator_equal( held, actual );
};

TYPE_SINGLE_ARG_MATCHER_MUSTMATCH( Type );

/****************************************************************
** Pointee
*****************************************************************/
MATCHER_DEFINE_NODE( Pointee, held, actual ) {
  return converting_operator_equal( held, *actual );
};

GENERIC_SINGLE_ARG_MATCHER( Pointee );

/****************************************************************
** IterableElementsAre
*****************************************************************/
MATCHER_DEFINE_NODE( IterableElementsAre, held, actual ) {
  bool should_stop = false;
  auto it          = std::begin( actual );
  FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
    should_stop = ( it == std::end( actual ) ) ||
                  !converting_operator_equal(
                      std::get<Idx>( held ), *it++ );
    return should_stop;
  };
  return ( it == std::end( actual ) ) && !should_stop;
};

GENERIC_TUPLE_ARG_MATCHER( IterableElementsAre );

/****************************************************************
** Ge
*****************************************************************/
MATCHER_DEFINE_NODE( Ge, held, actual ) {
  return converting_operator_greater( actual, held ) ||
         converting_operator_equal( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Ge );

/****************************************************************
** Le
*****************************************************************/
MATCHER_DEFINE_NODE( Le, held, actual ) {
  return !converting_operator_greater( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Le );

/****************************************************************
** Gt
*****************************************************************/
MATCHER_DEFINE_NODE( Gt, held, actual ) {
  return converting_operator_greater( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Gt );

/****************************************************************
** Lt
*****************************************************************/
MATCHER_DEFINE_NODE( Lt, held, actual ) {
  return !converting_operator_equal( actual, held ) &&
         !converting_operator_greater( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Lt );

/****************************************************************
** Ne
*****************************************************************/
MATCHER_DEFINE_NODE( Ne, held, actual ) {
  return !converting_operator_equal( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Ne );

/****************************************************************
** Eq
*****************************************************************/
MATCHER_DEFINE_NODE( Eq, held, actual ) {
  return converting_operator_equal( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Eq );

/****************************************************************
** Not
*****************************************************************/
MATCHER_DEFINE_NODE( Not, held, actual ) {
  return !converting_operator_equal( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Not );

/****************************************************************
** Approx
*****************************************************************/
MATCHER_DEFINE_NODE( Approx, held, actual ) {
  double const lower_bound = held.target - held.plus_minus;
  double const upper_bound = held.target + held.plus_minus;
  return ( actual >= lower_bound ) && ( actual <= upper_bound );
};

inline auto Approx( double target, double plus_minus ) {
  return detail::ApproxImpl<ApproxData, /*MustMatch=*/void>(
      "Approx",
      ApproxData{ .target = target, .plus_minus = plus_minus } );
}

/****************************************************************
** Approxf
*****************************************************************/
MATCHER_DEFINE_NODE( Approxf, held, actual ) {
  double const lower_bound = held.target - held.plus_minus;
  double const upper_bound = held.target + held.plus_minus;
  return ( actual >= lower_bound ) && ( actual <= upper_bound );
};

inline auto Approxf( float target, float plus_minus ) {
  return detail::ApproxImpl<ApproxData, /*MustMatch=*/void>(
      "Approxf",
      ApproxData{ .target = target, .plus_minus = plus_minus } );
}

/****************************************************************
** StartsWith
*****************************************************************/
MATCHER_DEFINE_NODE( StartsWith, held, actual ) {
  return std::string_view( actual ).starts_with( held );
};

CONCRETE_SINGLE_ARG_MATCHER( StartsWith, std::string );

/****************************************************************
** StrContains
*****************************************************************/
MATCHER_DEFINE_NODE( StrContains, held, actual ) {
  return std::string_view( actual ).find( held ) !=
         std::string_view::npos;
};

CONCRETE_SINGLE_ARG_MATCHER( StrContains, std::string );

/****************************************************************
** Matches
*****************************************************************/
MATCHER_DEFINE_NODE( Matches, held, actual ) {
  return std::regex_match( std::string( actual ),
                           std::regex( held ) );
};

CONCRETE_SINGLE_ARG_MATCHER( Matches, std::string );

/****************************************************************
** Empty
*****************************************************************/
MATCHER_DEFINE_NODE( Empty, /*held*/, actual ) {
  return actual.empty();
};

GENERIC_ZERO_ARG_MATCHER( Empty );

/****************************************************************
** HasSize
*****************************************************************/
MATCHER_DEFINE_NODE( HasSize, held, actual ) {
  return converting_operator_equal( held, actual.size() );
};

GENERIC_SINGLE_ARG_MATCHER( HasSize );

/****************************************************************
** Each
*****************************************************************/
// Matches a container, and requires that each element in the
// container match a given (single) matcher/value.
MATCHER_DEFINE_NODE( Each, held, actual ) {
  return std::all_of(
      actual.begin(), actual.end(), [&held]( auto const& elem ) {
        return converting_operator_equal( elem, held );
      } );
};

GENERIC_SINGLE_ARG_MATCHER( Each );

/****************************************************************
** AllOf
*****************************************************************/
// Matches a value and requires that all supplied matchers match
// that same value.
MATCHER_DEFINE_NODE( AllOf, held, actual ) {
  bool should_stop = false;
  FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
    should_stop = !converting_operator_equal(
        std::get<Idx>( held ), actual );
    return should_stop;
  };
  return !should_stop;
};

GENERIC_TUPLE_ARG_MATCHER( AllOf );

/****************************************************************
** AnyOf
*****************************************************************/
// Matches a value and requires that all supplied matchers match
// that same value.
MATCHER_DEFINE_NODE( AnyOf, held, actual ) {
  bool at_least_one_matches = false;
  FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
    at_least_one_matches = converting_operator_equal(
        std::get<Idx>( held ), actual );
    bool should_stop = at_least_one_matches;
    return should_stop;
  };
  return at_least_one_matches;
};

GENERIC_TUPLE_ARG_MATCHER( AnyOf );

/****************************************************************
** TupleElement
*****************************************************************/
// The specified tuple element matches the specified matcher.
// This should work with any tuple-like type that supports
// std::get<N>();
MATCHER_DEFINE_NODE( TupleElement, held, actual ) {
  constexpr size_t N = decltype( held.first )::value;
  auto& matcher      = held.second;
  return converting_operator_equal( matcher,
                                    std::get<N>( actual ) );
};

template<size_t N, typename M>
auto TupleElement( M&& to_match ) {
  using child_t = std::pair<std::integral_constant<size_t, N>,
                            std::remove_reference_t<M>>;
  return detail::TupleElementImpl<child_t, /*MustMatch=*/void>(
      "TupleElement",
      child_t{ {}, std::forward<M>( to_match ) } );
}

/****************************************************************
** Key
*****************************************************************/
template<typename M>
auto Key( M&& to_match ) {
  using child_t = std::pair<std::integral_constant<size_t, 0>,
                            std::remove_reference_t<M>>;
  return detail::TupleElementImpl<child_t, /*MustMatch=*/void>(
      "Key", child_t{ {}, std::forward<M>( to_match ) } );
}

/****************************************************************
** Field
*****************************************************************/
// Given a member variable pointer and a matcher, it requires
// that the member variable match the matcher.
MATCHER_DEFINE_NODE( Field, held, actual ) {
  return converting_operator_equal( held.second,
                                    actual.*( held.first ) );
};

// TODO: extract the class type from the member pointer and put
// it in MustMatch so that a Field matcher on its own can also
// disambiguate overloaded functions.
template<typename MemberVarT, typename M>
requires std::is_member_object_pointer_v<MemberVarT>
auto Field( MemberVarT&& member_ptr, M&& to_match ) {
  using child_t = std::pair<std::remove_reference_t<MemberVarT>,
                            std::remove_reference_t<M>>;
  return detail::FieldImpl<child_t, /*MustMatch=*/void>(
      "Field",
      child_t{ member_ptr, std::forward<M>( to_match ) } );
}

/****************************************************************
** Property
*****************************************************************/
// Given a pointer to member getter and a matcher, it requires
// that the property match the matcher.
MATCHER_DEFINE_NODE( Property, held, actual ) {
  return converting_operator_equal(
      held.second, ( actual.*( held.first ) )() );
};

template<typename MemberFnT, typename M>
requires std::is_member_function_pointer_v<MemberFnT>
auto Property( MemberFnT&& member_ptr, M&& to_match ) {
  using child_t = std::pair<std::remove_reference_t<MemberFnT>,
                            std::remove_reference_t<M>>;
  return detail::PropertyImpl<child_t, /*MustMatch=*/void>(
      "Property",
      child_t{ member_ptr, std::forward<M>( to_match ) } );
}

/****************************************************************
** Boolean
*****************************************************************/
MATCHER_DEFINE_NODE( Boolean, held, actual ) {
  return bool( actual ) == held;
};

CONCRETE_SINGLE_ARG_MATCHER( Boolean, bool );

/****************************************************************
** True
*****************************************************************/
MATCHER_DEFINE_NODE( True, /*held*/, actual ) {
  return bool( actual ) == true;
};

GENERIC_ZERO_ARG_MATCHER( True );

/****************************************************************
** False
*****************************************************************/
MATCHER_DEFINE_NODE( False, /*held*/, actual ) {
  return bool( actual ) == false;
};

GENERIC_ZERO_ARG_MATCHER( False );

/****************************************************************
** Null
*****************************************************************/
MATCHER_DEFINE_NODE( Null, /*held*/, actual ) {
  return actual == nullptr;
};

GENERIC_ZERO_ARG_MATCHER( Null );

} // namespace mock::matchers
