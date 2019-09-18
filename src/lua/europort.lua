--[[-------------------------------------------------------------
|
| europort.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-17.
|
| Description: Handles the europort.
|
--]]-------------------------------------------------------------

local function create_units_in_europort()
  local units = {
    e.unit_type.free_colonist,
    e.unit_type.soldier,
    e.unit_type.merchantman,
    e.unit_type.small_treasure,
    e.unit_type.privateer,
    e.unit_type.large_treasure
  }
  for _, u in ipairs( units ) do
    europort.create_unit_in_port( e.nation.dutch, u )
  end
end

package_exports = {
  create_units_in_europort = create_units_in_europort
}
