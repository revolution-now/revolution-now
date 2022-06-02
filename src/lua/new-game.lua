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
    nations={
      e.nation.english, e.nation.french, e.nation.dutch,
      e.nation.spanish
    }
  }
end

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

local function create_player_state( settings, nation, player )
  player.nation = nation
  player.human = true
  player.money = 1000 - 250 * settings.difficulty
  -- This is temporary so that it doesn't keep asking us.
  player.discovered_new_world = 'temporary'
  create_old_world_state( settings, player )
end

local function create_nations( options, root )
  local players = root:players().players
  local settings = root:settings()
  for _, nation in ipairs( options.nations ) do
    local player = players:reset_player( nation )
    create_player_state( settings, nation, player )
  end
end

local function create_turn_state( turns_state )
  turns_state.time_point.year = 1492
  turns_state.time_point.season = e.season.spring
end

-- The save-game state should be default-constructed before
-- calling this.
function M.create( options )
  options = options or {}
  -- Merge the options with the default ones so that any missing
  -- fields will have their default values.
  for k, v in pairs( M.default_options() ) do
    if options[k] == nil then options[k] = v end
  end

  local root = ROOT_STATE

  create_turn_state( root:turn() )

  local difficulty_int =
      DIFFICULTY_NAMES[options.difficulty_name]
  local settings = root:settings()
  settings.difficulty = assert( difficulty_int )

  create_nations( options, root )

  map_gen.generate_terrain()

  if options.render then render_terrain.redraw() end
  land_view.zoom_out_optimal()
end

return M
