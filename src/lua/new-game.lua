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
    render=true, -- FIXME
    -- This determines the nations and whether they are human
    -- (true) or AI controlled (false).
    nations={
      [e.nation.english]=false,
      [e.nation.french]=false,
      [e.nation.dutch]=true,
      [e.nation.spanish]=false
    }
  }
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

local function create_initial_units( nation )
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

  return coord
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
  local coord = create_initial_units( nation )
  player.last_high_seas = coord
  create_old_world_state( settings, player )
end

local function create_nations( options, root )
  local players = root.players.players
  local settings = root.settings
  for nation, is_human in pairs( options.nations ) do
    local player = players:reset_player( nation )
    create_player_state( settings, nation, player, is_human )
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

  local root = ROOT_STATE

  map_gen.generate_terrain()

  create_turn_state( root.turn )

  local difficulty_int =
      DIFFICULTY_NAMES[options.difficulty_name]
  local settings = root.settings
  settings.difficulty = assert( difficulty_int )

  create_nations( options, root )

  if options.render then render_terrain.redraw() end
  land_view.zoom_out_optimal()
end

return M
