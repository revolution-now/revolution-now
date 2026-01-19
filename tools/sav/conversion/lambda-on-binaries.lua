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
local insert = table.insert
local printf = util.printf
local format = string.format

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: ' .. 'lambda-on-binaries.lua ' ..
           '<structure-json-file> ' .. '<lambda-file>' ..
           '[<SAV-filename>, ...]' )
end

local function clear_line()
  io.write( format( '\r%s\r', string.rep( ' ', 60 ) ) )
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

  local structure_json = assert( args[1] )
  local lambda_file = assert( args[2] )
  remove( args, 1 )
  remove( args, 1 )
  local colony_savs = args
  assert( #colony_savs > 0 )

  info( 'loading lambda file ' .. lambda_file )
  local env = setmetatable( { printf=printf }, { __index=_ENV } )
  local lambda_module =
      assert( loadfile( lambda_file, 't', env ) )
  local lambda = assert( lambda_module() )
  check( type( lambda ) == 'function',
         'lambda module must return a function.' )

  local parsed = {}
  for i, filename in ipairs( colony_savs ) do
    clear_line()
    io.write( format( '\rloading [%d/%d] %s...', i, #colony_savs,
                      filename:match( '.*/(.*)' ) ) )
    io.flush()
    local json = sav_reader.load{
      structure_json=structure_json,
      colony_sav=filename,
    }
    assert( json )
    insert( parsed, { json=json, filename=filename } )
  end
  clear_line()
  io.flush()

  info( 'running lambda...' )
  for _, loaded in ipairs( parsed ) do
    local ok, msg = pcall( lambda, loaded.json )
    if not ok then
      printf( 'error in processing %s: %s',
              loaded.filename:match( '.*/(.*)' ), msg )
    end
  end

  -- Tell the lambda that we're finished so that it can print re-
  -- sults.
  lambda()

  return 0
end

exit( assert( main( table.pack( ... ) ) ) )