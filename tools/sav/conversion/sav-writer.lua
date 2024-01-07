--[[ ------------------------------------------------------------
|
| sav-writer.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2024-01-07.
|
| Description: Save a json doc to a binary SAV file.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local binary_saver = require( 'binary-saver' )
local json_transcode = require( 'json-transcode' )
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local check = util.check
local info = util.info

local BinarySaver = binary_saver.BinarySaver
local json_decode = json_transcode.decode

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
function M.save( args )
  local structure_json = assert( args.structure_json )
  local colony_json = assert( args.colony_json )
  local colony_sav_file = assert( args.colony_sav_file )

  -- Structure document.
  info( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  assert( structure.__metadata )
  assert( structure.HEADER )

  -- Binary SAV file.
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

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M