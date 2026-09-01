/****************************************************************
**lua-root-9.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-12.
*
* Description: Exposes part of the ss structure to Lua.
*
*****************************************************************/
// ss
#include "map.rds.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/ext-refl.hpp"
#include "luapp/ext-std.hpp"
#include "luapp/recursive-binder.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext-type-traverse.hpp"

// traverse
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"
#include "traverse/type-ext-base.hpp"
#include "traverse/type-ext-std.hpp"
#include "traverse/type-ext.hpp"

using namespace std;

namespace rn {

// NOTE: see the comment in lua-root-0.cpp for more info.
RUN_RECURSIVE_LUA_BINDER( ::rn::MapState );

} // namespace rn

/****************************************************************
** Linker.
*****************************************************************/
namespace rn {
void linker_dont_discard_module_ss_lua_root_9();
void linker_dont_discard_module_ss_lua_root_9() {}
} // namespace rn
