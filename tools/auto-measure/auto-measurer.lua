#!/usr/bin/env lua

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local cmd = require'moon.cmd'
local file = require'moon.file'
local logger = require'moon.logger'
local moon_tbl = require'moon.tbl'
local printer = require'moon.printer'
local sav_reader = require'sav.conversion.sav-reader'
local sav_writer = require'sav.conversion.sav-writer'
local setters = require'setters'
local str = require'moon.str'
local time = require'moon.time'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local exit = os.exit
local getenv = os.getenv
local insert = table.insert

local append_string_to_file = file.append_string_to_file
local bar = printer.bar
local command = cmd.command
local dbg = logger.dbg
local deep_copy = moon_tbl.deep_copy
local exists = file.exists
local info = logger.info
local on_ordered_kv = moon_tbl.on_ordered_kv
local print_data_table = printer.print_data_table
local printfln = printer.printfln
local read_file_lines = file.read_file_lines
local sleep = time.sleep
local trim = str.trim

assert( sav_reader.load )
assert( sav_writer.save )

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local KEY_DELAY = 50

logger.level = logger.levels.INFO

-----------------------------------------------------------------
-- Folders.
-----------------------------------------------------------------
local HOME = assert( getenv( 'HOME' ) )
local COLONIZE = format(
                     '%s/games/colonization/data/MPS/COLONIZE',
                     HOME )
local RN = format( '%s/dev/revolution-now', HOME )
local SAV = format( '%s/tools/sav', RN )

info( 'HOME:' .. HOME )
info( 'COLONIZE:' .. COLONIZE )
info( 'RN:' .. RN )

-----------------------------------------------------------------
-- xdotool
-----------------------------------------------------------------
local function xdotool( ... )
  return trim( command( 'xdotool', ... ) )
end

local function find_window_named( regex )
  info( 'searching for window with name ' .. regex )
  return xdotool( 'search', '--name', regex )
end

local __dosbox = nil

local function find_dosbox()
  local success, res = pcall( find_window_named,
                              'DOSBox.*VICEROY' )
  if success then return res end
end

local function dosbox()
  if not __dosbox then __dosbox = assert( find_dosbox() ) end
  return __dosbox
end

-----------------------------------------------------------------
-- General X commands.
-----------------------------------------------------------------
local function press_keys( ... )
  xdotool( 'key', '--delay=' .. KEY_DELAY, '--window', dosbox(),
           ... )
end

-----------------------------------------------------------------
-- Individual keys.
-----------------------------------------------------------------
local function down() press_keys'Down' end
local function up() press_keys'Up' end
local function left() press_keys'Left' end
local function right() press_keys'Right' end
local function enter() press_keys'Return' end

local function seq( tbl )
  for _, action in ipairs( tbl ) do action() end
end

local action_api = {
  press_keys=press_keys,
  down=down,
  up=up,
  left=left,
  right=right,
  enter=enter,
  seq=seq,
}

-----------------------------------------------------------------
-- Game specific composite commands.
-----------------------------------------------------------------
local function game_menu() press_keys( 'alt+g' ) end

-- Loads the game from COLONY07.SAV.
local function load_game()
  info( 'loading COLONY07.SAV...' )
  press_keys( 'slash' ) -- requires configuration in MENU.TXT
  seq{ up, up, up, enter } -- Select COLONY07.SAV.
  sleep( .5 ) -- Wait for game to load.
  enter() -- Close popup.
end

-- Saves the game to COLONY06.SAV.
local function save_game()
  info( 'saving COLONY06.SAV...' )
  press_keys( 'apostrophe' ) -- requires configuration in MENU.TXT
  seq{ up, up, enter } -- Select COLONY06.SAV.
  sleep( 1 ) -- Wait for game to save.
  enter() -- Close popup.
end

local function exit_game()
  -- Do nothing if the DOSBox window is not open.
  if not find_dosbox() then return end
  info( 'exiting game.' )
  game_menu()
  seq{ up, enter } -- Select "Exit to DOS".
  down() -- Highlight 'Yes'.
  -- Somehow, even though this succeeds in ending the program,
  -- the xdotool returns an error.
  pcall( xdotool, 'key', '--window', dosbox(), 'Return', '2>&1' )
end

-----------------------------------------------------------------
-- Read/Write SAV.
-----------------------------------------------------------------
local function read_sav( name )
  return sav_reader.load{
    structure_json=format( '%s/schema/sav-structure.json', SAV ),
    colony_sav=format( '%s/%s', COLONIZE, name ),
  }
end

local function write_sav( name, json )
  sav_writer.save{
    structure_json=format( '%s/schema/sav-structure.json', SAV ),
    colony_json=json,
    colony_sav=format( '%s/%s', COLONIZE, name ),
  }
end

-----------------------------------------------------------------
-- Result detection.
-----------------------------------------------------------------
local function record_outcome( args )
  local outfile = assert( args.outfile )
  local config = assert( args.config )
  local analyzer = assert( args.analyzer )
  local sav = read_sav( 'COLONY06.SAV' )
  assert( sav.HEADER )

  -- This invokes test-case-specific logic.
  local results = analyzer.collect_results( config, sav )

  local line = ''
  on_ordered_kv( results, function( k, v )
    line = format( '%s%s=%s ', line, k, tostring( v ) )
  end )
  line = trim( line )
  info( 'received line: %s', line )

  local stats = assert( args.stats )
  stats[line] = stats[line] or 0
  stats[line] = stats[line] + 1
  stats.__count = stats.__count + 1

  append_string_to_file( outfile, line .. '\n' )
end

-----------------------------------------------------------------
-- Main loop.
-----------------------------------------------------------------
local function print_stats( config, stats, exp_name )
  bar()
  local total = 0
  local longest = 0
  for k, v in pairs( stats ) do
    if k:match( '__' ) then goto continue end
    total = total + v
    if #k > longest then longest = #k end
    ::continue::
  end
  printfln( 'Stats (%d/%d trials): %s', stats.__count,
            config.target_trials, exp_name )
  bar()
  longest = longest + 2 -- colon plus space
  local fmt = format( '%%-%ds%%.1f%%%%', longest )
  on_ordered_kv( stats, function( k, v )
    if k:match( '__' ) then return end
    printfln( fmt, k .. ':', 100 * v / total )
  end )
  bar()
end

local function loop( args )
  local config = assert( args.config )
  local stats = assert( args.stats )
  local num_trials = assert( args.num_trials )
  local outfile = assert( args.outfile )
  local analyzer = assert( args.analyzer )
  local exp_name = assert( args.exp_name )
  info( 'running for %s...', exp_name )
  for _ = 1, num_trials do
    info( 'starting trial %d...', stats.__count )
    load_game()
    analyzer.action( action_api )
    save_game()
    record_outcome{
      analyzer=analyzer,
      config=config,
      stats=stats,
      outfile=outfile,
    }
    print_stats( config, stats, exp_name )
  end
end

-----------------------------------------------------------------
-- Config setter.
-----------------------------------------------------------------
local function set_config( config, analyzer )
  local setter_funcs = {}
  for k, v in pairs( config ) do
    local setter = setters[k]
    if setter then
      assert( type( setter ) == 'function' )
      local wrapper = function( json )
        info( 'invoking setter for "%s"...', k )
        setter( json, v )
      end
      insert( setter_funcs, wrapper )
    end
  end
  local sav_file_name = 'COLONY07.SAV'
  local json = read_sav( sav_file_name )
  for _, func in ipairs( setter_funcs ) do func( json ) end
  analyzer.post_config_setup( config, json )
  write_sav( sav_file_name, json )
end

-----------------------------------------------------------------
-- Driver for single run.
-----------------------------------------------------------------
local function run_single( args )
  local dir = assert( args.dir )
  local analyzer = assert( args.analyzer )
  local config = assert( args.config )

  dbg( 'Config:' )
  for k, v in pairs( config ) do dbg( '-%s=%s', k, v ) end

  local num_trials = config.target_trials
  local stats = { __count=0 }
  local exp_name = analyzer.experiment_name( config )
  local outfile = format( '%s/scenarios/%s.txt', dir, exp_name )
  if exists( outfile ) then
    local existing = read_file_lines( outfile )
    stats.__count = #existing
    for _, line in ipairs( existing ) do
      stats[line] = stats[line] or 0
      stats[line] = stats[line] + 1
    end
    if #existing >= config.target_trials then
      info( 'Target trials already achieved. exiting.' )
      print_stats( config, stats, exp_name )
      return stats
    end
    num_trials = config.target_trials - #existing
    print_stats( config, stats, exp_name )
    assert( num_trials > 0 )
    info( 'Partial results found. Running %d more times.',
          num_trials )
  end

  set_config( config, analyzer )
  loop{
    config=config,
    stats=stats,
    num_trials=num_trials,
    outfile=outfile,
    analyzer=analyzer,
    exp_name=exp_name,
  }
  info( 'finished.' )
  return stats
end

-----------------------------------------------------------------
-- Config flattener.
-----------------------------------------------------------------
local function flatten_configs( master )
  local res = { {} }
  for k, v in pairs( master ) do
    if type( v ) ~= 'table' then v = { v } end
    local new_res = {}
    for _, elem in ipairs( v ) do
      local copy = deep_copy( res )
      for _, config in ipairs( copy ) do
        config[k] = elem
        insert( new_res, config )
      end
    end
    res = new_res
  end
  return res
end

-----------------------------------------------------------------
-- Results summary table.
-----------------------------------------------------------------
local function print_results_summary(master_config, global_stats,
                                     analyzer )
  local tbls = analyzer.summary_tables( master_config,
                                        global_stats )
  for _, tbl in ipairs( tbls ) do
    bar( '=' )
    print( tbl.title )
    bar( '=' )
    print_data_table( tbl )
  end
end

-----------------------------------------------------------------
-- main.
-----------------------------------------------------------------
local function main( args )
  assert( getenv( 'DISPLAY' ),
          'the DISPLAY environment variable must be set.' )

  local dir = assert( args[1] )

  local analyzer = require( format( '%s.analyzer', dir ) )
  assert( analyzer.experiment_name )
  assert( analyzer.post_config_setup )
  assert( analyzer.collect_results )
  assert( analyzer.action )
  assert( analyzer.summary_tables )

  local master_config = assert( require(
                                    format( '%s.config', dir ) ) )
  assert( master_config.target_trials ) -- sanity check.

  local configs = flatten_configs( master_config )

  local global_stats = {}
  for _, config in ipairs( configs ) do
    local stats = run_single{
      dir=dir,
      analyzer=analyzer,
      config=config,
    }
    local exp_name = analyzer.experiment_name( config )
    global_stats[exp_name] = stats
  end
  print_results_summary( master_config, global_stats, analyzer )
  exit_game()
  return 0
end

-----------------------------------------------------------------
-- Loader.
-----------------------------------------------------------------
exit( assert( main( { ... } ) ) )