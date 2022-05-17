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

function M.main()
  player.set_players( {
    e.nation.dutch, e.nation.spanish, e.nation.english,
    e.nation.french
  } )

  map_gen.generate_terrain()
  land_view.set_zoom( .17 )
end

return M
