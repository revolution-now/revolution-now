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

-- The save-game state should be default-constructed before
-- calling this.
function M.create( options )
  options = options or M.default_options()
  player.set_players( {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  } )

  map_gen.generate_terrain()
  if options.render then render_terrain.redraw() end
  land_view.zoom_out_optimal()
end

return M
