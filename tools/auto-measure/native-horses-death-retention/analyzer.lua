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
local tribes_all = {
  'inca', --
  'aztec', --
  'arawak', --
  'iroquois', --
  'cherokee', --
  'apache', --
  'sioux', --
  'tupi', --
}

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
  local NOT_DEFEATED = get_mode( stats, 'NOT_DEFEATED' )
  local DEFEATED_AND_SAVED = get_mode( stats,
                                       'DEFEATED_AND_SAVED' )
  local DEFEATED_AND_NOT_SAVED =
      get_mode( stats, 'DEFEATED_AND_NOT_SAVED' )
  local total = NOT_DEFEATED + DEFEATED_AND_SAVED +
                    DEFEATED_AND_NOT_SAVED
  local DEFEATED = total - NOT_DEFEATED
  return {
    NOT_DEFEATED=NOT_DEFEATED,
    DEFEATED=DEFEATED,
    DEFEATED_AND_SAVED=DEFEATED_AND_SAVED,
    DEFEATED_AND_NOT_SAVED=DEFEATED_AND_NOT_SAVED,
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
  return {
    title=title,
    column_names=tech_levels( main_config.tribe_name ),
    row_names=main_config.difficulty,
    data=probabilities_for_outcome( main_config, global_stats,
                                    calculator ),
  }
end

local function probability_saves_horses_table(main_config,
                                              global_stats )
  local calculator = function( fields )
    return fields.DEFEATED_AND_SAVED / fields.DEFEATED
  end
  return table_def( main_config, global_stats,
                    'Probability Saves Horses', calculator )
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
  return format( '%s-%s', difficulty, techlevel )
end

-- Called after the config settings are applied via setters. This
-- method will apply some general setup changes that are needed
-- regardless of config.
function M.post_config_setup( _, json )
  info( 'applying post-config setup.' )

  -- Check that we have a single dwelling and set its properties.
  local dwelling_count = 0
  on_all_dwellings( json, function( dwelling )
    dwelling_count = dwelling_count + 1
    dwelling.growth_counter = 0
    dwelling.BLCS.brave_missing = false
  end )
  assert( dwelling_count == 1 )

  -- Check that we have a single mounted brave.
  local brave_count = 0
  local mounted_brave_count = 0
  on_all_braves( json, function( brave )
    brave_count = brave_count + 1
    if brave.type == 'Mounted brave' then
      mounted_brave_count = mounted_brave_count + 1
    end
  end )
  assert( brave_count == 1 )
  assert( mounted_brave_count == 1 )

  on_all_tribes( json, function( _, tribe )
    tribe.muskets = 0
    tribe.horse_herds = 0
    tribe.horse_breeding = 0
  end )

  -- For each nation, set attitude=4 toward each tribe. This pre-
  -- vents the game from asking us if we want to attack the tribe
  -- the first time.
  for _, nation in ipairs( json.NATION ) do
    for _, relation in ipairs( nation.relation_by_indian ) do
      relation['attitude?'] = 4
    end
  end

  -- Set game options.
  json.HEADER.game_options.tutorial_hints = false
  json.HEADER.game_options.combat_analysis = false
  json.HEADER.game_options.autosave = false
  json.HEADER.game_options.end_of_turn = true
  json.HEADER.game_options.fast_piece_slide = true
  json.HEADER.game_options.show_foreign_moves = false
  json.HEADER.game_options.show_indian_moves = false
end

function M.action( api )
  local left = assert( api.left )

  -- This action is to be run when there is a mounted brave to
  -- the left of a european military unit that is waiting for or-
  -- ders.

  -- Make the european unit attack the mounted brave.
  sleep( 1.0 )
  left()
  sleep( 2.5 ) -- Attack animation.
  -- A message sometimes pops up giving results. Use the left key
  -- to close it, that way if there is no message it won't cause
  -- any issues, it'll just move the white selector square to the
  -- left, since presumably we've enabled end-of-turn.
  left()
  -- Allow for potential depixelation animation of attacker.
  sleep( 2.0 )
end

function M.collect_results( config, SAV )
  local tribe_name = assert( config.tribe_name )

  -- Count the number of mounted braves.
  local has_mounted_brave = false
  for _, unit in ipairs( SAV.UNIT ) do
    if unit.type == 'Mounted brave' then
      has_mounted_brave = true
      break
    end
  end

  -- Check the horse_breeding field.
  local tribe = get_tribe( SAV, tribe_name )
  local horse_breeding = assert( tribe.horse_breeding )
  assert( type( horse_breeding ) == 'number' )
  assert( horse_breeding >= 0 )
  assert( horse_breeding == 0 or horse_breeding == 25 )

  local mode
  if has_mounted_brave then
    mode = 'NOT_DEFEATED'
    assert( tribe.horse_breeding == 0 )
  else
    if horse_breeding > 0 then
      mode = 'DEFEATED_AND_SAVED'
    else
      mode = 'DEFEATED_AND_NOT_SAVED'
    end
  end

  -- Can add more keys here if needed.
  return { mode=assert( mode ) }
end

function M.summary_tables( main_config, global_stats )
  return {
    probability_saves_horses_table( main_config, global_stats ),
  }
end

-- There are no more free parameters in the config, so we only
-- have a single non-parametrized summary file name.
function M.summary_file( _ ) return 'summary.txt' end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
