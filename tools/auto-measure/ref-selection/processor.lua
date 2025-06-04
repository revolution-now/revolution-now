-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local time = require'moon.time'
local logger = require'moon.logger'
local printer = require'moon.printer'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local insert = table.insert
local floor = math.floor

local sleep = time.sleep
local info = logger.info
local format_kv_table = printer.format_kv_table

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local INITIAL_UNIT_STORE_COUNT = 10

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
-- TODO

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

local function validate_names_txt( lines )
  -- TODO:
  --   * Man-o-war has movement 0.
  --   * Man-o-war has zero attack/combat.
  return true
end

-- This does a one-time validation of the original input sav file
-- before it is modified in any way. But this is only done once
-- at the start of the run, and is not done on each config. The
-- set_config method below is responsible for performing any val-
-- idation that needs to be done after a particular config is set
-- into the sav file.
local function validate_sav( json )
  -- TODO:
  --   * fast piece slide enabled.
  --   * end of turn enabled.
  --   * independence declared.
  --   * No units.
  --   * Correct number of REF units in store
  --     (=INITIAL_UNIT_STORE_COUNT).
  --   * One colony.
  --   * One AI (REF) player, one human player.
  return true
end

local function set_config( config, json )
  -- TODO
  --   * Put the white box over the colony so that as we're
  --     watching it we can roughly see what units are in there.
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

    -- The ship should always be damaged because it should have
    -- zero combat/attack in NAMES.TXT as a prereq for this test.
    -- In that case, a box pops up saying that was damaged.
    close_box()
  end

  -- "Your Excellency, the King's armies have little experience..."
  close_box()

  -- "Spain is considering intervention..."
  close_box()

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
  regulars = INITIAL_UNIT_STORE_COUNT - regulars
  cavalry = INITIAL_UNIT_STORE_COUNT - cavalry
  artillery = INITIAL_UNIT_STORE_COUNT - artillery
  assert( regulars >= 0 )
  assert( cavalry >= 0 )
  assert( artillery >= 0 )
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
}
