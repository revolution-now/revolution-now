/****************************************************************
**lua-root-0.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-12.
*
* Description: Exposes the entire ss structure to Lua.
*
*****************************************************************/
// ss
#include "events.rds.hpp"
#include "meta.rds.hpp"
#include "settings.rds.hpp"
#include "version.rds.hpp"

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

// This recursively traverses the the given data structure in
// the sense that it does a full type traversal and visits every
// type mentioned in every field and sub field and registers for
// its define_usertype_for method to be called when registration
// methods are called.
//
// It is done in this module and so that nothing needs to be ex-
// posed in header files. The exception to that is that if a
// module wants to e.g. define a Lua function that needs to
// push/pop type X then the header declaring type X will need to
// expose its Lua type traits. But most will not have to.
//
// NOTE: in theory we could just run this once on the top-level
// RootState as below, but that leads to a translation unit that
// takes too long to compile, so it is split up across files.
//
//   RUN_RECURSIVE_LUA_BINDER( ::rn::RootState );  // too big.
//
RUN_RECURSIVE_LUA_BINDER( ::rn::FormatVersion );
RUN_RECURSIVE_LUA_BINDER( ::rn::MetaState );
RUN_RECURSIVE_LUA_BINDER( ::rn::SettingsState );
RUN_RECURSIVE_LUA_BINDER( ::rn::EventsState );

} // namespace rn

/****************************************************************
** Linker.
*****************************************************************/
namespace rn {
void linker_dont_discard_module_ss_lua_root_0();
void linker_dont_discard_module_ss_lua_root_0() {}
} // namespace rn
