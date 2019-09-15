/****************************************************************
**lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Interface to lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

// base-util
#include "base-util/type-map.hpp"

// C++ standard library
#include <string>
#include <variant>

namespace rn {

/****************************************************************
** Run Lua Scripts
*****************************************************************/
namespace detail {
using LuaRetMap = TypeMap<   //
    KV<void, std::monostate> //
    >;
}

template<typename Ret>
expect<Get<detail::LuaRetMap, Ret, Ret>> lua(
    Str const& script );

template<>
expect<std::monostate> lua<void>( Str const& script );

template<>
expect<Str> lua<Str>( Str const& script );

/****************************************************************
** Utilites
*****************************************************************/
Vec<Str> format_lua_error_msg( Str const& msg );

/****************************************************************
** Testing
*****************************************************************/
void test_lua();

} // namespace rn
