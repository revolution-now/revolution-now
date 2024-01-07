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
  local colony_sav = assert( args.colony_sav )

  -- Structure document.
  info( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  assert( structure.__metadata )
  assert( structure.HEADER )

  -- Binary SAV file.
  check( colony_sav:match( '%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav )
  local out = assert( io.open( colony_sav, 'wb' ) )

  -- Traverse structure and emit binary.
  info( 'writing binary save file %s', colony_sav )
  local saver = BinarySaver( structure.__metadata, colony_json,
                             out )
  assert( saver )
  saver:struct( structure )

  -- Cleanup.
  out:close()
  return 0
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M