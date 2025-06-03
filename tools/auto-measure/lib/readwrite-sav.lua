-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local sav_reader = require'sav.conversion.sav-reader'
local sav_writer = require'sav.conversion.sav-writer'
local file = require'moon.file'
local logger = require'moon.logger'

assert( sav_reader.load )
assert( sav_writer.save )

-----------------------------------------------------------------
-- Alias.
-----------------------------------------------------------------
local format = string.format
local getenv = os.getenv
local exists = file.exists
local info = logger.info

-----------------------------------------------------------------
-- Folders.
-----------------------------------------------------------------
local HOME = assert( getenv( 'HOME' ) )
local COLONIZE = format(
                     '%s/games/colonization/data/MPS/COLONIZE',
                     HOME )
local RN = format( '%s/dev/revolution-now', HOME )
local SAV = format( '%s/tools/sav', RN )

info( 'HOME:' .. HOME )
info( 'COLONIZE:' .. COLONIZE )
info( 'RN:' .. RN )

-----------------------------------------------------------------
-- Read/Write SAV.
-----------------------------------------------------------------
-- Zero based.
local function sav_file_for_slot( n )
  return format( 'COLONY0%d.SAV', n )
end

local function path_for_sav( sav )
  if type( sav ) == 'number' then sav = sav_file_for_slot( sav ) end
  assert( type( sav ) == 'string' )
  return format( '%s/%s', COLONIZE, sav )
end

local function read_sav( sav )
  return sav_reader.load{
    structure_json=format( '%s/schema/sav-structure.json', SAV ),
    colony_sav=path_for_sav( sav ),
  }
end

local function write_sav( sav, json )
  sav_writer.save{
    structure_json=format( '%s/schema/sav-structure.json', SAV ),
    colony_json=json,
    colony_sav=path_for_sav( sav ),
  }
end

local function remove_sav( sav )
  local path = path_for_sav( sav )
  if exists( path ) then
    os.remove( path )
    assert( not exists( path ) )
  end
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  read_sav=read_sav,
  write_sav=write_sav,
  remove_sav=remove_sav,
  path_for_sav=path_for_sav,
  sav_file_for_slot=sav_file_for_slot,
}
