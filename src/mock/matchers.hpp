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
#include "node.hpp"

// base
#include "base/meta.hpp"

#define GENERIC_SINGLE_ARG_MATCHER( name )             \
  template<MatchableValue T>                           \
  auto name( T&& arg ) {                               \
    return detail::name##Impl<std::remove_cvref_t<T>>( \
        std::forward<T>( arg ) );                      \
  }

#define GENERIC_ZERO_ARG_MATCHER( name )                \
  inline constexpr auto name() {                        \
    struct Unused {                                     \
      bool operator==( Unused const& ) const = default; \
    };                                                  \
    return detail::name##Impl<Unused>( Unused{} );      \
  }

#define GENERIC_TUPLE_ARG_MATCHER( name )                      \
  template<MatchableValue... M>                                \
  auto name( M&&... to_match ) {                               \
    using child_t = std::tuple<std::remove_reference_t<M>...>; \
    return detail::name##Impl<child_t>(                        \
        child_t{ std::forward<M>( to_match )... } );           \
  }

#define CONCRETE_SINGLE_ARG_MATCHER( name, type )        \
  inline auto name( type arg ) {                         \
    return detail::name##Impl<type>( std::move( arg ) ); \
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
** Not
*****************************************************************/
MATCHER_DEFINE_NODE( Not, held, actual ) {
  return !converting_operator_equal( actual, held );
};

GENERIC_SINGLE_ARG_MATCHER( Not );

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
  constexpr size_t N       = decltype( held.first )::value;
  auto&            matcher = held.second;
  return converting_operator_equal( matcher,
                                    std::get<N>( actual ) );
};

template<size_t N, MatchableValue M>
auto TupleElement( M&& to_match ) {
  using child_t = std::pair<std::integral_constant<size_t, N>,
                            std::remove_reference_t<M>>;
  return detail::TupleElementImpl<child_t>(
      child_t{ {}, std::forward<M>( to_match ) } );
}

/****************************************************************
** Key
*****************************************************************/
template<MatchableValue M>
auto Key( M&& to_match ) {
  using child_t = std::pair<std::integral_constant<size_t, 0>,
                            std::remove_reference_t<M>>;
  return detail::TupleElementImpl<child_t>(
      child_t{ {}, std::forward<M>( to_match ) } );
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

template<typename MemberVarT, MatchableValue M>
requires std::is_member_object_pointer_v<MemberVarT>
auto Field( MemberVarT&& member_ptr, M&& to_match ) {
  using child_t = std::pair<std::remove_reference_t<MemberVarT>,
                            std::remove_reference_t<M>>;
  return detail::FieldImpl<child_t>(
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

template<typename MemberFnT, MatchableValue M>
requires std::is_member_function_pointer_v<MemberFnT>
auto Property( MemberFnT&& member_ptr, M&& to_match ) {
  using child_t = std::pair<std::remove_reference_t<MemberFnT>,
                            std::remove_reference_t<M>>;
  return detail::PropertyImpl<child_t>(
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
