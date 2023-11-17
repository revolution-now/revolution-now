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
local char = string.char
local exit = os.exit

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local ANSI_NORMAL = char( 27 ) .. '[00m'
local ANSI_GREEN = char( 27 ) .. '[32m'
local ANSI_RED = char( 27 ) .. '[31m'
local ANSI_YELLOW = char( 27 ) .. '[93m'
local ANSI_BLUE = char( 27 ) .. '[34m'
local ANSI_BOLD = char( 27 ) .. '[1m'

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
function M.printf( ... ) print( format( ... ) ) end

function M.info( ... )
  local msg = format( ... )
  M.printf( '%sinfo%s %s', ANSI_GREEN, ANSI_NORMAL, msg )
end

function M.dbg( ... )
  local msg = format( ... )
  M.printf( '%sdebug%s %s', ANSI_BLUE, ANSI_NORMAL, msg )
end

function M.warn( ... )
  local msg = format( ... )
  M.printf( '%swarning%s %s', ANSI_YELLOW, ANSI_NORMAL, msg )
end

function M.err( ... )
  local msg = format( ... )
  M.printf( '%s%serror%s %s', ANSI_RED, ANSI_BOLD, ANSI_NORMAL,
            msg )
end

function M.fatal( ... )
  M.err( ... )
  exit( 1 )
end

function M.check( condition, ... )
  if not condition then M.fatal( ... ) end
end

function M.not_implemented() assert( false, 'not implemented' ) end

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
