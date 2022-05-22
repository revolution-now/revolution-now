--[[ ------------------------------------------------------------
|
| new.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-22.
|
| Description: Creates a new game.
|
--]] ------------------------------------------------------------
local M = {}

-- The save-game state should be default-constructed before
-- calling this.
function M.create()
  player.set_players( {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  } )

  map_gen.generate_terrain()
  render_terrain.redraw()
  land_view.zoom_out_optimal()
end

return M
