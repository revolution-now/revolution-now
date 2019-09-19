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
    e.unit_type.small_treasure,
    e.unit_type.privateer,
    e.unit_type.large_treasure
  }
  for _, u in ipairs( units ) do
    europort.create_unit_in_port( nation, u )
  end
end

local function create_some_units_on_land( nation )
  coord = Coord{y=2, x=2}
  id = ownership.create_unit_on_map(
           e.nation.spanish, e.unit_type.free_colonist, coord )
  --ownership.unit_from_id( id ).fortify();

  coord = Coord{y=2, x=3}
  id = ownership.create_unit_on_map(
           e.nation.spanish, e.unit_type.soldier, coord )
  --ownership.unit_from_id( id ).sentry();

  coord = Coord{y=2, x=6}
  ownership.create_unit_on_map(
      e.nation.spanish, e.unit_type.privateer, coord )

  coord = Coord{y=3, x=2}
  id = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  --ownership.unit_from_id( id ).fortify();

  coord = Coord{y=3, x=3}
  id = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  --ownership.unit_from_id( id ).sentry();

  coord = Coord{y=3, x=6}
  id = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.privateer, coord )
  --ownership.unit_from_id( id ).clear_orders();
end

local function run()
  create_some_units_in_europort( e.nation.dutch )
  create_some_units_on_land()
end

package_exports = {
  run = run
}
