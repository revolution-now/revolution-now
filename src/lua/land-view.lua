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
  plow=function() return { plow={} } end,
}

local key_map = {
  -- Directional move orders.
  ['u']=orders.move( 'nw' ),
  ['i']=orders.move( 'n' ),
  ['o']=orders.move( 'ne' ),
  ['j']=orders.move( 'w' ),
  ['h']=orders.move( 'w' ),
  ['l']=orders.move( 'e' ),
  ['m']=orders.move( 'sw' ),
  ['n']=orders.move( 'sw' ),
  [',']=orders.move( 's' ),
  ['.']=orders.move( 'se' ),
  -- Other orders.
  ['k']=orders.forfeight(),
  ['f']=orders.fortify(),
  ['s']=orders.sentry(),
  ['w']=orders.wait(),
  ['b']=orders.build(),
  ['r']=orders.road(),
  ['p']=orders.plow(),
}

local shift_key_map = {
  -- Other orders.
  ['d']=orders.disband(),
}

local ctrl_key_map = {
  -- none.
}

-- Will accept a key press from the user in the land view and
-- will try to map it to an `orders` sumtype. If it can't then it
-- returns nil.
function M.key_to_orders( key, ctrl, shift )
  if key < 0 or key > 255 then return end
  -- Convert (integral) key into a one-character string, then
  -- look it up in the table. Will yield null if key not found.
  local stringed = string.char( key )
  if ctrl then return ctrl_key_map[stringed] end
  if shift then return shift_key_map[stringed] end
  return key_map[string.char( key )]
end

return M
