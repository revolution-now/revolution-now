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
local M = {}

-- Global function
function grid() terrain.toggle_grid() end

-- Take the currently blinking unit and turn it into a soldier.
function soldier()
  local blinker = assert( land_view.blinking_unit(),
                          'There is currently no unit asking ' ..
                              'for orders.' )
  local unit = unit_mgr.unit_from_id( blinker )
  assert( not unit:desc().ship,
          'Cannot convert a naval unit to a soldier.' )
  unit:change_type( unit_composer.UnitComposition
                        .create_with_type_obj(
                        unit_type.UnitType.create( 'soldier' ) ) )
end

return M