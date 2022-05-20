--[[ ------------------------------------------------------------
|
| cheat.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-19.
|
| Description: Assistance for cheating/debugging.
|
--]] ------------------------------------------------------------
local M = {}

local map_gen = require( 'map-gen' )

function M.reveal_map( what )
  what = what or 'all'
  local size = map_gen.world_size()
  if what == 'all' then
    for y = 0, size.h - 1 do
      for x = 0, size.w - 1 do
        local square = map_gen.at{ x=x, y=y }
        square:set_visible_for_all()
      end
    end
  end
  render_terrain.render_terrain()
end

return M