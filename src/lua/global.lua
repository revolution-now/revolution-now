--[[ ------------------------------------------------------------
|
| global.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-04-09.
|
| Description: Globals.
|
--]] ------------------------------------------------------------
-- Global function
function grid() terrain.toggle_grid() end

-- Take the currently blinking unit and turn it into a soldier.
function soldier()
  local blinker = assert( land_view.blinking_unit(),
                          'There is currently no unit asking ' ..
                              'for orders.' )
  local unit = ustate.unit_from_id( blinker )
  assert( not unit:desc().ship,
          'Cannot convert a naval unit to a soldier.' )
  unit:change_type( e.unit_type.soldier )
end

-- This module does not export anything since its purpose is to
-- create globals.
package_exports = {}