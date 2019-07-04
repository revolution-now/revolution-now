/****************************************************************
**ranges.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-12.
*
* Description: Utilities for ranges.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "fmt-helper.hpp"

// range-v3
#include "range/v3/numeric/accumulate.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/drop_while.hpp"
#include "range/v3/view/intersperse.hpp"
#include "range/v3/view/take.hpp"
#include "range/v3/view/take_while.hpp"
#include "range/v3/view/transform.hpp"
#include "range/v3/view/zip.hpp"

// C++ standard library
#include <utility>

namespace rn {

// This behaves just like the take_while function except that it
// will also include the element that stops the "taking," if
// there is one.
template<typename Func>
auto take_while_inclusive( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using Rng = decltype( r );
    return rv::concat( std::forward<Rng>( r ) //
                           | rv::take_while( f ),
                       std::forward<Rng>( r )    //
                           | rv::drop_while( f ) //
                           | rv::take( 1 ) );
  } );
}

template<typename Rng>
std::string rng_to_string( Rng&& rng ) {
  return rg::accumulate(
      std::forward<Rng>( rng )                           //
          | rv::transform( L( fmt::format( "{}", _ ) ) ) //
          | rv::intersperse( std::string( "," ) ),
      std::string{} );
}

// Given a function f and a range [1,2,3...] this will return a
// range of: [(1,f(1)), (2,f(2)), (3,f(3)), ...]
template<typename Func>
auto transform_pair( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using Rng = decltype( r );
    return rv::zip(
        std::forward<Rng>( r ),
        std::forward<Rng>( r ) | rv::transform( f ) );
  } );
}

// This function applies `f` to each element in the range to
// derive keys, then finds the minimum key, and returns the range
// value corresponding to that key.
template<typename Func>
auto min_by_key( Func f ) {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using ResType = decltype( *r.begin() );
    using KeyType = decltype( f( *r.begin() ) );
    using Rng     = decltype( r );
    std::optional<ResType> res{};
    std::optional<KeyType> min_key{};
    for( auto const& [elem, key] :
         std::forward<Rng>( r ) | transform_pair( f ) ) {
      if( !min_key.has_value() || key < *min_key ) {
        min_key = key;
        res     = elem;
      }
    }
    return res;
  } );
}

// Returns the maximum of a range, by value, if one exists. NOTE:
// unfortunately, due to a strange issue where clang doesn't
// agree with libstdc++'s implementation of std::optional we
// return a std::pair here with the first element indicating the
// presence of the return value (we would have liked to use
// std::optional). See:
// https://stackoverflow.com/questions/51379597/
//   should-you-be-able-move-from-stdoptionalt-where-t-has-non-trivial-constructo
inline auto maximum() {
  return ranges::make_pipeable( [=]( auto&& r ) {
    using ResType = decltype( *r.begin() );
    using Rng     = decltype( r );
    std::pair<bool, ResType> res( false, {} );
    for( auto const& elem : std::forward<Rng>( r ) )
      if( !res.first || res.second < elem ) res = {true, elem};
    return res;
  } );
}

// Returns the head value by value if there is one. This may not
// be efficient in some cases due to the copy, but returning a
// reference would seem kind of hard to reason about in terms of
// lifetimes, so this should at least be safe.
//
// Rng must be taken by reference in case it is e.g. a vector, we
// don't want to copy the vector.
template<typename Rng>
auto head( Rng&& r )
    -> Opt<std::decay_t<decltype( *r.begin() )>> {
  for( auto const& e : r ) return e;
  return std::nullopt;
}

} // namespace rn
