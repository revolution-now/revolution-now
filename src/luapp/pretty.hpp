/****************************************************************
**pretty.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Pretty-printer for Lua data structures.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct any;

/****************************************************************
** Public API.
*****************************************************************/
// Pretty-print a Lua data structure.  Will emit something like
// this:
//
//   {
//     a_str = "888",
//     an_int = 10,
//     foo = {
//       bar = {
//         baz = "hello",
//         some_key = false,
//       },
//       some_list = {
//         [1] = 8,
//         [2] = "world",
//         [3] = true,
//       },
//     },
//     my_key = 5.4,
//   }
//
// Non-printable things such as threads and functions will just
// have their addresses printed.
std::string pretty_print( any const& a );

} // namespace lua
