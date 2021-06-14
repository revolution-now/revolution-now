/****************************************************************
**helper.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua helper object.
*
*****************************************************************/
#include "helper.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"

// Lua
#include "lauxlib.h"

using namespace std;

namespace lua {

namespace {} // namespace

/****************************************************************
** helper
*****************************************************************/
helper::helper( cthread L ) : C( c_api( L ) ) {}

} // namespace lua
