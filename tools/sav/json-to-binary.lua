--[[ ------------------------------------------------------------
|
| json-to-binary.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-11-04.
|
| Description: Converts binary SAV to json.
|
--]] ------------------------------------------------------------
local binary_saver = require( 'binary-saver' )
local json_transcode = require( 'json-transcode' )
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit

local err = util.err
local check = util.check
local info = util.info

local BinarySaver = binary_saver.BinarySaver
local json_decode = json_transcode.decode

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: ' .. 'json-to-binary.lua ' ..
           '<structure-json-file> ' .. '<output> ' ..
           '<SAV-filename>' )
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

  -- Input JSON file.
  local input_json_file = assert( args[2] )
  info( 'parsing input JSON sav file %s...', input_json_file )
  local input_file = assert( io.open( input_json_file, 'r' ) )
  local colony_json = json_decode( input_file:read( 'a' ) )
  input_file:close()

  -- Binary SAV file.
  local colony_sav_file = assert( args[3] )
  check( colony_sav_file:match( '%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav_file )
  local colony_sav = assert( io.open( colony_sav_file, 'wb' ) )

  -- Traverse structure and emit binary.
  info( 'writing binary save file %s', colony_sav_file )
  local saver = BinarySaver( structure.__metadata, colony_json,
                             colony_sav )
  assert( saver )
  saver:struct( structure )

  -- Cleanup.
  colony_sav:close()
  return 0
end

exit( assert( main( table.pack( ... ) ) ) )