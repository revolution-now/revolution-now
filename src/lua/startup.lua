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
local M = {}

local function location( coord )
  local start = { x=19, y=9 }
  return Coord{ x=coord.x + start.x, y=coord.y + start.y }
end

local function create_some_roads()
  local tiles = {
    location{ x=1, y=1 }, --
    location{ x=2, y=1 }, --
    location{ x=2, y=2 }, --
    location{ x=2, y=2 }, --
    location{ x=3, y=2 }, --
    location{ x=1, y=4 }, --
    location{ x=2, y=6 }, --
    location{ x=2, y=7 }, --
    location{ x=2, y=8 }, --
    location{ x=2, y=9 } --
  }
  for _, tile in ipairs( tiles ) do road.set_road( tile ) end
end

local function create_some_plows()
  local tiles = {
    location{ x=1, y=3 }, --
    location{ x=2, y=2 }, --
    location{ x=2, y=3 }, --
    location{ x=2, y=5 }, --
    location{ x=3, y=5 } --
  }
  for _, tile in ipairs( tiles ) do plow.plow_square( tile ) end
end

local function unit_type( type, base_type )
  if base_type == nil then
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create( type ) )
  else
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create_with_base( type, base_type ) )
  end
end

local function create_some_units_in_old_world()
  -- Dutch ------------------------------------------------------
  local nation = e.nation.dutch
  local units = {
    unit_type( e.unit_type.free_colonist ),
    unit_type( e.unit_type.soldier ),
    unit_type( e.unit_type.merchantman ),
    unit_type( e.unit_type.privateer )
  }
  for _, u in ipairs( units ) do
    old_world.create_unit_in_port( nation, u )
  end

  local id = old_world.create_unit_in_port( nation, unit_type(
                                                e.unit_type
                                                    .merchantman ) )
  old_world.unit_sail_to_new_world( id )
  old_world.advance_unit_on_high_seas( id )
end

local function create_some_units_on_land( nation2 )
  -- Dutch ------------------------------------------------------
  local nation = e.nation.dutch
  local coord = location{ y=6, x=2 }
  local dragoon = unit_type( e.unit_type.dragoon,
                             e.unit_type.petty_criminal )
  local soldier = unit_type( e.unit_type.soldier )
  local veteran_dragoon =
      unit_type( e.unit_type.veteran_dragoon )
  local pioneer = unit_type( e.unit_type.pioneer )
  local treasure = unit_type( e.unit_type.large_treasure )
  local unit =
      ustate.create_unit_on_map( nation, dragoon, coord )
  -- unit:fortify();

  coord = location{ y=6, x=3 }
  unit = ustate.create_unit_on_map( nation, soldier, coord )
  -- unit:sentry();

  coord = location{ y=6, x=6 }
  ustate.create_unit_on_map( nation,
                             unit_type( e.unit_type.privateer ),
                             coord )
  coord = location{ y=5, x=6 }
  ustate.create_unit_on_map( nation,
                             unit_type( e.unit_type.privateer ),
                             coord )

  coord = location{ y=2, x=4 }
  ustate.create_unit_on_map( nation,
                             unit_type( e.unit_type.dragoon ),
                             coord )

  coord = location{ y=6, x=4 }
  ustate.create_unit_on_map( nation, pioneer, coord )

  coord = location{ y=7, x=4 }
  ustate.create_unit_on_map( nation, treasure, coord )

  -- French -----------------------------------------------------
  local nation = e.nation.french
  coord = location{ y=7, x=2 }
  unit = ustate.create_unit_on_map( nation, soldier, coord )
  coord = location{ y=8, x=2 }
  unit = ustate.create_unit_on_map( nation, veteran_dragoon,
                                    coord )
  -- unit:fortify();

  coord = location{ y=7, x=3 }
  unit = ustate.create_unit_on_map( nation, soldier, coord )
  coord = location{ y=8, x=3 }
  unit = ustate.create_unit_on_map( nation, dragoon, coord )
  -- unit:sentry();

  coord = location{ y=7, x=6 }
  unit = ustate.create_unit_on_map( nation, unit_type(
                                        e.unit_type.privateer ),
                                    coord )
  coord = location{ y=8, x=6 }
  unit = ustate.create_unit_on_map( nation, unit_type(
                                        e.unit_type.privateer ),
                                    coord )
end

local function create_some_colonies()
  -- Dutch ------------------------------------------------------
  local nation = e.nation.dutch
  local coord = location{ y=2, x=4 }
  local unit = ustate.create_unit_on_map( nation, unit_type(
                                              e.unit_type
                                                  .free_colonist ),
                                          coord )
  local col_id =
      colony_mgr.found_colony( unit:id(), 'New London' )
  local colony = cstate.colony_from_id( col_id )
  colony:set_commodity_quantity( e.commodity.lumber, 100 )
  colony:set_commodity_quantity( e.commodity.tools, 45 )
  colony:set_commodity_quantity( e.commodity.muskets, 100 )
  colony:set_commodity_quantity( e.commodity.horses, 100 )

  -- French -----------------------------------------------------
  local nation = e.nation.french
  coord = location{ y=4, x=4 }
  unit = ustate.create_unit_on_map( nation, unit_type(
                                        e.unit_type.free_colonist ),
                                    coord )
  col_id = colony_mgr.found_colony( unit:id(), 'New York' )
  local colony = cstate.colony_from_id( col_id )
  colony:add_building( e.colony_building.stockade )
  colony:set_commodity_quantity( e.commodity.cloth, 93 )
  colony:set_commodity_quantity( e.commodity.tools, 120 )
  colony:set_commodity_quantity( e.commodity.muskets, 100 )
  colony:set_commodity_quantity( e.commodity.horses, 100 )
end

local ARC = e.ground_terrain.arctic
local DST = e.ground_terrain.desert
local GRA = e.ground_terrain.grassland
local MSH = e.ground_terrain.marsh
local PLN = e.ground_terrain.plains
local PRA = e.ground_terrain.prairie
local SAV = e.ground_terrain.savannah
local SWA = e.ground_terrain.swamp
local TUNDRA = e.ground_terrain.tundra

local HLL = e.land_overlay.hills
local MTN = e.land_overlay.mountains
local FOR = e.land_overlay.forest

local ___ = 'none'

local ground_tiles = {
  { SWA, ARC, PRA, ARC, ___, ___, GRA, ARC, PLN, ARC },
  { SWA, SWA, PRA, PLN, ___, ___, GRA, DST, PLN, MSH },
  { SWA, SAV, PRA, PRA, ___, ___, DST, DST, PLN, MSH },
  { MSH, SWA, PLN, PLN, ___, ___, ARC, MSH, MSG, PLN },
  { MSH, SWA, SAV, PRA, ___, ___, PRA, GRA, DST, PLN },
  { SAV, PRA, PRA, ARC, ___, ___, PRA, MSG, MSH, SWA },
  { SAV, PLN, PLN, ARC, ___, ___, DST, PLN, PLN, SWA },
  { SWA, GRA, PRA, MSH, ___, ___, PRA, DST, SAV, PLN },
  { SWA, GRA, GRA, MSH, ___, ___, DST, DST, SAV, PLN },
  { ARC, SAV, ARC, PLN, ___, ___, ARC, DST, ARC, MSH }
}

local overlay_tiles = {
  { ___, ___, ___, ___, ___, ___, ___, ___, ___, ___ },
  { ___, ___, MTN, ___, ___, ___, ___, ___, ___, ___ },
  { ___, ___, MTN, ___, ___, ___, FOR, FOR, FOR, ___ },
  { ___, MTN, FOR, FOR, ___, ___, ___, FOR, ___, ___ },
  { ___, ___, ___, FOR, ___, ___, ___, ___, ___, ___ },
  { FOR, ___, ___, ___, ___, ___, ___, HLL, ___, ___ },
  { ___, ___, ___, ___, ___, ___, ___, HLL, MTN, ___ },
  { ___, HLL, HLL, HLL, ___, ___, ___, ___, ___, ___ },
  { ___, ___, ___, ___, ___, ___, ___, ___, ___, ___ },
  { ___, FOR, ___, ___, ___, ___, ___, MTN, MTN, MTN }
}

local function set_ground_tiles()
  for y, row in ipairs( ground_tiles ) do
    for x, terrain in ipairs( row ) do
      if terrain ~= 'none' then
        local c = Coord{ x=x, y=y }
        local square = world_map.at( location( c ) )
        square.ground = terrain
      end
    end
  end
end

local function set_overlay_tiles()
  for y, row in ipairs( overlay_tiles ) do
    for x, overlay in ipairs( row ) do
      if terrain ~= 'none' then
        local c = Coord{ x=x, y=y }
        local square = world_map.at( location( c ) )
        square.overlay = overlay
      end
    end
  end
end

local function set_terrain_tiles()
  set_ground_tiles()
  set_overlay_tiles()
end

function M.main()
  world_map.generate_terrain()
  player.set_players( {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  } )

  create_some_units_in_old_world()
  create_some_units_on_land()
  create_some_colonies()
  create_some_roads()
  create_some_plows()
  set_terrain_tiles()
  land_view.center_on_tile{ x=23, y=15 }
end

return M
