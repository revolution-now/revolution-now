--[[ ------------------------------------------------------------
|
| lambda-on-binary.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2025-05-03.
|
| Description: Runs a lambda function on a parsed binary.
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

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: ' .. 'lambda-on-binary.lua ' ..
           '<structure-json-file> ' .. '<SAV-filename> ' ..
           '<lambda-file>' )
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
  local lambda_file = assert( args[3] )

  local parsed = sav_reader.load{
    structure_json=structure_json,
    colony_sav=colony_sav,
  }
  assert( parsed )

  info( 'loading lambda file ' .. lambda_file )
  local env = setmetatable( { printf=util.printf },
                            { __index=_ENV } )
  local lambda_module =
      assert( loadfile( lambda_file, 't', env ) )
  local lambda = assert( lambda_module() )
  check( type( lambda ) == 'function',
         'lambda module must return a function.' )

  info( 'running lambda...' )
  lambda( parsed )

  return 0
end

exit( assert( main( table.pack( ... ) ) ) )