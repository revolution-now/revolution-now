--[[ ------------------------------------------------------------
|
| binary-to-json.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-16.
|
| Description: Converts binary SAV to json.
|
--]] ------------------------------------------------------------
local binary_loader = require( 'binary-loader' )
local json_transcode = require( 'json-transcode' )
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit

local err = util.err
local check = util.check
local fatal = util.fatal
local info = util.info

local NewBinaryLoader = binary_loader.NewBinaryLoader
local pprint_ordered = json_transcode.pprint_ordered
local json_decode = json_transcode.decode

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: ' .. 'binary-to-json.lua ' ..
           '<structure-json-file> ' .. '<SAV-filename> ' ..
           '<output>' )
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  -- Check args.
  if #args < 3 then
    usage()
    return 1
  end
  if #args > 3 then warn( 'extra unused arguments detected' ) end

  -- Structure document.
  local structure_json = assert( args[1] )
  info( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  assert( structure.__metadata )
  assert( structure.HEADER )

  -- Binary SAV file.
  local colony_sav = assert( args[2] )
  check( colony_sav:match( '%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav )

  -- Output JSON file.
  local output_json = assert( args[3] )
  local out = assert( io.open( output_json, 'w' ) )

  -- Parsing.
  info( 'reading save file %s', colony_sav )
  local loader = assert( NewBinaryLoader( structure.__metadata,
                                          colony_sav ) )
  local parsed = loader:struct( structure )

  -- Print stats.
  local stats = loader:stats()
  info( 'finished parsing. stats:' )
  info( 'bytes read: %d', stats.bytes_read )
  if stats.bytes_remaining > 0 then
    fatal( 'bytes remaining: %d', stats.bytes_remaining )
  end

  -- Encoding and outputting JSON.
  info( 'encoding json output to file %s...', output_json )
  local printer = pprint_ordered( parsed )
  local need_nl = false
  for line in printer do
    if need_nl then out:write( '\n' ) end
    need_nl = true
    out:write( line )
  end

  -- Cleanup.
  out:close()
  return 0
end

exit( assert( main( table.pack( ... ) ) ) )