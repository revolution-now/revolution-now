-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local time = require'moon.time'
local logger = require'moon.logger'
local moontbl = require'moon.tbl'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local insert = table.insert
local floor = math.floor

local sleep = time.sleep
local info = logger.info
local on_ordered_kv = moontbl.on_ordered_kv

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
      local exp_name = experiment_name{
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

-- TODO: move to moonlib and dedupe.
local function format_kv_table( tbl, args )
  local start = args.start or ''
  local ending = args.ending or ''
  local kv_sep = args.kv_sep or '='
  local pair_sep = args.pair_sep or ','
  pair_sep = ''
  local line = ''
  on_ordered_kv( tbl, function( k, v )
    line = format( '%s%s%s%s%s', line, pair_sep, k, kv_sep,
                   tostring( v ) )
    pair_sep = args.pair_sep or ','
  end )
  return format( '%s%s%s', start, line, ending )
end

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
local function experiment_name( config )
  return format_kv_table( config, {
    start='[',
    ending=']',
    kv_sep='=',
    pair_sep='|',
  } )
end

local function set_config( config, json )
  -- TODO
end

local function action( api )
  -- local enter = assert( api.enter )
  -- enter()
  -- sleep( 1.0 )
end

local deleteme = 0

local function collect_results( json )
  deleteme = deleteme + 1
  -- Can add more keys here if needed.
  return { ref_selection=deleteme }
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  experiment_name=experiment_name,
  set_config=set_config,
  action=action,
  collect_results=collect_results,
}
