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

MATCHER_DEFINE_NODE( Any, lhs [[maybe_unused]],
                     rhs [[maybe_unused]] ) {
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

MATCHER_DEFINE_NODE( Pointee, lhs, rhs ) {
  return converting_operator_equal( lhs, *rhs );
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

MATCHER_DEFINE_NODE( IterableElementsAre, lhs, rhs ) {
  bool stop = false;
  auto it   = std::begin( rhs );
  FOR_CONSTEXPR_IDX( Idx, std::tuple_size_v<T> ) {
    stop = ( it == std::end( rhs ) ) ||
           !converting_operator_equal( std::get<Idx>( lhs ),
                                       *it++ );
    return stop;
  };
  return ( it == std::end( rhs ) ) && !stop;
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
  return converting_operator_ge( actual, held );
};

} // namespace detail

template<MatchableValue T>
auto Ge( T&& arg ) {
  return detail::GeImpl<std::remove_cvref_t<T>>(
      std::forward<T>( arg ) );
}

} // namespace mock::matchers
