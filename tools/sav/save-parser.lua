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
local lunajson = require( 'lunajson' )

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

local function dbg( ... )
  local msg = format( ... )
  printf( '%sdebug%s %s', ANSI_BLUE, ANSI_NORMAL, msg )
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

local function usage()
  err( 'usage: sav.lua <structure-json-file> <SAV-filename>' )
end

-----------------------------------------------------------------
-- JSON helpers.
-----------------------------------------------------------------
-- lunajson.decode(jsonstr, [pos, [nullv, [arraylen]]])
local JNULL = {}

-- Decode  a string of json.
local function json_decode( json_string )
  -- Start decoding from the start of the string.
  local pos = 0
  -- `null` inside the json will be decoded as this sentinel.
  local nullv = JNULL
  -- Store the length of an array in array[0]. This can be used
  -- to distinguish empty arrays from empty objects.
  local arraylen = true
  local tbl =
      lunajson.decode( json_string, pos, nullv, arraylen )
  assert( tbl )
  return tbl
end

-----------------------------------------------------------------
-- Parsers.
-----------------------------------------------------------------
local SAVParser = {}

function SAVParser:backtrace()
  local res = 'top'
  for _, e in ipairs( self.backtrace_ ) do
    res = format( '%s.%s', res, e )
  end
  return res
end

function SAVParser:struct( struct )
  dbg( 'parsing struct [%s]', self:backtrace() )
  assert( struct )
  assert( struct.__key_order )
  assert( not struct.struct )
  assert( not struct.bit_struct )
  assert( not struct.size )
  local res = {}
  for _, e in ipairs( struct.__key_order ) do
    assert( struct[e] )
    if not e:match( '__' ) then
      res[e] = self:entity( struct, e )
    end
  end
  return res
end

function SAVParser:bit_struct( bit_struct )
  dbg( 'parsing bit_struct [%s]', self:backtrace() )
  assert( bit_struct )
  assert( bit_struct.__key_order )
  return JNULL
end

function SAVParser:string( desc )
  dbg( 'parsing string [%s]', self:backtrace() )
  assert( desc )
  assert( desc.type == 'str' )
  assert( type( desc.size ) == 'number' )
  return JNULL
end

function SAVParser:entity( parent, field )
  assert( parent )
  assert( field )
  local tbl = parent[field]
  assert( type( tbl ) == 'table',
          format( 'field %s is not a table', field ) )
  table.insert( self.backtrace_, field )
  local res = JNULL
  if tbl.struct then
    res = self:struct( tbl.struct )
  elseif tbl.bit_struct then
    res = self:bit_struct( tbl.bit_struct )
  elseif tbl.size then
    if tbl.type == 'str' then
      res = self:string( tbl ) --
    end
  end
  table.remove( self.backtrace_ )
  return res
end

local function NewSAVParser( metadata )
  local obj = {}
  obj.metadata_ = metadata
  obj.backtrace_ = {}
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify parsers.', 2 ) end,
    __index=SAVParser,
    __metatable=false,
  } )
  return obj
end

local function parse_sav( structure, sav )
  assert( sav )
  local parser = NewSAVParser( assert( structure.__metadata ) )
  local res = { _type='SAV' }
  local success, msg = pcall( function()
    res.HEAD = parser:struct( structure )
  end )
  assert( success, format( 'error at location [%s]: %s',
                           parser:backtrace(), msg ) )
  return res
end

-----------------------------------------------------------------
-- Reporters.
-----------------------------------------------------------------
local function is_identifier( str )
  local m = str:match( '^[a-zA-Z_][a-zA-Z0-9_]*$' )
  return m ~= nil and #m > 0
end

-- Recursive function that pretty-prints a layout.
local function pprint( o, prefix, spaces )
  spaces = spaces or ''
  if o == JNULL then
    return 'null'
  elseif type( o ) == 'table' then
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
        if not is_identifier( k_str ) then
          k_str = '"' .. k_str .. '"'
        end
        s = s .. prefix .. spaces .. k_str .. ': ' ..
                pprint( v, prefix, spaces ) .. ',\n'
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
  if #args < 2 then
    usage()
    return 1
  end
  if #args > 2 then warn( 'extra unused arguments detected' ) end
  local structure_json = assert( args[1] )
  log( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  local colony_sav = assert( args[2] )
  check( colony_sav:match( '^COLONY%d%d%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav )
  log( 'reading save file %s', colony_sav )
  local res = parse_sav( structure,
                         assert( io.open( colony_sav, 'rb' ) ) )
  log( 'finished parsing. output:' )
  print( pprint( res, '', '' ) )
  return 0
end

os.exit( assert( main( table.pack( ... ) ) ) )