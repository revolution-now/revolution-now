--[[ ------------------------------------------------------------
|
| sav.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-16.
|
| Description: Prototype *.SAV parser (OG's save-game files).
|
--]] ------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
-- TODO
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
local ANSI_BOLD = char( 27 ) .. '[1m'
-- local ANSI_UNDER = char( 27 ) .. '[4m'

-----------------------------------------------------------------
-- General utils.
-----------------------------------------------------------------
local function printf( ... ) print( format( ... ) ) end

local function log( ... )
  local msg = format( ... )
  printf( '%sinfo%s %s', ANSI_GREEN, ANSI_NORMAL, msg )
end

local function warn( ... )
  local msg = format( ... )
  printf( '%swarning%s %s', ANSI_YELLOW, ANSI_NORMAL, msg )
end

local function err( ... )
  local msg = format( ... )
  printf( '%s%serror%s %s', ANSI_RED, ANSI_BOLD, ANSI_NORMAL, msg )
end

local function fatal( ... )
  err( ... )
  exit( 1 )
end

local function check( condition, ... )
  if not condition then fatal( ... ) end
end

local function usage() err( 'usage: sav.lua <filename>' ) end

-----------------------------------------------------------------
-- Parsers.
-----------------------------------------------------------------
local function parse_sav( f )
  assert( f )
  local res = { _type='SAV' }
  -- TODO
  return res
end

-----------------------------------------------------------------
-- Reporters.
-----------------------------------------------------------------
-- Recursive function that pretty-prints a layout.
local function print_sav( o, prefix, spaces )
  spaces = spaces or ''
  if type( o ) == 'table' then
    local meta = getmetatable( o )
    if meta and meta.__tostring then
      local s = ''
      if #spaces == 0 then s = prefix end
      return s .. meta.__tostring( o )
    end
    local name = o._type or ''
    if #name > 0 then name = name .. ' ' end
    local s = ''
    if #spaces == 0 then s = prefix end
    s = s .. name .. '{\n'
    spaces = spaces .. '  '
    local keys = {}
    for k, _ in pairs( o ) do table.insert( keys, k ) end
    table.sort( keys )

    for _, k in ipairs( keys ) do
      if not tostring( k ):match( '^_' ) then
        local v = assert( o[k] )
        local k_str = tostring( k )
        s = s .. prefix .. spaces .. k_str .. '=' ..
                print_sav( v, prefix, spaces ) .. ',\n'
      end
    end
    return s .. prefix .. string.sub( spaces, 3 ) .. '}'
  elseif type( o ) == 'string' then
    local s = ''
    if #spaces == 0 then s = prefix end
    return s .. '\'' .. tostring( o ) .. '\''
  else
    local s = ''
    if #spaces == 0 then s = prefix end
    return s .. tostring( o )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  if #args == 0 then
    usage()
    return 1
  end
  if #args > 1 then warn( 'extra unused arguments detected' ) end
  local filename = assert( args[1] )
  assert( #filename > 0 )
  check( filename:match( '^COLONY%d%d%.SAV$' ),
         'filename %s has invalid format.', filename )
  log( 'reading filename %s', filename )
  local res = parse_sav( assert( io.open( filename, 'rb' ) ) )
  log( 'finished parsing. output:' )
  print( print_sav( res, '', '' ) )
  return 0
end

os.exit( assert( main( table.pack( ... ) ) ) )