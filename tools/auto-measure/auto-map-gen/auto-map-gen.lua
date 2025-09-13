-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local file = require'moon.file'
local logger = require'moon.logger'
local time = require'moon.time'
local cmd = require'moon.cmd'
local str = require'moon.str'
local coldo = require'lib.coldo'
local readwrite = require'lib.readwrite-sav'
local xdotool = require'lib.xdotool'
local dosbox = require'lib.dosbox'
local launch = require'lib.launch'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local exit = os.exit
local getenv = os.getenv

local exists = file.exists
local realpath = file.realpath
local info = logger.info
local sav_exists = readwrite.sav_exists
local path_for_sav = readwrite.path_for_sav
local save_game = coldo.save_game
local action_api = xdotool.action_api
local sleep = time.sleep
local forget_dosbox_window = dosbox.forget_dosbox_window
local command = cmd.command
local trim = str.trim

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local SAV_SLOT = 0 -- COLONY00.SAV

-----------------------------------------------------------------
-- Global Init.
-----------------------------------------------------------------
logger.level = logger.levels.INFO

local HOME = assert( getenv( 'HOME' ) )
local GAMEGEN = format(
                    '%s/dev/revolution-now/tools/auto-measure/auto-map-gen/gamegen',
                    HOME )
local COLONIZE = format(
                     '%s/games/colonization/data/MPS/COLONIZE',
                     HOME )
local VICEROY_EXE = format( '%s/VICEROY.EXE', COLONIZE )

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function validate_environment()
  assert( getenv( 'DISPLAY' ),
          'the DISPLAY environment variable must be set.' )
  assert( exists( GAMEGEN ),
          format( 'target folder %s does not exist.', GAMEGEN ) )
  assert( realpath( VICEROY_EXE ):match( 'VICEORIG' ),
          'VICEROY.EXE should be a symlink to VICEORIG.EXE' )

end

local function select_strength( level, actions )
  assert( level )
  assert( type( level ) == 'number' )
  if level == -1 then
    actions.up()
    return
  end
  if level == 0 then return end
  if level == 1 then
    actions.down()
    return
  end
  error( format( 'unsupported level: %s', tostring( level ) ) )
end

local function select_land_configuration( config, actions )
  -- Select Land Mass.
  select_strength( config.land_mass, actions )

  -- Select Land Form.
  actions.right()
  select_strength( config.land_form, actions )

  -- Select Temperature.
  actions.right()
  select_strength( config.temperature, actions )

  -- Select Climate.
  actions.right()
  select_strength( config.climate, actions )
end

local function generate_one_map( config )
  assert( not sav_exists( SAV_SLOT ), format(
              '%s must not exist.', path_for_sav( SAV_SLOT ) ) )

  forget_dosbox_window()
  local launcher<close> = launch.colonization_launcher()

  sleep( 3 )

  local actions = action_api( dosbox.window() )

  -- Select "customize new world."
  actions.down()
  actions.down()
  actions.enter()
  sleep( .3 ) -- wait for screen to change.

  -- Select land configuration.
  select_land_configuration( config, actions )
  actions.enter() -- next screen.

  -- Select conquistador.
  actions.right()
  actions.right()
  actions.enter()

  -- Select french.
  actions.right()
  actions.enter()
  actions.enter() -- confirm name.
  actions.enter() -- skip "about france" screen.
  actions.enter() -- skip "special abilities" screen.
  actions.enter() -- skip "year of our lord" screen.

  -- Harbor enpixelates.
  sleep( 1 )
  actions.enter() -- leave harbor when map-gen is done.
  actions.enter() -- leave harbor when map-gen is done.

  -- Wait for map gen to finish.
  sleep( 11 )

  -- Land view is now visible.
  save_game( SAV_SLOT )

  sleep( 1 )
end

local function make_config_dir( config )
  local function level( of )
    assert( of )
    if of == 1 then return 'b' end
    if of == 0 then return 'm' end
    if of == -1 then return 't' end
    error( format( 'unsupported level: ', of ) )
  end
  return format( '%s%s%s%s', level( config.land_mass ),
                 level( config.land_form ),
                 level( config.temperature ),
                 level( config.climate ) )
end

-----------------------------------------------------------------
-- main.
-----------------------------------------------------------------
local function main()
  validate_environment()

  local config = require( 'config' )
  assert( config )
  local subdir = make_config_dir( config )
  local output_dir = format( '%s/config/%s', GAMEGEN, subdir )
  command( 'mkdir', '-p', output_dir )
  local target_count = assert( config.target_count )
  local have_count = tonumber( trim(
                                   command( format(
                                                'ls %s | wc -l',
                                                output_dir ) ) ) )
  local need_count = target_count - have_count
  if need_count <= 0 then
    info( 'all files generated.' )
    exit( 0 )
  end
  for i = 1, need_count do
    info( 'generating %d/%d...', i, need_count )
    generate_one_map( config )
    assert( sav_exists( SAV_SLOT ) )
    local sav_src = path_for_sav( SAV_SLOT )
    local sav_dst = format( '%s/COLONY00.SAV.%d', output_dir,
                            os.time() )
    assert( os.rename( sav_src, sav_dst ), format(
                'failed to move %s to %s', sav_src, sav_dst ) )
    assert( not sav_exists( SAV_SLOT ) )
    sleep( 2 )
  end

  info( 'generated %d maps.', need_count )
  info( 'finished.' )
  return 0
end

-----------------------------------------------------------------
-- Loader.
-----------------------------------------------------------------
exit( assert( main() ) )