--[[-------------------------------------------------------------
|
| ustate.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2020-03-10.
|
| Description: Lua utilities for the ustate module.
|
--]]-------------------------------------------------------------

local function change_to_soldier( unit_id )
  ustate.unit_from_id( unit_id ):change_type( e.unit_type.soldier )
end

package_exports = {
  change_to_soldier = change_to_soldier
}
