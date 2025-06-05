local M = {}

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert

-----------------------------------------------------------------
-- Metadata.
-----------------------------------------------------------------
local NATION_NAMES = {
  'english', --
  'french', --
  'spanish', --
  'dutch', --
}

local NATION_4BIT_SHORT = {
  'EN', --
  'FR', --
  'SP', --
  'NL', --
  'in', --
  'az', --
  'aw', --
  'ir', --
  'ch', --
  'ap', --
  'si', --
  'tu', --
  '  ', --
}

-- 1-based.
function M.name_for_nation_idx( idx )
  return assert( NATION_NAMES[idx] )
end

function M.nation_to_idx( nation )
  if type( nation ) == 'number' then
    assert( nation >= 1 )
    assert( nation <= 4 )
    return nation
  end
  for idx, name in ipairs( NATION_NAMES ) do
    if name == nation then return idx end
  end
  error( 'failed to find nation named ' .. tostring( nation ) )
end

-----------------------------------------------------------------
-- Utilities.
-----------------------------------------------------------------
local function coord_for( x, y ) return { x=x, y=y } end

-----------------------------------------------------------------
-- HEADER
-----------------------------------------------------------------
function M.set_difficulty( json, difficulty )
  assert( json.HEADER )
  local jvalues = {
    discoverer='Discoverer', --
    explorer='Explorer', --
    conquistador='Conquistador', --
    governor='Governor', --
    viceroy='Viceroy', --
  }
  local jvalue = assert( jvalues[difficulty] )
  json.HEADER.difficulty = jvalue
end

-----------------------------------------------------------------
-- COLONY
-----------------------------------------------------------------
function M.assert_single_colony( json )
  assert( json.HEADER.colony_count == 1 )
  assert( #json.COLONY == 1 )
end

function M.coord_for_colony( colony )
  local x = assert( colony['x, y'][1] )
  local y = assert( colony['x, y'][2] )
  assert( type( x ) == 'number' )
  assert( type( y ) == 'number' )
  assert( x >= 1 )
  assert( y >= 1 )
  assert( x <= 56 )
  assert( y <= 70 )
  return coord_for( x, y )
end

function M.set_colony_fortification( colony, fortification )
  local jvalues = {
    none='0', --
    stockade='1', --
    fort='2', --
    fortress='3', --
  }
  local jvalue = assert( jvalues[fortification] )
  colony.buildings.fortification = jvalue
end

function M.set_colony_stock( colony, commodity, quantity )
  assert( type( quantity ) == 'number' )
  assert( quantity > 0 )
  assert( colony.stock[commodity] )
  colony.stock[commodity] = assert( quantity )
end

-- Will return ONLY tiles that exist. Will NOT include colony tile.
function M.tiles_around_colony( colony )
  local cc = M.coord_for_colony( colony )
  local coords_all = {
    coord_for( cc.x - 1, cc.y - 1 ),
    coord_for( cc.x + 0, cc.y - 1 ),
    coord_for( cc.x + 1, cc.y - 1 ),
    coord_for( cc.x - 1, cc.y + 0 ),
    -- coord_for( cc.x + 0, cc.y + 0 ),
    coord_for( cc.x + 1, cc.y + 0 ),
    coord_for( cc.x - 1, cc.y + 1 ),
    coord_for( cc.x + 0, cc.y + 1 ),
    coord_for( cc.x + 1, cc.y + 1 ),
  }
  local coords_exist = {}
  for _, coord in ipairs( coords_all ) do
    if M.square_exists( coord ) then
      insert( coords_exist, coord )
    end
  end
  return coords_exist
end

-- Will do ONLY tiles that exist. Will NOT include colony tile.
function M.on_tiles_around_colony( colony, fn )
  local tiles = M.tiles_around_colony( colony )
  for _, tile in ipairs( tiles ) do fn( tile ) end
end

-----------------------------------------------------------------
-- PLAYER
-----------------------------------------------------------------
function M.find_human( json )
  local found = nil
  for i, player in ipairs( json.PLAYER ) do
    assert( player.control_type )
    if player.control_type == 'PLAYER' then
      assert( not found,
              'multiple HUMAN players found when one single one expected.' )
      found = i
    end
  end
  return found
end

function M.find_REF( json )
  assert( json.HEADER.game_flags_1.independence_declared )
  local found = nil
  for i, player in ipairs( json.PLAYER ) do
    assert( player.control_type )
    if player.control_type == 'AI' then
      assert( not found,
              'multiple AI players found when one single REF expected.' )
      found = i
    end
  end
  return found
end

-----------------------------------------------------------------
-- Map general.
-----------------------------------------------------------------
function M.square_exists( coord )
  local x = assert( coord.x )
  local y = assert( coord.y )
  return x >= 1 and x <= 56 and y >= 1 and y <= 70
end

function M.lookup_grid( grid, coord )
  assert( M.square_exists( coord ) )
  return assert( grid[coord.y * 58 + coord.x] )
end

-----------------------------------------------------------------
-- PATH
-----------------------------------------------------------------
function M.set_visitor_nation( json, coord, nation )
  local nation_idx = M.nation_to_idx( nation )
  local visitor = assert( NATION_4BIT_SHORT[nation_idx] )
  local tile = M.lookup_grid( json.PATH, coord )
  assert( tile.visitor_nation )
  tile.visitor_nation = visitor
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
