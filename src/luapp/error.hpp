/****************************************************************
**error.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Types for error handling in the luapp library.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/valid.hpp"

// C++ standard library
#include <string_view>

namespace lua {

/****************************************************************
** expect/valid
*****************************************************************/
// The type we use for reporting errors raised by lua.
using lua_error_t = std::string;
// We can change the error type in the future, but it must at
// least be constructible from a std::string.
static_assert(
    std::is_constructible_v<lua_error_t, std::string> );

// valid_or
using lua_valid = base::valid_or<lua_error_t>;
lua_valid lua_invalid( lua_error_t err );

// expect
template<typename T>
using lua_expect = base::expect<T, lua_error_t>;

template<typename T>
lua_expect<T> lua_expected( T&& arg ) {
  return base::expected<lua_error_t>( std::forward<T>( arg ) );
}

template<typename T, typename Arg>
lua_expect<T> lua_unexpected( Arg&& arg ) {
  return base::unexpected<T, lua_error_t>(
      std::forward<Arg>( arg ) );
}

/****************************************************************
** Throwing errors
*****************************************************************/
namespace detail {

[[noreturn]] void throw_lua_error_impl( cthread          L,
                                        std::string_view msg );

}

template<typename... Args>
[[noreturn]] void throw_lua_error( cthread          L,
                                   std::string_view fmt_str,
                                   Args&&... args ) {
  detail::throw_lua_error_impl(
      L, fmt::format( fmt_str, std::forward<Args>( args )... ) );
}

} // namespace lua
