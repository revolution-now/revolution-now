local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local time = require'moon.time'
local logger = require'moon.logger'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format

local sleep = time.sleep
local info = logger.info

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local tribe_to_idx = {
  inca=1,
  aztec=2,
  arawak=3,
  iroquois=4,
  cherokee=5,
  apache=6,
  sioux=7,
  tupi=8,
}

local tribe_to_techlevel = {
  inca='civilized',
  aztec='advanced',
  arawak='agrarian',
  iroquois='agrarian',
  cherokee='agrarian',
  apache='semi-nomadic',
  sioux='semi-nomadic',
  tupi='semi-nomadic',
}

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function get_tribe( SAV, tribe )
  local tribe_idx = assert( tribe_to_idx[tribe] )
  return SAV.TRIBE[tribe_idx]
end

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
-- Functions.
-----------------------------------------------------------------
function M.experiment_name( config )
  local tribe_name = assert( config.tribe_name )
  local techlevel = assert( tribe_to_techlevel[tribe_name] )
  local difficulty = assert( config.difficulty )
  local brave = assert( config.brave )
  return format( '%s-%s-%s-brave', difficulty, techlevel, brave )
end

-- Called after the config settings are applied via setters. This
-- method will apply some general setup changes that are needed
-- regardless of config.
function M.post_config_setup( config, json )
  info( 'applying post-config setup.' )

  -- Here we're not going to change anything because we don't
  -- know how to create a brave if one doesn't exist. So we'll
  -- just make sure that we have what we need.
  local brave_count = 0
  on_all_braves( json,
                 function( _ ) brave_count = brave_count + 1 end )
  if config.brave == 'new' then
    assert( brave_count == 0 )
    on_all_dwellings( json, function( dwelling )
      dwelling.BLCS.brave_missing = true
    end )
  elseif config.brave == 'old' then
    assert( brave_count == 1 )
    on_all_dwellings( json, function( dwelling )
      dwelling.BLCS.brave_missing = false
    end )
  else
    error( 'unknown value for "brave": ' ..
               tostring( config.brave ) )
  end

  on_all_dwellings( json, function( dwelling )
    dwelling.growth_counter = 0
  end )
  if config.brave == 'new' then
    on_all_dwellings( json, function( dwelling )
      local tribe = dwelling.nation_id:lower()
      if tribe == config.tribe_name then
        dwelling.growth_counter = 20
      end
    end )
  end

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
  on_all_tribes( json, function( i, tribe )
    if tribes[i] == config.tribe_name then
      tribe.muskets = 1
    else
      tribe.muskets = 0
    end
  end )
end

function M.action( api )
  local enter = assert( api.enter )

  -- This action is to be run when there is a single dwelling
  -- with a regular brave right on top of it and where the tribe
  -- has one unit of muskets to potentially give to the brave on
  -- the very next turn.

  -- Advance the turn to make the game consider giving muskets to
  -- the brave.
  enter()
  sleep( 1.0 )
end

function M.collect_results( config, SAV )
  local tribe_name = assert( config.tribe_name )

  -- Count the number of armed braves.
  local has_armed_brave = false
  for _, unit in ipairs( SAV.UNIT ) do
    if unit.type == 'Armed brave' then
      has_armed_brave = true
      break
    end
  end

  -- Check the number of remaining muskets.
  local tribe = get_tribe( SAV, tribe_name )
  local muskets = assert( tribe.muskets )
  assert( type( muskets ) == 'number' )
  assert( muskets >= 0 )

  local mode = 'NOT_RECEIVED'
  if has_armed_brave then
    mode = 'RECEIVED_AND_CONSUMED'
    if muskets > 0 then mode = 'RECEIVED_AND_RETAINED' end
  end

  -- Can add more keys here if needed.
  return { mode=mode }
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
