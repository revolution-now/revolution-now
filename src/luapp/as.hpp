/****************************************************************
**as.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-19.
*
* Description: Cast from one Lua-aware type to another via Lua.
*
*****************************************************************/
#pragma once

// luapp
#include "error.hpp"
#include "ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/source-loc.hpp"

namespace lua {

template<typename From, typename To>
concept Castable = Pushable<From> && Gettable<To> &&
    CompatibleNvalues<From, To>;

// Needed to prevent weird indentation after concept.
struct FixClangFormat1 {};

namespace detail {

void        as_pop( cthread L, int n );
std::string as_type_name( cthread L, int idx );

} // namespace detail

template<typename To, typename From>
requires Castable<From, To>
[[nodiscard]] To as(
    cthread L, From&& from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  int n_pushed = lua::push( L, FWD( from ) );
  To  to       = get_or_luaerr<To>( L, -1, loc );
  detail::as_pop( L, n_pushed );
  return to;
}

template<typename To, typename From>
requires Castable<From, To> && HasCthread<From>
[[nodiscard]] To as(
    From&&          from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  cthread L = from.this_cthread();
  return as<To>( L, FWD( from ), loc );
}

// Wraps the type in a maybe<...> so that it won't throw errors
// if the conversion does not succeed (maybe has a type_traits
// specialization that makes this happen).
template<typename To, typename From>
requires Castable<From, To>
[[nodiscard]] auto safe_as(
    From&&          from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  return as<base::maybe<To>>( FWD( from ), loc );
}

} // namespace lua
