--[[-------------------------------------------------------------
|
| populate.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-17.
|
| Description: Functions used during testing to create various
|              kinds of unit setups.
|
--]]-------------------------------------------------------------

local function create_units_in_europort()
  europort.create_unit_in_port( "free_colonist" )
  europort.create_unit_in_port( "soldier" )
  europort.create_unit_in_port( "merchantman" )
  europort.create_unit_in_port( "small_treasure" )
  europort.create_unit_in_port( "privateer" )
  europort.create_unit_in_port( "large_treasure" )
end

package_exports = {
  create_units_in_europort = create_units_in_europort
}
