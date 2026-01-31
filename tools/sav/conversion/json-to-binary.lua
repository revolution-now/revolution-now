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
local sav_writer = require( 'sav-writer' )
local json_transcode = require( 'json-transcode' )
local logger = require( 'moon.logger' )

-----------------------------------------------------------------
-- Global Settings.
-----------------------------------------------------------------
logger.level = logger.levels.WARNING

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit

local err = logger.err
local info = logger.info

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

  -- Get program arguments.
  local structure_json = assert( args[1] )
  local input_json_file = assert( args[2] )
  local colony_sav = assert( args[3] )

  -- Input JSON file.
  info( 'parsing input JSON sav file %s...', input_json_file )
  local input_file<close> = assert(
                                io.open( input_json_file, 'r' ) )
  local colony_json = json_decode( input_file:read( 'a' ) )
  input_file:close()

  -- Save it. We save to a temp file so that if the json parsing
  -- fails for some reason then it won't corrupt the original bi-
  -- nary file, since that file gets written to gradually as the
  -- json gets parsed.
  local colony_sav_tmp = colony_sav .. '.new'
  sav_writer.save{
    structure_json=structure_json,
    colony_json=colony_json,
    colony_sav=colony_sav_tmp,
  }
  assert( os.rename( colony_sav_tmp, colony_sav ) )

  return 0
end

exit( assert( main( table.pack( ... ) ) ) )