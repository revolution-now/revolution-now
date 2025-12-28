-- This lambda will gather some statistics on the layout and con-
-- tents of the OG's maps in order to inform map generation and
-- dwelling distribution.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local abs = math.abs
local insert = table.insert

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return function( json )
  local total_tiles = 0
  local land_tiles = 0
  local left_most_land_x = 1000000000
  local right_most_land_x = -1
  Q.on_all_tiles( function( tile )
    total_tiles = total_tiles + 1
    if Q.is_land( json, tile ) then
      land_tiles = land_tiles + 1
      if tile.x < left_most_land_x then
        left_most_land_x = tile.x
      end
      if tile.x > right_most_land_x then
        right_most_land_x = tile.x
      end
    end
  end )
  assert( total_tiles == 3920 )
  local num_dwellings = 0
  local dwelling_locations = {}
  for _, dwelling in ipairs( json.DWELLING ) do
    num_dwellings = num_dwellings + 1
    local tile = assert( Q.coord_for_dwelling( dwelling ) )
    insert( dwelling_locations, Q.rastorize( tile ) )
  end
  local dwellings_close = {}
  for _, rast1 in ipairs( dwelling_locations ) do
    for _, rast2 in ipairs( dwelling_locations ) do
      if rast1 == rast2 then goto continue end
      local tile1 = Q.unrastorize( rast1 )
      local tile2 = Q.unrastorize( rast2 )
      local dist = 2
      if abs( tile1.x - tile2.x ) <= dist and
          abs( tile1.y - tile2.y ) <= dist then
        dwellings_close[rast1] = true
      end
      ::continue::
    end
  end
  local num_dwellings_close = 0
  for _, _ in pairs( dwellings_close ) do
    num_dwellings_close = num_dwellings_close + 1
  end
  printfln( 'land fraction: %.1f%%',
            land_tiles / total_tiles * 100 );
  printfln( 'num_dwellings: %d', num_dwellings )
  printfln( 'num_dwellings_close: %d', num_dwellings_close )
  printfln( 'dwelling fraction: %.1f%%',
            num_dwellings / land_tiles * 100 );
  printfln( 'left_most_land_x (NG coords): %d',
            left_most_land_x - 1 )
  printfln( 'right_most_land_x (NG coords): %d',
            right_most_land_x - 1 )
end