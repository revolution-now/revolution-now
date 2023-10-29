--[[ ------------------------------------------------------------
|
| save-parser.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-16.
|
| Description: SAV parser (OG's save-game files).
|
--]] ------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local lunajson = require( 'lunajson' )
local binary_loader = require( 'binary-loader' )
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit
local yield = coroutine.yield
local NewBinaryLoader = binary_loader.NewBinaryLoader
local err = util.err
local info = util.info
local fatal = util.fatal
local check = util.check

-----------------------------------------------------------------
-- General utils.
-----------------------------------------------------------------
local function usage()
  err(
      'usage: sav.lua <structure-json-file> <SAV-filename> <output>' )
end

-----------------------------------------------------------------
-- JSON helpers.
-----------------------------------------------------------------
local JNULL = {}

-- Decode a string of json.
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
-- Structure document.
-----------------------------------------------------------------
local function add_reverse_metadata( metadata )
  for _, v in pairs( metadata ) do
    if type( v ) == 'table' then
      local reverse = {}
      for k2, v2 in pairs( v ) do
        assert( not reverse[v2] )
        reverse[v2] = k2
      end
      v.__reverse = reverse
      setmetatable( v.__reverse, {
        __index=function( tbl, k )
          if rawget( tbl, k ) then
            return rawget( tbl, k )
          end
          if rawget( tbl, k:lower() ) then
            return rawget( tbl, k:lower() )
          end
          if rawget( tbl, k:upper() ) then
            return rawget( tbl, k:upper() )
          end
        end,
      } )
    end
  end
end

-----------------------------------------------------------------
-- Parsers.
-----------------------------------------------------------------
local function parse_sav( structure, parser )
  assert( parser )
  local res
  local success, msg = pcall( function()
    res = parser:struct( structure )
  end )
  check( success, 'error at location [%s]: %s',
         parser:backtrace(), msg )
  return res, parser:stats()
end

-----------------------------------------------------------------
-- Reporters.
-----------------------------------------------------------------
local function appender()
  local res = ''
  return function( segment )
    if not segment then
      -- flush line.
      yield( res )
      res = ''
    else
      res = res .. segment
    end
  end
end

-- Coroutine generator function that pretty-prints a layout in
-- conforming JSON while preserving key ordering. Each time a
-- line is produced it will yield it.
local function pprint_json( append, o, prefix, spaces )
  spaces = spaces or ''
  assert( o ~= JNULL )
  if type( o ) == 'table' and not o[0] then
    -- Object.
    if #spaces == 0 then append( prefix ) end
    append( '{' )
    append()
    spaces = spaces .. '  '
    local keys = {}
    if o.__key_order then
      for _, k in ipairs( o.__key_order ) do
        assert( type( k ) == 'string' )
        if not k:match( '^_' ) then
          table.insert( keys, k )
        end
      end
    else
      for k, _ in pairs( o ) do
        assert( type( k ) == 'string' )
        if not k:match( '^_' ) then
          table.insert( keys, k )
        end
      end
      table.sort( keys )
    end
    for i, k in ipairs( keys ) do
      assert( o[k] ~= nil )
      local v = o[k]
      k = '"' .. k .. '"'
      append( prefix .. spaces .. k .. ': ' )
      pprint_json( append, v, prefix, spaces )
      if i ~= #keys then append( ',' ) end
      append()
    end
    append( prefix .. string.sub( spaces, 3 ) .. '}' )
  elseif type( o ) == 'table' and o[0] then
    -- Array.
    if #spaces == 0 then append( prefix ) end
    append( '[' )
    append()
    spaces = spaces .. '  '
    for i, e in ipairs( o ) do
      append( prefix .. spaces )
      pprint_json( append, e, prefix, spaces )
      if i ~= #o then append( ',' ) end
      append()
    end
    append( prefix .. string.sub( spaces, 3 ) .. ']' )
  elseif type( o ) == 'string' then
    local s = ''
    if #spaces == 0 then s = prefix end
    append( s .. '"' .. tostring( o ) .. '"' )
  else
    local s = ''
    if #spaces == 0 then s = prefix end
    append( s .. tostring( o ) )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  if #args < 3 then
    usage()
    return 1
  end
  if #args > 3 then warn( 'extra unused arguments detected' ) end
  local structure_json = assert( args[1] )
  info( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  info( 'producing reverse metadata mapping...' )
  assert( structure.__metadata )
  add_reverse_metadata( structure.__metadata )
  local colony_sav = assert( args[2] )
  check( colony_sav:match( '%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav )
  info( 'reading save file %s', colony_sav )
  local output_json = assert( args[3] )
  local handler = NewBinaryLoader( structure.__metadata,
                                   colony_sav )
  local parsed, stats = parse_sav( structure, handler )
  info( 'finished parsing. stats:' )
  info( 'bytes read: %d', stats.bytes_read )
  if stats.bytes_remaining > 0 then
    fatal( 'bytes remaining: %d', stats.bytes_remaining )
  end
  info( 'encoding json...' )
  local append = appender()
  local printer = coroutine.wrap( function()
    pprint_json( append, parsed, '', '' )
    append() -- emit final brace.
  end )
  info( 'writing output file %s...', output_json )
  local out = assert( io.open( output_json, 'w' ) )
  for line in printer do out:write( line .. '\n' ) end
  out:close()
  return 0
end

exit( assert( main( table.pack( ... ) ) ) )