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
end

local function create_some_units_on_land( nation )
  local coord = Coord{y=6, x=2}
  local unit = ownership.create_unit_on_map(
           e.nation.spanish, e.unit_type.free_colonist, coord )
  unit:fortify();

  coord = Coord{y=6, x=3}
  unit = ownership.create_unit_on_map(
           e.nation.spanish, e.unit_type.soldier, coord )
  unit:sentry();

  coord = Coord{y=6, x=6}
  ownership.create_unit_on_map(
      e.nation.spanish, e.unit_type.privateer, coord )

  coord = Coord{y=7, x=2}
  unit = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  unit:fortify();

  coord = Coord{y=7, x=3}
  unit = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.soldier, coord )
  unit:sentry();

  coord = Coord{y=7, x=6}
  unit = ownership.create_unit_on_map(
           e.nation.english, e.unit_type.privateer, coord )
  unit:clear_orders();
end

local function main()
  create_some_units_in_europort( e.nation.dutch )
  create_some_units_on_land()
end

package_exports = {
  main = main
}
