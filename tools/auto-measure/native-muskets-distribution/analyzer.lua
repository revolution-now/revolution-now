local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local time = require'moon.time'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format

local sleep = time.sleep

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
