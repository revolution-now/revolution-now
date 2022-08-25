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
function M.default_options()
  return {
    difficulty='discoverer',
    -- This determines which nations are enabled and some proper-
    -- ties.
    nations={
      ['english']={ human=false },
      ['french']={ human=false },
      ['spanish']={ human=false },
      ['dutch']={ human=true }
    },
    map={} -- use default map options.
  }
end

-----------------------------------------------------------------
-- Settings
-----------------------------------------------------------------
local function set_default_settings( options, settings )
  settings.difficulty = options.difficulty
  settings.fast_piece_slide = true
end

-----------------------------------------------------------------
-- Units
-----------------------------------------------------------------
local function build_unit_type( type, base_type )
  if base_type == nil then
    return unit_composer.UnitComposition.create_with_type_obj(
               unit_type.UnitType.create( type ) )
  else
    return unit_composer.UnitComposition.create_with_type_obj(
               unit_type.UnitType.create_with_base( type,
                                                    base_type ) )
  end
end

local function create_initial_units_for_nation( nation, root )
  local player = root.players.players:get( nation )
  local coord = map_gen.initial_ships_pos()[nation]
  local merchantman = build_unit_type( 'merchantman' )
  local soldier = build_unit_type( 'soldier' )
  local pioneer = build_unit_type( 'pioneer' )

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
  local veteran_dragoon = build_unit_type( 'veteran_dragoon' )

  ustate.create_unit_on_map( nation1, veteran_dragoon,
                             { x=1, y=1 } ):fortify()
  ustate.create_unit_on_map( nation1, veteran_dragoon,
                             { x=1, y=2 } ):fortify()
  ustate.create_unit_on_map( nation2, veteran_dragoon,
                             { x=2, y=1 } )
  ustate.create_unit_on_map( nation2, veteran_dragoon,
                             { x=2, y=2 } )
end

-- FIXME: temporary
local function create_all_units( options, root )
  local nation1
  for nation, _ in pairs( options.nations ) do
    nation1 = nation
    if nation1 then break end
  end
  assert( nation1 )

  local size = ROOT.terrain:size()
  local origin = { x=size.w // 2 - 8, y=size.h // 2 - 4 }

  local land_units = {
    'petty_criminal', 'indentured_servant', 'free_colonist',
    'native_convert', 'soldier', 'dragoon', 'pioneer',
    'missionary', 'scout', 'expert_farmer', 'expert_fisherman',
    'expert_sugar_planter', 'expert_tobacco_planter',
    'expert_cotton_planter', 'expert_fur_trapper',
    'expert_lumberjack', 'expert_ore_miner',
    'expert_silver_miner', 'master_carpenter',
    'master_rum_distiller', 'master_tobacconist',
    'master_weaver', 'master_fur_trader', 'master_blacksmith',
    'master_gunsmith', 'elder_statesman', 'firebrand_preacher',
    'hardy_colonist', 'jesuit_colonist', 'seasoned_colonist',
    'veteran_colonist', 'veteran_soldier', 'veteran_dragoon',
    'continental_army', 'continental_cavalry', 'regular',
    'cavalry', 'hardy_pioneer', 'jesuit_missionary',
    'seasoned_scout', 'artillery', 'damaged_artillery',
    'wagon_train', 'small_treasure', 'large_treasure'
  }

  local function create( where, unit_name )
    local unit = ustate.create_unit_on_map( nation1,
                                            build_unit_type(
                                                unit_name ),
                                            where )
    unit:fortify()
  end

  for i, unit_name in ipairs( land_units ) do
    local coord = {
      x=origin.x + (i - 1) % 7,
      y=origin.y + (i - 1) // 7
    }
    create( coord, unit_name )
  end

  origin = { x=size.w // 2 + 2, y=size.h // 2 - 1 }

  -- Ships
  local ships = {
    'caravel', 'merchantman', 'galleon', 'privateer', 'frigate',
    'man_o_war'
  }

  for i, unit_name in ipairs( ships ) do
    local coord = {
      x=origin.x + (i - 1) % 3,
      y=origin.y + (i - 1) // 3
    }
    create( coord, unit_name )
  end
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
  local init_commodity = function( comm, ask_price )
    old_world.market.commodities[comm]
        .current_ask_price_in_hundreds = ask_price
    old_world.market.commodities[comm]
        .starting_ask_price_in_hundreds = ask_price
  end

  init_commodity( 'food', 10 )
  init_commodity( 'sugar', 10 )
  init_commodity( 'tobacco', 10 )
  init_commodity( 'cotton', 10 )
  init_commodity( 'fur', 10 )
  init_commodity( 'lumber', 10 )
  init_commodity( 'ore', 10 )
  init_commodity( 'silver', 10 )
  init_commodity( 'horses', 10 )
  init_commodity( 'rum', 10 )
  init_commodity( 'cigars', 10 )
  init_commodity( 'cloth', 10 )
  init_commodity( 'coats', 10 )
  init_commodity( 'trade_goods', 10 )
  init_commodity( 'tools', 10 )
  init_commodity( 'muskets', 10 )

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
  player.money = 1000 -- TODO: depends on difficulty.
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
  turns_state.time_point.season = 'spring'
end

-----------------------------------------------------------------
-- Testing
-----------------------------------------------------------------
local function add_testing_options( options )
  options.nations = { english={ human=true } }
  options.difficulty = 'discoverer'
  -- options.map.type = 'half_and_half'
  -- options.map.world_size = { w=4, h=4 }
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

  local root = ROOT

  set_default_settings( options, root.settings )

  map_gen.generate( options.map )

  local world_size = root.terrain:size()
  root.land_view.viewport:set_world_size_tiles( world_size )

  create_turn_state( root.turn )

  create_nations( options, root )

  if options.map.type == 'battlefield' then
    create_battlefield_units( options, root )
  elseif options.map.type == 'half_and_half' then
    -- FIXME: temporary
    create_all_units( options, root )
  else
    create_initial_units( options, root )
  end

  -- Temporary.
  root.land_view.viewport:center_on_tile{
    x=world_size.w // 2 - 2,
    y=world_size.h // 2 - 1
  }
end

return M
