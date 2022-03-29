--[[ ------------------------------------------------------------
|
| land-view.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-12-16.
|
| Description: Hooks used by land-view module.
|
--]] ------------------------------------------------------------
local M = {}

local orders = {
  move=function( d ) return { move={ d=assert( d ) } } end,
  forfeight=function() return { forfeight={} } end,
  fortify=function() return { fortify={} } end,
  sentry=function() return { sentry={} } end,
  disband=function() return { disband={} } end,
  wait=function() return { wait={} } end,
  build=function() return { build={} } end,
  road=function() return { road={} } end,
  plow=function() return { plow={} } end
}

local key_map = {
  -- Directional move orders.
  ['u']=orders.move( e.direction.nw ),
  ['i']=orders.move( e.direction.n ),
  ['o']=orders.move( e.direction.ne ),
  ['j']=orders.move( e.direction.w ),
  ['h']=orders.move( e.direction.w ),
  ['l']=orders.move( e.direction.e ),
  ['m']=orders.move( e.direction.sw ),
  ['n']=orders.move( e.direction.sw ),
  [',']=orders.move( e.direction.s ),
  ['.']=orders.move( e.direction.se ),
  -- Other orders.
  ['k']=orders.forfeight(),
  ['f']=orders.fortify(),
  ['s']=orders.sentry(),
  ['d']=orders.disband(),
  ['w']=orders.wait(),
  ['b']=orders.build(),
  ['r']=orders.road(),
  ['p']=orders.plow()
}

-- Will accept a key press from the user in the land view and
-- will try to map it to an `orders` sumtype. If it can't then it
-- returns nil.
function M.key_to_orders( key, ctrl, shift )
  if ctrl or shift then return end
  if key < 0 or key > 255 then return end
  -- Convert (integral) key into a one-character string, then
  -- look it up in the table. Will yield null if key not found.
  return key_map[string.char( key )]
end

return M
