local M = {}

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format

-----------------------------------------------------------------
-- Metadata.
-----------------------------------------------------------------
local NATION_NAMES = {
  'english', --
  'french', --
  'spanish', --
  'dutch', --
}

local NATION_TO_JSON = {
  english='England',
  french='France',
  spanish='Spain',
  dutch='Netherlands',
}

-- FIXME: import this from smcol_sav_struct.json
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

-- FIXME: import this from smcol_sav_struct.json
local JSON_PROFESSION_TO_VALUE = {
  ['Expert farmer']='00',
  ['Master sugar planter']='01',
  ['Master tobacco planter']='02',
  ['Master cotton planter']='03',
  ['Expert fur trapper']='04',
  ['Expert lumberjack']='05',
  ['Expert ore miner']='06',
  ['Expert silver miner']='07',
  ['Expert fisherman']='08',
  ['Master distiller']='09',
  ['Master tobacconist']='0A',
  ['Master weaver']='0B',
  ['Master fur trader']='0C',
  ['Master carpenter']='0D',
  ['Master blacksmith']='0E',
  ['Master gunsmith']='0F',
  ['Firebrand preacher']='10',
  ['Elder statesman']='11',
  ['*(Student)']='12',
  ['*(Free colonist)']='13',
  ['Hardy pioneer']='14',
  ['Veteran soldier']='15',
  ['Seasoned scout']='16',
  ['Veteran dragoon']='17',
  ['Jesuit missionary']='18',
  ['Indentured servant']='19',
  ['Petty criminal']='1A',
  ['Indian convert']='1B',
  ['Free colonist']='1C',
}

local UNIT_TO_JSON_PROFESSION = {
  expert_farmer='Expert farmer',
  master_sugar_planter='Master sugar planter',
  master_tobacco_planter='Master tobacco planter',
  master_cotton_planter='Master cotton planter',
  expert_fur_trapper='Expert fur trapper',
  expert_lumberjack='Expert lumberjack',
  expert_ore_miner='Expert ore miner',
  expert_silver_miner='Expert silver miner',
  expert_fisherman='Expert fisherman',
  master_distiller='Master distiller',
  master_tobacconist='Master tobacconist',
  master_weaver='Master weaver',
  master_fur_trader='Master fur trader',
  master_carpenter='Master carpenter',
  master_blacksmith='Master blacksmith',
  master_gunsmith='Master gunsmith',
  firebranch_preacher='Firebrand preacher',
  elder_statesman='Elder statesman',
  unused_student='*(Student)',
  unused_free_colonist='*(Free colonist)',
  hardy_pioneer='Hardy pioneer',
  veteran_soldier='Veteran soldier',
  seasoned_scout='Seasoned scout',
  veteran_dragoon='Veteran dragoon',
  jesuit_missionary='Jesuit missionary',
  indentured_servant='Indentured servant',
  petty_criminal='Petty criminal',
  native_convert='Indian convert',
  free_colonist='Free colonist',
}

local UNIT_TO_JSON_UNIT_TYPE = {
  colonist='Colonist',
  soldier='Soldier',
  pioneer='Pioneer',
  missionary='Missionary',
  dragoon='Dragoon',
  scout='Scout',
  regular='Tory regular',
  continental_cavalry='Continental cavalry',
  cavalry='Tory cavalry',
  continental_army='Continental army',
  treasure='Treasure',
  artillery='Artillery',
  wagon_train='Wagon train',
  caravel='Caravel',
  merchantman='Merchantman',
  galleon='Galleon',
  privateer='Privateer',
  frigate='Frigate',
  man_o_war='Man-O-War',
  brave='Brave',
  armed_brave='Armed brave',
  mounted_brave='Mounted brave',
  mounted_warrior='Mounted warrior',
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

function M.nation_name( nation )
  return M.name_for_nation_idx( M.nation_to_idx( nation ) )
end

-----------------------------------------------------------------
-- Utilities.
-----------------------------------------------------------------
local function coord_for( x, y ) return { x=x, y=y } end

-- Will return ONLY tiles that exist. Will NOT include input tile.
function M.surrounding_coords( cc )
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
-- Land View
-----------------------------------------------------------------
function M.set_white_box( json, coord )
  assert( json.STUFF.white_box_x )
  assert( json.STUFF.white_box_y )
  json.STUFF.white_box_x = assert( coord.x )
  json.STUFF.white_box_y = assert( coord.y )
end

-----------------------------------------------------------------
-- COLONY
-----------------------------------------------------------------
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
  assert( quantity >= 0 )
  assert( colony.stock[commodity] )
  colony.stock[commodity] = assert( quantity )
end

-- Will return ONLY tiles that exist. Will NOT include colony tile.
function M.tiles_around_colony( colony )
  return M.surrounding_coords( M.coord_for_colony( colony ) )
end

-- Will do ONLY tiles that exist. Will NOT include colony tile.
function M.on_tiles_around_colony( colony, fn )
  local tiles = M.tiles_around_colony( colony )
  for _, tile in ipairs( tiles ) do fn( tile ) end
end

-----------------------------------------------------------------
-- PLAYER
-----------------------------------------------------------------
function M.find_unique_human( json )
  local found = nil
  for i, player in ipairs( json.PLAYER ) do
    assert( player.control )
    if player.control == 'PLAYER' then
      assert( not found,
              'multiple HUMAN players found when one single one expected.' )
      found = i
    end
  end
  return found
end

function M.find_REF( json )
  assert( json.HEADER.game_flags_1.independence_declared,
          'REF will not exist before independence is declared.' )
  local found = nil
  for i, player in ipairs( json.PLAYER ) do
    assert( player.control )
    if player.control == 'AI' then
      assert( not found,
              'multiple AI players found when one single REF expected.' )
      found = i
    end
  end
  return found
end

-----------------------------------------------------------------
-- UNIT
-----------------------------------------------------------------
local function set_unit_type( o, unit )
  local type = UNIT_TO_JSON_UNIT_TYPE[unit]
  local profession = UNIT_TO_JSON_PROFESSION[unit]
  if type and profession then
    o.type = type
    o.profession_or_treasure_amount = profession
    return
  end

  local function lookup_type( ng_name )
    o.type = assert( UNIT_TO_JSON_UNIT_TYPE[ng_name],
                     'failed to look up ' .. tostring( ng_name ) ..
                         ' in json unit type table.' )
  end

  local function lookup_profession( ng_name )
    local val = assert( JSON_PROFESSION_TO_VALUE[assert(
                            UNIT_TO_JSON_PROFESSION[ng_name] )] )
    o.profession_or_treasure_amount = val
  end

  if unit:match( '^expert_' ) then
    lookup_type( 'colonist' )
    lookup_profession( unit )
    return
  end

  local map = {
    soldier={ type_key='soldier', profession_key='free_colonist' },
    dragoon={ type_key='dragoon', profession_key='free_colonist' },
    veteran_soldier={
      type_key='soldier',
      profession_key='veteran_soldier',
    },
    veteran_dragoon={
      type_key='dragoon',
      profession_key='veteran_dragoon',
    },
    artillery={
      type_key='artillery',
      profession_key='free_colonist',
    },
    damaged_artillery={
      type_key='artillery',
      profession_key='free_colonist',
      damaged=true,
    },
    continental_army={
      type_key='continental_army',
      profession_key='veteran_soldier',
    },
    continental_cavalry={
      type_key='continental_cavalry',
      -- NOTE: OG does veteran_soldier here, not veteran_dragoon.
      profession_key='veteran_soldier',
    },
    wagon_train={
      type_key='wagon_train',
      profession_key='expert_farmer',
    },
    petty_criminal={
      type_key='colonist',
      profession_key='petty_criminal',
    },
    indentured_servant={
      type_key='colonist',
      profession_key='indentured_servant',
    },
    -- TODO: add more here.
  }

  if map[unit] then
    lookup_type( map[unit].type_key )
    lookup_profession( map[unit].profession_key )
    if map[unit].damaged then o.unknown15.damaged = true end
    return
  end

  error( 'setting unit type ' .. unit .. ' not supported.' )
end

local function new_unit()
  local unit = {
    ['x, y']={ 0, 0 },
    type='Colonist',
    nation_info={
      nation_id='England',
      vis_to_english=false,
      vis_to_french=false,
      vis_to_spanish=false,
      vis_to_dutch=false,
    },
    unknown15={
      unknown15a='0000000', --
      damaged=false, --
    },
    moves=0,
    origin_settlement=255,
    ai_plan_mode='X',
    orders='none',
    goto_x=1,
    goto_y=1,
    unknown18='00',
    holds_occupied=0,
    cargo_items={
      { cargo_1='food', cargo_2='food' },
      { cargo_1='food', cargo_2='food' },
      { cargo_1='food', cargo_2='food' },
    },
    cargo_hold={ 0, 0, 0, 0, 0, 0 },
    turns_worked=0,
    profession_or_treasure_amount=assert(
        JSON_PROFESSION_TO_VALUE['Free colonist'] ),
    transport_chain={
      next_unit_idx=-1, --
      prev_unit_idx=-1, --
    },
  }
  return unit
end

local function recalc_unit_chain( json, coord )
  local prev_idx = -1
  for idx, unit in ipairs( json.UNIT ) do
    local json_idx = idx - 1
    local unit_x = assert( unit['x, y'][1] )
    local unit_y = assert( unit['x, y'][2] )
    assert( type( unit_x ) == 'number' )
    assert( type( unit_y ) == 'number' )
    if unit_x == coord.x and unit_y == coord.y then
      local chain = assert( unit.transport_chain )
      chain.prev_unit_idx = prev_idx
      chain.next_unit_idx = -1
      if prev_idx ~= -1 then
        json.UNIT[prev_idx + 1].transport_chain.next_unit_idx =
            json_idx
      end
      prev_idx = json_idx
    end
  end
end

function M.add_unit_map( json, unit, nation, coord, options )
  options = options or {}
  options.orders = options.orders or 'none'
  options.finished_turn = options.finished_turn or false

  nation = M.nation_name( nation )
  assert( M.square_exists( coord ) )

  -- Create empty unit object.
  local o = new_unit()

  -- Set fields.
  o['x, y'] = { coord.x, coord.y }
  -- This handles:
  --   o.type
  --   o.profession_or_treasure_amount
  --   o.unknown15.damaged.
  set_unit_type( o, unit )
  o.nation_info.nation_id = NATION_TO_JSON[nation]
  assert( o.nation_info.nation_id, format(
              'nation %s does not exist.', tostring( nation ) ) )
  o.orders = options.orders
  o.moves = options.finished_turn and 255 or 0

  -- Increment HEADER.unit_count.
  json.HEADER.unit_count = json.HEADER.unit_count + 1

  -- Insert unit object.
  insert( json.UNIT, o )

  -- Redo next/prev unit idx.
  recalc_unit_chain( json, coord )

  -- Set has_unit = 'U'.
  local mask = M.lookup_grid( json.MASK, coord )
  mask.has_unit = 'U'

  -- Set visitor_nation on tile.
  M.set_visitor_nation( json, coord, nation )

  -- Set visitor nation on surroundings if desired.
  local surrounding = M.surrounding_coords( coord )
  for _, tile in ipairs( surrounding ) do
    M.set_visitor_nation_if_empty( json, tile, nation )
  end
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
  -- +1 for lua 1-based lists.
  return assert( grid[coord.y * 58 + coord.x + 1] )
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

function M.set_visitor_nation_if_empty( json, coord, nation )
  local tile = M.lookup_grid( json.PATH, coord )
  assert( tile.visitor_nation )
  if tile.visitor_nation == '  ' then
    M.set_visitor_nation( json, coord, nation )
  end
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
