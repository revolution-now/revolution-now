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
local json_transcode = require( 'json-transcode' )
local sav_reader = require( 'sav-reader' )
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit

local err = util.err
local info = util.info

local pprint_ordered = json_transcode.pprint_ordered

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

  local structure_json = assert( args[1] )
  local colony_sav = assert( args[2] )
  local output_json = assert( args[3] )

  local parsed = sav_reader.load{
    structure_json=structure_json,
    colony_sav=colony_sav,
  }

  -- Output JSON file.
  local out = assert( io.open( output_json, 'w' ) )

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