--[[ ------------------------------------------------------------
|
| colony-mgr.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-12-27.
|
| Description: Lua helpers for the colony-mgr module.
|
--]] ------------------------------------------------------------
local M = {}

-- This is called with the id of any new colony that is founded.
function M.on_founded_colony( col )
  log.info( string.format( 'adding some goods to colony %d.',
                           col:id() ) )
  -- local muskets = col:commodities()[e.commodity.muskets]
  -- local horses = col:commodities()[e.commodity.horses]
  -- local tools = col:commodities()[e.commodity.tools]
  -- col:commodities()[e.commodity.muskets] = muskets + 100
  -- col:commodities()[e.commodity.horses] = horses + 100
  -- col:commodities()[e.commodity.tools] = tools + 60
end

return M
