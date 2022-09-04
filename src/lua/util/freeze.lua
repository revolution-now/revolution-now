--[[ ------------------------------------------------------------
|
| freeze.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Helpers for freezing globals.
|
--]] ------------------------------------------------------------
local M = {}

-- This will prevent creating new globals and modifying existing
-- globals.  Example usage at top of module:
--
--   local freeze = require( 'freeze' )
--   local _ENV = freeze.globals( _ENV )
--
--   print = 5 -- error!
--   xyz = 4 -- error!
--
function M.globals( env )
  return setmetatable( {}, {
    __index=_ENV,
    __newindex=function( _, key, _ )
      error( 'attempt to modify global "' .. key ..
                 '" which is not permitted.' )
    end
  } )
end

return M