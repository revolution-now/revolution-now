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
  local unit = ustate.unit_from_id( blinker )
  assert( not unit:desc().ship,
          'Cannot convert a naval unit to a soldier.' )
  unit:change_type( unit_composer.UnitComposition
                        .create_with_type_obj(
                        utype.UnitType.create( 'soldier' ) ) )
end

-- Convert all land units that are directly on land in the upper
-- left block to the target unit type. If the `nation` parameter
-- is nil then it will do so for all nations.
function convert_units( unit_type, nation )
  for y = 0, 20 do
    for x = 0, 20 do
      local ids = ustate.units_from_coord( Coord{ x=x, y=y } )
      for _, id in ipairs( ids ) do
        local unit = ustate.unit_from_id( id )
        local correct_nation = (nation == nil) or
                                   (nation == unit:nation())
        if not unit:desc().ship and unit:desc().type ~= unit_type and
            correct_nation then
          log.debug( 'changing unit ' .. id .. '.' )
          unit:change_type( unit_composer.UnitComposition
                                .create_with_type_obj(
                                utype.UnitType
                                    .create( unit_type ) ) )
        end
      end
    end
  end
end

return M