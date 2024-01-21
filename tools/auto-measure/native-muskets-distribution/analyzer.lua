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
local insert = table.insert
local floor = math.floor

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

local function get_mode( stats, name )
  local val = stats['mode=' .. name] or 0
  assert( type( val ) == 'number' )
  local int, frac = math.modf( val )
  assert( frac == 0 )
  assert( int >= 0 )
  return int
end

local function get_stats_fields( stats )
  local NOT_RECEIVED = get_mode( stats, 'NOT_RECEIVED' )
  local RECEIVED_AND_CONSUMED = get_mode( stats,
                                          'RECEIVED_AND_CONSUMED' )
  local RECEIVED_AND_RETAINED = get_mode( stats,
                                          'RECEIVED_AND_RETAINED' )
  local total = NOT_RECEIVED + RECEIVED_AND_CONSUMED +
                    RECEIVED_AND_RETAINED
  local RECEIVED = total - NOT_RECEIVED
  return {
    NOT_RECEIVED=NOT_RECEIVED,
    RECEIVED=RECEIVED,
    RECEIVED_AND_CONSUMED=RECEIVED_AND_CONSUMED,
    RECEIVED_AND_RETAINED=RECEIVED_AND_RETAINED,
    total=total,
  }
end

local function tech_levels( tribes )
  local res = {}
  for _, t in ipairs( tribes ) do
    insert( res, assert( tribe_to_techlevel[t] ) )
  end
  return res
end

local function probabilities_for_outcome(main_config,
                                         global_stats, f )
  local data = {}
  for _, d in ipairs( main_config.difficulty ) do
    local row = {}
    insert( data, row )
    for _, t in ipairs( main_config.tribe_name ) do
      local exp_name = M.experiment_name{
        difficulty=d,
        tribe_name=t,
        brave=main_config.brave,
      }
      local stats = assert( global_stats[exp_name] )
      local fields = get_stats_fields( stats )
      local fraction = f( fields )
      local percent = floor( 100 * fraction + .5 )
      insert( row, format( '%d%%', percent ) )
    end
  end
  return data
end

local function table_def(main_config, global_stats, title,
                         calculator )
  local title_suffix = ' (Old Brave)'
  if main_config.brave == 'new' then
    title_suffix = ' (New Brave)'
  end
  return {
    title=title .. title_suffix,
    column_names=tech_levels( main_config.tribe_name ),
    row_names=main_config.difficulty,
    data=probabilities_for_outcome( main_config, global_stats,
                                    calculator ),
  }
end

local function probability_receives_muskets_table(main_config,
                                                  global_stats )
  local calculator = function( fields )
    return fields.RECEIVED / fields.total
  end
  return table_def( main_config, global_stats,
                    'Probability Receives Muskets', calculator )
end

local function probability_consumes_muskets_table(main_config,
                                                  global_stats )
  local calculator = function( fields )
    return fields.RECEIVED_AND_CONSUMED / fields.RECEIVED
  end
  return table_def( main_config, global_stats,
                    'Probability Consumes Muskets', calculator )
end

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
function M.experiment_name( config )
  local function assert_not_list( o )
    assert( o )
    assert( type( o ) ~= 'table' )
    return o
  end
  local tribe_name = assert_not_list( config.tribe_name )
  local techlevel = assert( tribe_to_techlevel[tribe_name] )
  local difficulty = assert_not_list( config.difficulty )
  local brave = assert_not_list( config.brave )
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

function M.summary_tables( main_config, global_stats )
  return {
    probability_receives_muskets_table( main_config, global_stats ),
    probability_consumes_muskets_table( main_config, global_stats ),
  }
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
