--[[ ------------------------------------------------------------
|
| util.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: General helpers for the Lua SAV manipulators.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
function M.printf( ... ) print( format( ... ) ) end

-- Deep compare of two lua types.
function M.deep_compare( l, r )
  if l == r then return true end
  if type( l ) ~= type( r ) then return false end
  if type( l ) ~= 'table' then return false end
  -- They are both tables.
  assert( type( r ) == 'table' )
  for k, v in pairs( l ) do
    if not M.deep_compare( v, r[k] ) then return false end
  end
  for k, v in pairs( r ) do
    if not M.deep_compare( l[k], v ) then return false end
  end
  return true
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
