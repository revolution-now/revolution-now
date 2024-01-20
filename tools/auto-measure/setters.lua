-----------------------------------------------------------------
-- SAV file property setters.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local logger = require'moon.logger'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local info = logger.info

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function on_all_braves( json, op )
  assert( json.UNIT )
  assert( type( json.UNIT ) == 'table' )
  for _, unit in ipairs( json.UNIT ) do
    if unit.type:lower():match( 'brave' ) then op( unit ) end
  end
end

local function on_all_dwellings( json, op )
  assert( json.DWELLING )
  assert( type( json.DWELLING ) == 'table' )
  for _, dwelling in ipairs( json.DWELLING ) do op( dwelling ) end
end

local function on_all_tribes( json, op )
  assert( json.TRIBE )
  assert( type( json.TRIBE ) == 'table' )
  assert( #json.TRIBE == 8 )
  for i, tribe in ipairs( json.TRIBE ) do op( i, tribe ) end
end

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
function M.difficulty( json, value )
  assert( json.HEADER )
  local json_names = {
    discoverer='Discoverer', --
    explorer='Explorer', --
    conquistador='Conquistador', --
    governor='Governor', --
    viceroy='Viceroy', --
  }
  local json_value = assert( json_names[value] )
  info( 'setting difficulty to "%s".', json_value )
  json.HEADER.difficulty = json_value
end

function M.tribe_name( json, value )
  assert( json.HEADER )
  info( 'setting tribe to "%s".', value )

  local tribes = {
    'inca', --
    'aztec', --
    'arawak', --
    'iroquois', --
    'cherokee', --
    'apache', --
    'sioux', --
    'tupi', --
  }

  local json_names = {
    inca='Inca', --
    aztec='Aztec', --
    arawak='Arawak', --
    iroquois='Iroquois', --
    cherokee='Cherokee', --
    apache='Apache', --
    sioux='Sioux', --
    tupi='Tupi', --
  }

  local to_json_visitor_codes = {
    inca='in', --
    aztec='az', --
    arawak='aw', --
    iroquois='ir', --
    cherokee='ch', --
    apache='ap', --
    sioux='si', --
    tupi='tu', --
  }

  local from_json_visitor_codes = {
    ['in']='inca', --
    az='aztec', --
    aw='arawak', --
    ir='iroquois', --
    ch='cherokee', --
    ap='apache', --
    si='sioux', --
    tu='tupi', --
  }

  local json_name = assert( json_names[value] )
  local json_visitor_code =
      assert( to_json_visitor_codes[value] )

  -- 1. Change all braves to the given tribe.
  on_all_braves( json, function( brave )
    brave.nation_info.nation_id = json_name
  end )

  -- 2. Change all dwellings to the given tribe.
  local total_dwellings = 0
  on_all_dwellings( json, function( dwelling )
    dwelling.nation_id = json_name
    total_dwellings = total_dwellings + 1
  end )
  assert( total_dwellings == json.HEADER.dwelling_count )
  local tribe_dwelling_count = assert( json.STUFF
                                           .tribe_dwelling_count )
  for _, tribe_name in ipairs( tribes ) do
    tribe_dwelling_count[tribe_name] = 0
    if tribe_name == value then
      tribe_dwelling_count[tribe_name] = total_dwellings
    end
  end

  -- 3. Make all tribes extinct except the given tribe.
  on_all_tribes( json, function( i, tribe )
    tribe.tribe_flags.extinct = (tribes[i] ~= value)
  end )

  -- 5. Change all native PATH visitors to the given tribe.
  for _, tile in ipairs( json.PATH ) do
    if from_json_visitor_codes[tile.visitor_nation] then
      tile.visitor_nation = json_visitor_code
    end
  end
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
