--[[ ------------------------------------------------------------
|
| lambda-on-binaries.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2026-01-19.
|
| Description: Runs a lambda function on multiple parsed binaries.
|
--]] ------------------------------------------------------------
local sav_reader = require( 'sav-reader' )
local util = require( 'util' )
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
local check = logger.check
local remove = table.remove
local printf = util.printf
local format = string.format

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: ' .. 'lambda-on-binaries.lua ' ..
           '<structure-json-file> ' .. '<label>' ..
           '<lambda-file>' .. '[<SAV-filename>, ...]' )
end

local function clear_line()
  io.write( format( '\r%s\r', string.rep( ' ', 60 ) ) )
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  -- Check args.
  if #args < 4 then
    usage()
    return 1
  end

  local structure_json = assert( args[1] )
  local label = assert( args[2] )
  local lambda_file = assert( args[3] )
  remove( args, 1 )
  remove( args, 1 )
  remove( args, 1 )
  local colony_savs = args
  assert( #colony_savs > 0 )

  info( 'loading lambda file ' .. lambda_file )
  local env = setmetatable( { printf=printf }, { __index=_ENV } )
  local lambda_module =
      assert( loadfile( lambda_file, 't', env ) )
  local module = assert( lambda_module() )
  local lambda = assert( module.lambda )
  local finished = assert( module.finished )
  check( type( lambda ) == 'function',
         'lambda module must return a function.' )

  for i, filename in ipairs( colony_savs ) do
    clear_line()
    io.write( format( '\rrunning [%d/%d] %s...', i, #colony_savs,
                      filename:match( '.*/(.*)' ) ) )
    io.flush()
    local json = sav_reader.load{
      structure_json=structure_json,
      colony_sav=filename,
    }
    assert( json )

    local ok, msg = pcall( lambda, json )
    if not ok then
      printf( 'error in processing %s: %s',
              filename:match( '.*/(.*)' ), msg )
    end
    if i % 100 == 0 then collectgarbage() end
  end
  clear_line()
  io.flush()

  -- Tell the lambda that we're finished so that it can print re-
  -- sults.
  finished( label )

  return 0
end

exit( assert( main( table.pack( ... ) ) ) )