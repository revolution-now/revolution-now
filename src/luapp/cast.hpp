/****************************************************************
**cast.hpp
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
concept Castable =
    Pushable<From> && Gettable<To> &&
    CompatibleNvalues<From, To> &&
    // If it's already assignable then no need to cast.
    !std::is_assignable_v<To, From> &&
    !std::is_convertible_v<From, To>;

namespace detail {

void        cast_pop( cthread L, int n );
std::string cast_type_name( cthread L, int idx );

// If this is ever taken out of the detail namespace then it
// needs to be independently unit tested.
template<typename To, typename From>
requires Castable<From, To>
[[nodiscard]] To cast(
    cthread L, From&& from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  int n_pushed = lua::push( L, from );
  To  to       = get_or_luaerr<To>( L, -1, loc );
  detail::cast_pop( L, n_pushed );
  return to;
}

} // namespace detail

template<typename To, typename From>
requires Castable<From, To> && HasCthread<From>
[[nodiscard]] To cast(
    From&&          from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  cthread L = from.this_cthread();
  return detail::cast<To>( L, FWD( from ), loc );
}

// Wraps the type in a maybe<...> so that it won't throw errors
// if the conversion does not succeed (maybe has a type_traits
// specialization that makes this happen).
template<typename To, typename From>
requires Castable<From, To>
[[nodiscard]] auto safe_cast(
    From&&          from,
    base::SourceLoc loc = base::SourceLoc::current() ) {
  return cast<base::maybe<To>>( FWD( from ), loc );
}

} // namespace lua
