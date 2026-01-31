--[[ ------------------------------------------------------------
|
| binary-to-json.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2024-1-1.
|
| Description: Loads a binary SAV file into a JSON doc.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local binary_loader = require( 'binary-loader' )
local json_transcode = require( 'json-transcode' )
local logger = require( 'moon.logger' )

-----------------------------------------------------------------
-- Global Settings.
-----------------------------------------------------------------
logger.level = logger.levels.WARNING

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local check = logger.check
local fatal = logger.fatal
local info = logger.info

local NewBinaryLoader = binary_loader.NewBinaryLoader
local json_decode = json_transcode.decode

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
function M.load( args )
  -- Check args.
  local structure_json = assert( args.structure_json )
  local colony_sav = assert( args.colony_sav )

  -- Structure document.
  info( 'decoding json structure file %s...', structure_json )
  local f_structure<close> = assert(
                                 io.open( structure_json, 'r' ) )
  local structure = json_decode( f_structure:read( 'a' ) )
  assert( structure.__metadata )
  assert( structure.HEADER )

  -- Binary SAV file.
  check( colony_sav:match( '%.SAV*' ),
         'colony_sav %s has invalid format.', colony_sav )

  -- Parsing.
  info( 'reading save file %s', colony_sav )
  local loader<close> = assert( NewBinaryLoader(
                                    structure.__metadata,
                                    colony_sav ) )
  local parsed = loader:struct( structure )

  -- Print stats.
  local stats = loader:stats()
  info( 'finished parsing. stats:' )
  info( 'bytes read: %d', stats.bytes_read )
  if stats.bytes_remaining > 0 then
    fatal( 'bytes remaining: %d', stats.bytes_remaining )
  end

  return parsed
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M