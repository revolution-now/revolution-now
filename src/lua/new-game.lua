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

function M.default_options()
  return {
    render=true --
  }
end

local function create_old_world_state( old_world )
  -- Immigrants state.
  old_world.immigration.next_recruit_cost_base = 50
  old_world.immigration.immigrants_pool[1] =
      e.unit_type.expert_farmer
  old_world.immigration.immigrants_pool[2] =
      e.unit_type.free_colonist
  old_world.immigration.immigrants_pool[3] =
      e.unit_type.seasoned_scout

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

local function create_player_state( player )
  player:set_human( true )

  player:set_money( 1000 )

  player:set_crosses( 32 )

  create_old_world_state( player:old_world() )
end

-- The save-game state should be default-constructed before
-- calling this.
function M.create( options )
  options = options or M.default_options()

  local nations = {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  }
  player.set_players( nations )
  for _, nation in ipairs( nations ) do
    create_player_state( player.player_object( nation ) )
  end

  map_gen.generate_terrain()

  if options.render then render_terrain.redraw() end
  land_view.zoom_out_optimal()
end

return M
