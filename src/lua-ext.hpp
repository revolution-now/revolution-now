/****************************************************************
**lua-ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-19.
*
* Description: Sol2 Lua type customizations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "id.hpp"
#include "lua.hpp"
#include "typed-int.hpp"

LUA_TYPED_INT( ::rn::X );
LUA_TYPED_INT( ::rn::Y );
LUA_TYPED_INT( ::rn::W );
LUA_TYPED_INT( ::rn::H );

LUA_TYPED_INT( ::rn::UnitId );
