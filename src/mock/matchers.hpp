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

namespace mock::matchers {

/****************************************************************
** Any
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Any, /*held*/, /*actual*/ ) {
  return true;
};

}

template<MatchableValue T>
constexpr auto Any( T&& ) {
  return detail::AnyImpl<std::remove_cvref_t<T>>( 0 );
}

inline constexpr auto _ = Any( 0 );

/****************************************************************
** Pointee
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Pointee, held, actual ) {
  return converting_operator_equal( held, *actual );
};

} // namespace detail

template<MatchableValue T>
auto Pointee( T&& arg ) {
  return detail::PointeeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** IterableElementsAre
*****************************************************************/
namespace detail {

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

} // namespace detail

template<MatchableValue... M>
auto IterableElementsAre( M&&... to_match ) {
  using child_t = std::tuple<std::remove_reference_t<M>...>;
  return detail::IterableElementsAreImpl<child_t>(
      child_t{ std::forward<M>( to_match )... } );
}

/****************************************************************
** Ge
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Ge, held, actual ) {
  return converting_operator_greater( actual, held ) ||
         converting_operator_equal( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Ge( T&& arg ) {
  return detail::GeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Le
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Le, held, actual ) {
  return !converting_operator_greater( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Le( T&& arg ) {
  return detail::LeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Gt
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Gt, held, actual ) {
  return converting_operator_greater( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Gt( T&& arg ) {
  return detail::GtImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Lt
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Lt, held, actual ) {
  return !converting_operator_equal( actual, held ) &&
         !converting_operator_greater( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Lt( T&& arg ) {
  return detail::LtImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Ne
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Ne, held, actual ) {
  return !converting_operator_equal( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Ne( T&& arg ) {
  return detail::NeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Not
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Not, held, actual ) {
  return !converting_operator_equal( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Not( T&& arg ) {
  return detail::NotImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** StartsWith
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( StartsWith, held, actual ) {
  return std::string_view( actual ).starts_with( held );
};

} // namespace detail

inline auto StartsWith( std::string arg ) {
  return detail::StartsWithImpl<std::string>( std::move( arg ) );
}

/****************************************************************
** StrContains
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( StrContains, held, actual ) {
  return std::string_view( actual ).find( held ) !=
         std::string_view::npos;
};

} // namespace detail

inline auto StrContains( std::string arg ) {
  return detail::StrContainsImpl<std::string>(
      std::move( arg ) );
}

/****************************************************************
** Empty
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Empty, /*held*/, actual ) {
  return actual.empty();
};

} // namespace detail

inline auto Empty() {
  struct Unused {
    bool operator==( Unused const& ) const = default;
  };
  return detail::EmptyImpl<Unused>( Unused{} );
}

/****************************************************************
** HasSize
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( HasSize, held, actual ) {
  return converting_operator_equal( held, actual.size() );
};

} // namespace detail

template<MatchableValue T>
auto HasSize( T&& arg ) {
  return detail::HasSizeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** Each
*****************************************************************/
// Matches a container, and requires that each element in the
// container match a given (single) matcher/value.
namespace detail {

MATCHER_DEFINE_NODE( Each, held, actual ) {
  return std::all_of(
      actual.begin(), actual.end(), [&held]( auto const& elem ) {
        return converting_operator_equal( elem, held );
      } );
};

} // namespace detail

template<MatchableValue T>
auto Each( T&& arg ) {
  return detail::EachImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

/****************************************************************
** AllOf
*****************************************************************/
// Matches a value and requires that all supplied matchers match
// that same value.
namespace detail {

MATCHER_DEFINE_NODE( AllOf, held, actual ) {
  bool should_stop = false;
  FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
    should_stop = !converting_operator_equal(
        std::get<Idx>( held ), actual );
    return should_stop;
  };
  return !should_stop;
};

} // namespace detail

template<MatchableValue... M>
auto AllOf( M&&... to_match ) {
  using child_t = std::tuple<std::remove_reference_t<M>...>;
  return detail::AllOfImpl<child_t>(
      child_t{ std::forward<M>( to_match )... } );
}

/****************************************************************
** AnyOf
*****************************************************************/
// Matches a value and requires that all supplied matchers match
// that same value.
namespace detail {

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

} // namespace detail

template<MatchableValue... M>
auto AnyOf( M&&... to_match ) {
  using child_t = std::tuple<std::remove_reference_t<M>...>;
  return detail::AnyOfImpl<child_t>(
      child_t{ std::forward<M>( to_match )... } );
}

/****************************************************************
** TupleElement
*****************************************************************/
// The specified tuple element matches the specified matcher.
// This should work with any tuple-like type that supports
// std::get<N>();
namespace detail {

MATCHER_DEFINE_NODE( TupleElement, held, actual ) {
  constexpr size_t N       = decltype( held.first )::value;
  auto&            matcher = held.second;
  return converting_operator_equal( matcher,
                                    std::get<N>( actual ) );
};

} // namespace detail

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
namespace detail {

MATCHER_DEFINE_NODE( Field, held, actual ) {
  return converting_operator_equal( held.second,
                                    actual.*( held.first ) );
};

} // namespace detail

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
namespace detail {

MATCHER_DEFINE_NODE( Property, held, actual ) {
  return converting_operator_equal(
      held.second, ( actual.*( held.first ) )() );
};

} // namespace detail

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
namespace detail {

MATCHER_DEFINE_NODE( Boolean, held, actual ) {
  return bool( actual ) == held;
};

} // namespace detail

inline auto Boolean( bool b ) {
  return detail::BooleanImpl<bool>( std::move( b ) );
}

/****************************************************************
** True
*****************************************************************/
inline auto True() { return detail::BooleanImpl<bool>( true ); }

/****************************************************************
** False
*****************************************************************/
inline auto False() {
  return detail::BooleanImpl<bool>( false );
}

/****************************************************************
** Null
*****************************************************************/
namespace detail {

MATCHER_DEFINE_NODE( Null, /*held*/, actual ) {
  return actual == nullptr;
};

} // namespace detail

inline auto Null() {
  struct Unused {
    bool operator==( Unused const& ) const = default;
  };
  return detail::NullImpl<Unused>( Unused{} );
}

} // namespace mock::matchers
