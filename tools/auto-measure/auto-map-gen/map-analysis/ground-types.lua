-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of ground terrains.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local abs = math.abs
local insert = table.insert
local format = string.format
local concat = table.concat

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
-- The data here spans multiple savs.
local D = {
  total_savs=0,
  land={},
  savannah={}, --
}

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function lambda( json )
  D.total_savs = D.total_savs + 1
  Q.on_all_tiles( function( tile )
    local terrain = Q.terrain_at( json, tile )

    D.land[tile.y] = D.land[tile.y] or 0
    if terrain.surface == 'water' then
      D.land[tile.y] = D.land[tile.y] + 1
    end

    D.savannah[tile.y] = D.savannah[tile.y] or 0
    if terrain.ground == 'savannah' then
      D.savannah[tile.y] = D.savannah[tile.y] + 1
    end
  end )
end

local function finished()
  local f<close> = assert( io.open( 'dist.csv', 'w' ) )
  local emit = function( fmt, ... )
    -- printfln( fmt, ... )
    f:write( format( fmt, ... ) )
    f:write( '\n' )
  end
  local header = { 'y', 'savannah' }
  emit( '%s', concat( header, ',' ) )
  for y = 1, 70 do
    local count = assert( D.savannah[y] )
    local land = assert( D.land[y] )
    local density = count / land
    emit( '%d,%f', y, density )
  end
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return function( json )
  if json == nil then
    finished()
  else
    lambda( json )
  end
end