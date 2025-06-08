-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local time = require'moon.time'
local logger = require'moon.logger'
local printer = require'moon.printer'
local designer = require'lib.designer'
local mtbl = require'moon.tbl'
local list = require'moon.list'
local set = require'moon.set'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
-- local insert = table.insert
-- local floor = math.floor

local sleep = time.sleep
local info = logger.info
local format_kv_table = printer.format_kv_table
local deep_copy = mtbl.deep_copy
local join = list.join
local set_size = set.set_size

local D = designer

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local INITIAL_UNIT_STORE_COUNT = 10

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
local function validate_config( config )
  assert( type( config.difficulty ) == 'string' )
  assert( type( config.fortification ) == 'string' )
  assert( type( config.muskets ) == 'number' )
  assert( type( config.horses ) == 'number' )
  assert( type( config.already_landed ) == 'boolean' )
  assert( type( config.orders ) == 'string' )
  assert( type( config.unit_set ) == 'table' )
end

local function experiment_name( config )
  local copy = deep_copy( config )
  local unique_units = set_size( set( config.unit_set ) )
  if #config.unit_set > 5 and unique_units == 1 then
    copy.unit_set = format( '%s*%d', config.unit_set[1],
                            #config.unit_set )
  else
    copy.unit_set = join( config.unit_set, '-' )
  end
  return format_kv_table( copy, {
    start='',
    ending='',
    kv_sep='=',
    pair_sep='|',
  } )
end

local function validate_names_txt( names )
  local man_o_war = assert( names.UNIT['Man-O-War'] )
  -- Needed so that we don't have to wait for the tory man-o-war
  -- to move around after dropping off the troops.
  assert( man_o_war.movement == 0 )
  -- Needed in case there is a fort in order to guarantee that
  -- the man-o-war loses when it will inevitably be fired upon,
  -- in order to add determinacy.
  assert( man_o_war.combat == 0 )
  assert( man_o_war.attack == 0 )

  return true
end

-- This does a one-time validation of the original input sav file
-- before it is modified in any way. But this is only done once
-- at the start of the run, and is not done on each config. The
-- set_config method below is responsible for performing any val-
-- idation that needs to be done after a particular config is set
-- into the sav file.
local function validate_sav( json )
  assert( json.HEADER.game_flags_1.fast_piece_slide )
  assert( json.HEADER.game_flags_1.end_of_turn )
  assert( json.HEADER.game_flags_1.independence_declared )
  assert( not json.HEADER.game_flags_1.combat_analysis )
  -- This must be false otherwise some additional popups will ap-
  -- pear that will throw things off.
  assert( not json.HEADER.game_flags_1.independence_war_intro )

  assert( json.HEADER.end_of_turn_sign == 'Flashing' )

  assert( #json.UNIT == 0 )

  assert( json.HEADER.colony_count == 1 )
  assert( #json.COLONY == 1 )
  local colony = json.COLONY[1]

  -- Check colony has warehouse and that it has a high enough
  -- level to hold the quantities of horses/muskets that we may
  -- need to put there. If the warehouse level is not high enough
  -- then we might get a popup saying that some quantity has
  -- spoiled, which will mess things up.
  assert( colony.buildings.warehouse )
  assert( colony.warehouse_level == 255 )

  assert( D.find_REF( json ) )

  local SC = INITIAL_UNIT_STORE_COUNT
  assert( json.HEADER.expeditionary_force['regulars'] == SC )
  assert( json.HEADER.expeditionary_force['dragoons'] == SC )
  assert( json.HEADER.expeditionary_force['man-o-wars'] == SC )
  assert( json.HEADER.expeditionary_force['artillery'] == SC )

  local human_idx = assert( D.find_unique_human( json ) )
  local human_player = assert( json.PLAYER[human_idx] )
  assert( human_player.player_flags.named_new_world )
  return true
end

local function set_config( config, json )
  local colony = assert( json.COLONY[1] )
  local colony_coord = D.coord_for_colony( colony )
  local ref_idx = assert( D.find_REF( json ) )
  local human_idx = assert( D.find_unique_human( json ) )

  -- Put the white box over the colony so that as we're watching
  -- it we can roughly see what units are in there.
  D.set_white_box( json, colony_coord )

  -- difficulty.
  local difficulty = assert( config.difficulty )
  info( 'setting difficulty to "%s".', difficulty )
  D.set_difficulty( json, difficulty )

  -- fortifications.
  local fortification = assert( config.fortification )
  D.set_colony_fortification( colony, fortification )

  -- horses/muskets.
  local horses = assert( config.horses )
  local muskets = assert( config.muskets )
  D.set_colony_stock( colony, 'horses', horses )
  D.set_colony_stock( colony, 'muskets', muskets )

  -- unit_set.
  local unit_opts = { orders=config.orders, finished_turn=true }
  for _, unit in ipairs( config.unit_set ) do
    D.add_unit_map( json, unit, human_idx, colony_coord,
                    unit_opts )
  end

  -- already_landed. NOTE: need to do this after placing the
  -- units since placing the units can sometimes affect the vis-
  -- itor nation of surrounding tiles.
  local already_landed = config.already_landed
  assert( already_landed ~= nil )
  local visitor_nation = already_landed and ref_idx or human_idx
  D.on_tiles_around_colony( colony, function( tile )
    D.set_visitor_nation( json, tile, visitor_nation )
  end )
end

local function action( config, api )
  -- We're supposed to be starting with the end of turn sign
  -- blinking, ready to go to the next turn where the REF will
  -- land next to our only colony.

  local function close_box()
    -- Use left instead of space or enter because it is safer, in
    -- the sense that, just in case something goes wrong and
    -- there is no box open, hitting the left key will not do any
    -- damage; at most will just move the white box.
    api.left()
  end

  -- This seems necessary in order for the space bar that we'll
  -- hit next to end the turn.
  api.press_keys( 'm' )

  -- End the turn to cause the REF to land.
  api.space()

  -- Allow a bit of time for the REF turn to start.
  sleep( 0.3 )

  -- Close the box that tells us the REF are landing.
  close_box()

  -- Wait for units to unload. This should be enough if "fast
  -- piece slide" is enabled.
  sleep( 2.0 )

  -- "Your Excellency, the King's forces control..."
  close_box()

  local fortification = assert( config.fortification )
  if fortification == 'fort' or fortification == 'fortress' then
    -- "Fortress opens fire on Tory Man-o-War."
    close_box()

    -- Wait for the firing to happen.
    sleep( 0.75 )

    -- The ship should always be damaged because it should have
    -- zero combat/attack in NAMES.TXT as a prereq for this test.
    -- In that case, a box pops up saying that was damaged.
    close_box()

    -- Wait for the ship depixelation animation.
    sleep( 1.5 )
  end

  -- "Your Excellency, the King's armies have little experience..."
  -- close_box()

  -- "Spain is considering intervention..."
  -- close_box()

  -- This should leave us at our end of turn and the white box
  -- visible.

  -- Now when the file is saved, we just check the number of REF
  -- units in store that have been subtracted.
end

local function collect_results( json )
  local force = assert( json.HEADER.expeditionary_force )
  local regulars = assert( force.regulars )
  local cavalry = assert( force.dragoons )
  local artillery = assert( force.artillery )
  local ships = assert( force['man-o-wars'] )
  regulars = INITIAL_UNIT_STORE_COUNT - regulars
  cavalry = INITIAL_UNIT_STORE_COUNT - cavalry
  artillery = INITIAL_UNIT_STORE_COUNT - artillery
  ships = INITIAL_UNIT_STORE_COUNT - ships
  assert( regulars >= 0 )
  assert( cavalry >= 0 )
  assert( artillery >= 0 )
  assert( ships == 1 )
  return {
    ref_selection=format( '%d/%d/%d', regulars, cavalry,
                          artillery ),
  }
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  experiment_name=experiment_name,
  set_config=set_config,
  action=action,
  collect_results=collect_results,
  validate_sav=validate_sav,
  validate_names_txt=validate_names_txt,
  validate_config=validate_config,
}
