--[[ ------------------------------------------------------------
|
| startup.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-17.
|
| Description: Code to be run at startup.
|
--]] ------------------------------------------------------------
local function create_some_units_in_old_world( nation )
  local units = {
    e.unit_type.free_colonist, e.unit_type.soldier,
    e.unit_type.merchantman, e.unit_type.privateer
  }
  for _, u in ipairs( units ) do
    old_world.create_unit_in_port( nation, u )
  end

  local id = old_world.create_unit_in_port( nation, e.unit_type
                                                .merchantman )
  old_world.unit_sail_to_new_world( id )
  old_world.advance_unit_on_high_seas( id )
end

local function create_some_units_on_land( nation1, nation2 )
  local coord = Coord{ y=6, x=2 }
  local unit = ustate.create_unit_on_map( nation1, e.unit_type
                                              .free_colonist,
                                          coord )
  unit:fortify();

  coord = Coord{ y=6, x=3 }
  unit = ustate.create_unit_on_map( nation1, e.unit_type.soldier,
                                    coord )
  unit:sentry();

  coord = Coord{ y=6, x=6 }
  ustate.create_unit_on_map( nation1, e.unit_type.privateer,
                             coord )

  coord = Coord{ y=7, x=2 }
  unit = ustate.create_unit_on_map( nation2, e.unit_type.soldier,
                                    coord )
  unit:fortify();

  coord = Coord{ y=7, x=3 }
  unit = ustate.create_unit_on_map( nation2, e.unit_type.soldier,
                                    coord )
  unit:sentry();

  coord = Coord{ y=7, x=6 }
  unit = ustate.create_unit_on_map( nation2,
                                    e.unit_type.privateer, coord )
  unit:clear_orders();
end

local function create_some_colonies( nation1, nation2 )
  local coord = Coord{ y=2, x=4 }
  local unit = ustate.create_unit_on_map( nation1, e.unit_type
                                              .free_colonist,
                                          coord )
  local col_id =
      colony_mgr.found_colony( unit:id(), 'New London' )
  local colony = cstate.colony_from_id( col_id )
  colony:set_commodity_quantity( e.commodity.lumber, 100 )

  coord = Coord{ y=4, x=4 }
  unit = ustate.create_unit_on_map( nation2,
                                    e.unit_type.free_colonist,
                                    coord )
  col_id = colony_mgr.found_colony( unit:id(), 'New York' )
  local colony = cstate.colony_from_id( col_id )
  colony:add_building( e.colony_building.stockade )
  colony:set_commodity_quantity( e.commodity.cloth, 93 )
end

local function main()
  terrain.generate_terrain()
  player.set_players( {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  } )

  nation1 = e.nation.dutch
  nation2 = e.nation.french

  create_some_units_in_old_world( nation1 )
  create_some_units_on_land( nation1, nation2 )
  create_some_colonies( nation1, nation2 )
end

package_exports = { main=main }
