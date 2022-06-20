--[[ ------------------------------------------------------------
|
| new-game.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-22.
|
| Description: Creates a new game.
|
--]] ------------------------------------------------------------
local M = {}

local map_gen = require( 'map-gen' )

-----------------------------------------------------------------
-- Options
-----------------------------------------------------------------
local DIFFICULTY_NAMES = {
  ['discoverer']=0,
  ['explorer']=1,
  ['conquistador']=2,
  ['governor']=3,
  ['viceroy']=4
}

function M.default_options()
  return {
    difficulty_name='discoverer',
    -- This determines which nations are enabled and some proper-
    -- ties.
    nations={
      [e.nation.english]={ human=false },
      [e.nation.french]={ human=false },
      [e.nation.spanish]={ human=false },
      [e.nation.dutch]={ human=true }
    },
    map={} -- use default map options.
  }
end

-----------------------------------------------------------------
-- Settings
-----------------------------------------------------------------
local function set_default_settings( options, settings )
  local difficulty_int =
      DIFFICULTY_NAMES[options.difficulty_name]
  settings.difficulty = assert( difficulty_int )
  settings.fast_piece_slide = true
end

-----------------------------------------------------------------
-- Units
-----------------------------------------------------------------
local function unit_type( type, base_type )
  if base_type == nil then
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create( type ) )
  else
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create_with_base( type, base_type ) )
  end
end

local function create_initial_units_for_nation( nation, root )
  local player = root.players.players:get( nation )
  local coord = map_gen.initial_ships_pos()[nation]
  if not coord then return { x=0, y=0 } end
  local merchantman = unit_type( e.unit_type.merchantman )
  local soldier = unit_type( e.unit_type.soldier )
  local pioneer = unit_type( e.unit_type.pioneer )

  local merchantman_unit = ustate.create_unit_on_map( nation,
                                                      merchantman,
                                                      coord )
  ustate.create_unit_in_cargo( nation, soldier,
                               merchantman_unit:id() )
  ustate.create_unit_in_cargo( nation, pioneer,
                               merchantman_unit:id() )

  player.starting_position = coord
end

local function create_initial_units( options, root )
  for nation, _ in pairs( options.nations ) do
    create_initial_units_for_nation( nation, root )
  end
end

local function create_battlefield_units( options, root )
  local nation1
  local nation2
  for nation, _ in pairs( options.nations ) do
    nation1 = nation2
    nation2 = nation
    if nation1 then break end
  end
  assert( nation1 )
  assert( nation2 )
  local veteran_dragoon =
      unit_type( e.unit_type.veteran_dragoon )

  ustate.create_unit_on_map( nation1, veteran_dragoon,
                             { x=1, y=1 } ):fortify()
  ustate.create_unit_on_map( nation1, veteran_dragoon,
                             { x=1, y=2 } ):fortify()
  ustate.create_unit_on_map( nation2, veteran_dragoon,
                             { x=2, y=1 } )
  ustate.create_unit_on_map( nation2, veteran_dragoon,
                             { x=2, y=2 } )
end

-----------------------------------------------------------------
-- Players State
-----------------------------------------------------------------
local function create_old_world_state( settings, player )
  local old_world = player.old_world
  -- Immigrants state.
  old_world.immigration.next_recruit_cost_base = 50
  old_world.immigration.immigrants_pool[1] =
      immigration.pick_next_unit_for_pool( player, settings )
  old_world.immigration.immigrants_pool[2] =
      immigration.pick_next_unit_for_pool( player, settings )
  old_world.immigration.immigrants_pool[3] =
      immigration.pick_next_unit_for_pool( player, settings )

  -- Tax rate.
  old_world.taxes.tax_rate = 7

  -- Market state.
  local cotton_item = old_world.market.commodities[e.commodity
                          .cotton]
  cotton_item.sell_price_in_hundreds = 4
  cotton_item.boycott = true
  cotton_item.price_movement = .33

  local muskets_item = old_world.market.commodities[e.commodity
                           .muskets]
  muskets_item.sell_price_in_hundreds = 5
  muskets_item.boycott = false
  muskets_item.price_movement = .44

  -- Expeditionary force.
  old_world.expeditionary_force.regulars = 3
  old_world.expeditionary_force.cavalry = 2
  old_world.expeditionary_force.artillery = 2
  old_world.expeditionary_force.men_of_war = 3
end

local function create_player_state(settings, nation, player,
                                   is_human )
  player.nation = nation
  player.human = is_human
  player.money = 1000 - 250 * settings.difficulty
  -- This is temporary so that it doesn't keep asking us.
  player.discovered_new_world = 'New Netherlands'
  create_old_world_state( settings, player )
end

local function create_nations( options, root )
  local players = root.players.players
  local settings = root.settings
  for nation, tbl in pairs( options.nations ) do
    local player = players:reset_player( nation )
    create_player_state( settings, nation, player, tbl.human )
  end
end

-----------------------------------------------------------------
-- Turn State
-----------------------------------------------------------------
local function create_turn_state( turns_state )
  turns_state.time_point.year = 1492
  turns_state.time_point.season = e.season.spring
end

-----------------------------------------------------------------
-- Testing
-----------------------------------------------------------------
local function add_testing_options( options )
  -- options.nations = {
  --   [e.nation.french]={ human=true },
  --   [e.nation.dutch]={ human=true }
  -- }
  -- options.map.world_size = { w=4, h=4 }
  -- options.map.type = 'battlefield'

  options.map.world_size = { w=16, h=16 }
end

-----------------------------------------------------------------
-- Creates a new Game
-----------------------------------------------------------------
-- This should be called af the player decides the parameters of
-- the game, which should be passed in as options here. Also, the
-- save-game state should be default-constructed before calling
-- this.
function M.create( options )
  options = options or {}
  -- Merge the options with the default ones so that any missing
  -- fields will have their default values.
  for k, v in pairs( M.default_options() ) do
    if options[k] == nil then options[k] = v end
  end

  add_testing_options( options )

  local root = ROOT_STATE

  set_default_settings( options, root.settings )

  map_gen.generate( options.map )

  create_turn_state( root.turn )

  create_nations( options, root )

  if options.map.type == 'battlefield' then
    create_battlefield_units( options, root )
  else
    create_initial_units( options, root )
  end
end

return M
