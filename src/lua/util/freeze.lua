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
    __index=env,
    __newindex=function( _, key, _ )
      error( 'attempt to modify global "' .. key ..
                 '" which is not permitted.' )
    end,
  } )
end

-- This will do the following on the table:
--
--   1. Prevent reading non-existent fields.
--   2. Prevent adding new fields.
--   3. Prevent modifying existing fields.
--   4. Apply these same rules recursively.
--
-- Return value: new table that is hardened. The input table is
-- not changed.
function M.harden( tbl )
  local frozen = {}
  for k, v in pairs( tbl ) do
    if type( v ) == 'table' then
      frozen[k] = M.harden( v )
    else
      frozen[k] = v
    end
  end
  return setmetatable( {}, {
    __index=function( _, k )
      local v = frozen[k]
      if v == nil then
        error( 'attempt to read non-existent key: ' ..
                   tostring( k ), 2 )
      end
      return v
    end,
    __pairs=function( _ ) return pairs( frozen ) end,
    __ipairs=function( _ ) return ipairs( frozen ) end,
    __newindex=function( _, _, _ )
      error( 'attempt to modify a read-only table.' )
    end,
    __metatable=false,
  } )
end

return M
