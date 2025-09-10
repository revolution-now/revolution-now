#!/usr/bin/env lua

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local file = require'moon.file'
local logger = require'moon.logger'
local moon_tbl = require'moon.tbl'
local printer = require'moon.printer'
local setters = require'setters'
local str = require'moon.str'
local posix = require'posix'
local coldo = require'lib.coldo'
local readwrite = require'lib.readwrite-sav'
local xdotool = require'lib.xdotool'
local dosbox = require'lib.dosbox'
local libconfig = require'lib.config'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local rep = string.rep
local exit = os.exit
local getenv = os.getenv
local insert = table.insert

local append_string_to_file = file.append_string_to_file
local bar = printer.bar
local dbg = logger.dbg
local flatten_configs = libconfig.flatten_configs
local exists = file.exists
local info = logger.info
local on_ordered_kv = moon_tbl.on_ordered_kv
local format_data_table = printer.format_data_table
local printfln = printer.printfln
local read_file_lines = file.read_file_lines
local trim = str.trim
local remove_sav = readwrite.remove_sav
local read_sav = readwrite.read_sav
local write_sav = readwrite.write_sav
local path_for_sav = readwrite.path_for_sav
local save_game = coldo.save_game
local load_game = coldo.load_game
local exit_game = coldo.exit_game
local action_api = xdotool.action_api

local mkdir = posix.mkdir

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local INPUT_SAV_NUM = 6 -- COLONY06.SAV
local OUTPUT_SAV_NUM = 7 -- COLONY07.SAV

-----------------------------------------------------------------
-- Global Init.
-----------------------------------------------------------------
logger.level = logger.levels.INFO

-----------------------------------------------------------------
-- Result detection.
-----------------------------------------------------------------
local function record_outcome( args )
  local outfile = assert( args.outfile )
  local config = assert( args.config )
  local analyzer = assert( args.analyzer )
  local sav = read_sav( OUTPUT_SAV_NUM )
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
  local target_sav = path_for_sav( OUTPUT_SAV_NUM )
  for _ = 1, num_trials do
    info( 'starting trial %d...', stats.__count )
    load_game( INPUT_SAV_NUM )
    analyzer.action( action_api( dosbox.window() ) )
    -- First remove the file to which we will be saving as a
    -- simple way to verify that the file was actually saved. It
    -- is difficult to otherwise verify that directly because we
    -- have to trust that we successfully opened the game menu to
    -- save it, which can't know directly because we can't "see"
    -- the contents of the game window.
    remove_sav( OUTPUT_SAV_NUM )
    assert( not exists( target_sav ) )
    save_game( OUTPUT_SAV_NUM )
    assert( exists( target_sav ), 'failed to save file' )
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
  local json = read_sav( INPUT_SAV_NUM )
  for _, func in ipairs( setter_funcs ) do func( json ) end
  analyzer.post_config_setup( config, json )
  write_sav( INPUT_SAV_NUM, json )
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
  local scenarios_dir = format( '%s/scenarios', dir )
  mkdir( scenarios_dir )
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
-- Results summary table.
-----------------------------------------------------------------
local function print_results_summary(main_config, global_stats,
                                     analyzer, summary_path )
  local tbls =
      analyzer.summary_tables( main_config, global_stats )
  local out = assert( io.open( summary_path, 'w' ) )
  for _, tbl in ipairs( tbls ) do
    bar( '=' )
    print( tbl.title )
    bar( '=' )
    local tbl_str = format_data_table( tbl )
    print( tbl_str )
    out:write( tbl.title .. '\n' )
    out:write( rep( '=', 80 ) .. '\n' )
    out:write( tbl_str .. '\n\n' )
  end
  out:close()
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
  assert( analyzer.summary_file )

  local main_config = assert(
                          require( format( '%s.config', dir ) ) )
  assert( main_config.target_trials ) -- sanity check.
  local summary_file = assert(
                           analyzer.summary_file( main_config ) )
  local summary_path = format( '%s/%s', dir, summary_file )

  local configs = flatten_configs( main_config )

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
  print_results_summary( main_config, global_stats, analyzer,
                         summary_path )
  exit_game()
  return 0
end

-----------------------------------------------------------------
-- Loader.
-----------------------------------------------------------------
exit( assert( main{ ... } ) )