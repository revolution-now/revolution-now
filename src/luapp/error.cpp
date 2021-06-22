/****************************************************************
**error.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: [FILL ME IN]
*
*****************************************************************/
#include "error.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"
#include "base/macros.hpp"

using namespace std;

namespace lua {

namespace detail {

// TODO: add test for this.
void throw_lua_error_impl( cthread L, string_view msg ) {
  c_api C( L );
  C.push( msg );
  C.error();
}

} // namespace detail

} // namespace lua
