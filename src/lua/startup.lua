-----------------------------------------------------------------
-- startup.lua
--
-- Project: Revolution Now
--
-- Created by dsicilia on 2019-09-17.
--
-- Description: Code to be run at startup.
--
-----------------------------------------------------------------

local function create_some_units_in_europort( nation )
  local units = {
    e.unit_type.free_colonist,
    e.unit_type.soldier,
    e.unit_type.merchantman,
    e.unit_type.privateer,
  }
  for _, u in ipairs( units ) do
    europort.create_unit_in_port( nation, u )
  end

  local id = europort.create_unit_in_port( nation,
                                           e.unit_type.merchantman )
  europort.unit_sail_to_new_world( id )
  europort.advance_unit_on_high_seas( id )
end

local function create_some_units_on_land( nation )
  local coord = Coord{y=6, x=2}
  local unit = ustate.create_unit_on_map(
           e.nation.spanish, e.unit_type.free_colonist, coord )
  unit:fortify();

  coord = Coord{y=6, x=3}
  unit = ustate.create_unit_on_map(
           e.nation.spanish, e.unit_type.soldier, coord )
  unit:sentry();

  coord = Coord{y=6, x=6}
  ustate.create_unit_on_map(
      e.nation.spanish, e.unit_type.privateer, coord )

  coord = Coord{y=7, x=2}
  unit = ustate.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  unit:fortify();

  coord = Coord{y=7, x=3}
  unit = ustate.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  unit:sentry();

  coord = Coord{y=7, x=6}
  unit = ustate.create_unit_on_map(
           e.nation.english, e.unit_type.privateer, coord )
  unit:clear_orders();
end

local function main()
  terrain.generate_terrain()
  player.set_players( {
    e.nation.dutch,
    e.nation.spanish,
    e.nation.english,
    e.nation.french
  } )
  create_some_units_in_europort( e.nation.dutch )
  create_some_units_on_land()
end

package_exports = {
  main = main
}
