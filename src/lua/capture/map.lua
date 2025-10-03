--[[ ------------------------------------------------------------
|
| map.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2025-10-03.
|
| Description: Captures segements of the map.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local insert = table.insert
local sort = table.sort

-----------------------------------------------------------------
-- point
-----------------------------------------------------------------
-- Creates a fancy coord object. Should always use this to create
-- a coord.
local function point( p, ... )
  assert( #{ ... } == 0 )
  p = p or { x=0, y=0 }
  assert( p )
  assert( type( p ) == 'table' )
  if p.__point then return p end
  p.x = p.x or 0
  p.y = p.y or 0
  local o = { x=p.x, y=p.y }
  -- The __point can be used to identify a coord as an object.
  return setmetatable( o, {
    __eq=function( l, r ) return l.x == r.x and l.y == r.y end,
    __index=function( tbl, k )
      if k == '__point' then return true end
      return rawget( tbl, k )
    end,
    __newindex=function()
      error( 'cannot add new fields to a point object.' )
    end,
    __tostring=function( tbl )
      return format( '{x=%d,y=%d}', tbl.x, tbl.y )
    end,
    __metatable=false,
  } )
end

-----------------------------------------------------------------
-- General helpers.
-----------------------------------------------------------------
local function printf( fmt, ... ) print( format( fmt, ... ) ) end

-----------------------------------------------------------------
-- MapSquare helpers.
-----------------------------------------------------------------
-- This will try to reduce the number of squares and consolidate
-- them based on their properties w.r.t. movement points. For ex-
-- ample, a mountain tile has the same movement points regardless
-- of the ground type, so we will set its ground type to grass-
-- land so that they are all consistent and we reduce the number
-- of unique tiles.
--
-- NOTE: that we can't make any simplifications just because the
-- tile has a road, because the properties of that tile do matter
-- if it is arrived at via a square that does not contain a road.
local function normalize_square_for_movement( square )
  square.ground_resource = nil
  square.forest_resource = nil
  square.irrigation = false
  if square.overlay == 'mountains' or square.overlay == 'hills' then
    square.ground = 'grassland'
  end
end

local function cpp_mapsquare_str( square )
  local res = ''
  local function add( ... ) res = res .. format( ... ) end
  local function field( ... ) res = res .. format( ... ) .. ',' end
  add( '{' )
  field( '.surface=%s', square.surface )
  field( '.ground=%s', square.ground )
  if square.overlay then field( '.overlay=%s', square.overlay ) end
  if square.river then field( '.river=%s', square.river ) end
  if square.road then field( '.road=%s', square.road ) end
  if square.sea_lane then
    field( '.sea_lane=%s', square.sea_lane )
  end
  if square.lost_city_rumor then
    field( '.lost_city_rumor=%s', square.lost_city_rumor )
  end
  add( '}' )
  res = res:gsub( ',}', '}' )
  return res
end

-- This is needed even if the C++ MapSquare gives __eq to the lua
-- usertype because the arguments to this function will be a C++
-- MapSquare and a lua table, i.e. not both C++ types.
local function squares_equal( l, r )
  local function bool( b ) return b or false end
  return l.surface == r.surface and l.ground == r.ground and
             l.overlay == r.overlay and l.river == r.river and
             l.ground_resource == r.ground_resource and
             l.forest_resource == r.forest_resource and
             bool( l.irrigation ) == bool( r.irrigation ) and
             bool( l.road ) == bool( r.road ) and
             bool( l.sea_lane ) == bool( r.sea_lane ) and
             bool( l.lost_city_rumor ) ==
             bool( r.lost_city_rumor ) --
end

-----------------------------------------------------------------
-- Stock squares.
-----------------------------------------------------------------
-- LuaFormatter off
local SEA_LANE       = { surface='water', ground='arctic',   sea_lane=true,                   }
local OCEAN          = { surface='water', ground='arctic',                                    }
local ARCTIC         = { surface='land',  ground='arctic',                                    }
local GRASSLAND      = { surface='land',  ground='grassland'                                  }
local SAVANNAH       = { surface='land',  ground='savannah'                                   }
local ARCTIC_ROAD    = { surface='land',  ground='arctic',                         road=true, }
local GRASSLAND_ROAD = { surface='land',  ground='grassland',                      road=true, }
local SAVANNAH_ROAD  = { surface='land',  ground='savannah',                       road=true, }
local MOUNTAINS      = { surface='land',  ground='grassland', overlay='mountains',            }
local HILLS          = { surface='land',  ground='grassland', overlay='hills',                }
local MOUNTAINS_ROAD = { surface='land',  ground='grassland', overlay='mountains', road=true, }
local HILLS_ROAD     = { surface='land',  ground='grassland', overlay='hills',     road=true, }
-- LuaFormatter on

-- Squares that don't fit one of these will be assigned a dynami-
-- cally chosen symbol.
local SQUARE_NAMES = {
  [OCEAN]='_',
  [SEA_LANE]='x',
  [ARCTIC]='a',
  [ARCTIC_ROAD]='A',
  [MOUNTAINS]='m',
  [MOUNTAINS_ROAD]='M',
  [HILLS]='h',
  [HILLS_ROAD]='H',
  [GRASSLAND]='g',
  [GRASSLAND_ROAD]='G',
  [SAVANNAH]='s',
  [SAVANNAH_ROAD]='S',
}

-- This list must not contain any of the special symbols reserved
-- for named squares above.
local DYN_SYMBOLS = 'bcdeijklnopqrtuvwyzBCDEFIJKLNOPQRTUVWXYZ'

local function symbol_for_square( square )
  for named, symbol in pairs( SQUARE_NAMES ) do
    if squares_equal( square, named ) then return symbol end
  end
end

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
-- Will take a region of the map defined by the two coordinates
-- passed in and will dump it to a file in a format that can be
-- pasted into a unit test to recreate that map.
function M.map_segment( nw, se )
  nw = point( nw )
  se = point( se )
  assert( nw.x <= se.x )
  assert( nw.y <= se.y )
  print( format( 'map_segment: nw=%s, se=%s', nw, se ) )
  local terrain = assert( ROOT.terrain )
  local squares = {}
  local symbol_idx = 1
  local function find_square( square )
    assert( square )
    for k, v in pairs( squares ) do
      if square == k then return k, v end
    end
  end
  for y = nw.y, se.y do
    for x = nw.x, se.x do
      local p = point{ x=x, y=y }
      local square = terrain:square_at( p )
      normalize_square_for_movement( square )
      if not find_square( square ) then
        local symbol = symbol_for_square( square )
        assert( symbol ~= 'f',
                'f is not allowed because we use it in unit tests as the lambda' )
        if symbol then
          assert( not DYN_SYMBOLS:match( symbol ), format(
                      'symbol %s is a special symbol but is also in the list of generic symbols',
                      symbol ) )
        else
          symbol = assert(
                       DYN_SYMBOLS:sub( symbol_idx, symbol_idx ) )
          symbol_idx = symbol_idx + 1
        end
        squares[square] = { count=1, symbol=symbol }
        -- print( format( '  new tile at %s: %s', p, square ) )
      end
    end
  end
  local total = 0
  for _, _ in pairs( squares ) do total = total + 1 end
  printf( 'unique squares: %d', total )

  -- START generating code.

  local filename = 'test.cpp'
  local f<close> = assert( io.open( filename, 'w' ) )
  f:write(
      '    // NOTE: the below was generated using lua/capture/map.lua\n' )
  f:write( '    using enum e_surface;\n' )
  f:write( '    using enum e_ground_terrain;\n' )
  f:write( '    using enum e_land_overlay;\n' )
  f:write( '    using enum e_river;\n' )
  f:write( '\n' )
  f:write( '    using MS = MapSquare;\n' )
  f:write( '\n' )
  local map_squares = {}
  f:write( '    // clang-format off\n' )
  for square, info in pairs( squares ) do
    local s = format( '    MS const %s%s;', info.symbol,
                      cpp_mapsquare_str( square ) )
    insert( map_squares, format( '%s', s ) )
  end
  sort( map_squares )
  for _, line in ipairs( map_squares ) do
    f:write( format( '%s\n', line ) )
  end
  f:write( '    // clang-format on\n' )
  f:write( '\n' )

  f:write( '    // clang-format off\n' )
  f:write( '    vector<MapSquare> tiles{ /*\n' )

  local x_numbers = ''
  for x = nw.x, se.x do
    x_numbers = x_numbers .. ' ' .. format( '%x', x - nw.x )
  end
  f:write( format( '         %s\n', x_numbers ) )
  for y = nw.y, se.y do
    local row = ''
    row = row .. format( '      %x*/ ', y - nw.y )
    for x = nw.x, se.x do
      local p = point{ x=x, y=y }
      local square = terrain:square_at( p )
      normalize_square_for_movement( square )
      assert( find_square( square ),
              format( 'could not find square at %s', p ) )
      local _, info = find_square( square )
      assert( info )
      row = row .. format( '%s,', info.symbol )
    end
    row = row .. format( ' /*%x\n', y - nw.y )
    f:write( row )
  end
  f:write( format( '         %s\n', x_numbers ) )
  f:write( '    */};\n' )
  f:write( '    // clang-format on\n' )
  f:write( '\n' )
  f:write(
      format( '    w.build_map( std::move( tiles ), %d );\n',
              se.x - nw.x + 1 ) )
  f:write( '    // NOTE: end generated code.\n' )
  f:write( '\n' )

  print( format( 'wrote output to %s', filename ) )
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M