#!/usr/bin/env lua

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local file = require'moon.file'
local logger = require'moon.logger'
local printer = require'moon.printer'
local coldo = require'lib.coldo'
local readwrite = require'lib.readwrite-sav'
local xdotool = require'lib.xdotool'
local dosbox = require'lib.dosbox'
local libconfig = require'lib.config'
local names = require'lib.names'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local exit = os.exit
local getenv = os.getenv
local insert = table.insert

local append_string_to_file = file.append_string_to_file
local dbg = logger.dbg
local flatten_configs = libconfig.flatten_configs
local exists = file.exists
local info = logger.info
local read_file_lines = file.read_file_lines
local remove_sav = readwrite.remove_sav
local read_sav = readwrite.read_sav
local copy_sav = readwrite.copy_sav
local modify_sav = readwrite.modify_sav
local path_for_sav = readwrite.path_for_sav
local names_txt_path = readwrite.names_txt_path
local save_game = coldo.save_game
local load_game = coldo.load_game
local action_api = xdotool.action_api
local format_kv_table = printer.format_kv_table
local pause_dosbox = dosbox.pause

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
-- The way it works is that we read in the template, modify it
-- with the config, then save that as the "start" file. Then the
-- experiment's actions are performed and the result is saved as
-- the "finish" file.
local FINISH_SAV_NUM = 0 -- COLONY00.SAV
local START_SAV_NUM = 1 -- COLONY01.SAV
local TEMPLATE_SAV_NUM = 2 -- COLONY02.SAV

-----------------------------------------------------------------
-- Global Init.
-----------------------------------------------------------------
logger.level = logger.levels.INFO

-----------------------------------------------------------------
-- Validation.
-----------------------------------------------------------------
local function validate_template_sav( processor )
  local json = read_sav( TEMPLATE_SAV_NUM )
  assert( json.HEADER )
  assert( processor.validate_sav( json ) )
end

-----------------------------------------------------------------
-- Result detection.
-----------------------------------------------------------------
local function record_outcome( args )
  local outfile = assert( args.outfile )
  local _ = assert( args.config )
  local processor = assert( args.processor )
  local exp_name = assert( args.exp_name )

  local json = read_sav( FINISH_SAV_NUM )
  assert( json.HEADER )

  -- This invokes test-case-specific logic.
  local results = processor.collect_results( json )
  local value = format_kv_table( results, {
    start='',
    ending='',
    kv_sep='=',
    pair_sep='|',
  } )

  info( 'case: %s', exp_name )
  info( 'result: %s', value )
  local line = format( '%s\t%s', exp_name, value )
  append_string_to_file( outfile, line .. '\n' )
end

-----------------------------------------------------------------
-- Run a single config.
-----------------------------------------------------------------
local function run_config( args )
  local config = assert( args.config )
  local outfile = assert( args.outfile )
  local processor = assert( args.processor )
  local exp_name = assert( args.exp_name )

  processor.validate_config( config )

  remove_sav( START_SAV_NUM )
  copy_sav( TEMPLATE_SAV_NUM, START_SAV_NUM )

  -- Set the current config in the input SAV file.
  modify_sav( START_SAV_NUM, function( json )
    processor.set_config( config, json )
  end )

  load_game( START_SAV_NUM )
  processor.action( config, action_api( dosbox.window() ) )
  -- First remove the file to which we will be saving as a simple
  -- way to verify that the file was actually saved. It is diffi-
  -- cult to otherwise verify that directly because we have to
  -- trust that we successfully opened the game menu to save it,
  -- which can't know directly because we can't "see" the con-
  -- tents of the game window.
  remove_sav( FINISH_SAV_NUM )
  local target_sav = path_for_sav( FINISH_SAV_NUM )
  assert( not exists( target_sav ) )
  save_game( FINISH_SAV_NUM )
  assert( exists( target_sav ),
          'failed to save file ' .. target_sav )
  record_outcome{
    config=config,
    outfile=outfile,
    processor=processor,
    exp_name=exp_name,
  }
end

-----------------------------------------------------------------
-- Run through all combinations of configs.
-----------------------------------------------------------------
local function run_configs( args )
  local dir = assert( args.dir )
  local processor = assert( args.processor )
  local configs = assert( args.configs )

  local results = {}

  local outfile = format( '%s/results.txt', dir )
  local already_run = {}
  if exists( outfile ) then
    local existing = read_file_lines( outfile )
    for i, line in ipairs( existing ) do
      dbg( 'parsing existing line %d', i )
      local iter = assert( line:gmatch( '[^\t]+' ) )
      local exp_name = assert( iter() )
      local result = assert( iter() )
      assert( not iter() ) -- no more items.
      assert( not already_run[exp_name] ) -- no duplicate keys.
      already_run[exp_name] = true
      insert( results, { exp_name=exp_name, result=result } )
    end
  end

  local need_to_run = {}
  for _, config in ipairs( configs ) do
    local exp_name = processor.experiment_name( config )
    if not already_run[exp_name] then
      insert( need_to_run, { exp_name=exp_name, config=config } )
    end
  end

  if #need_to_run == 0 then
    info( 'All configs have already been run. Exiting.' )
    return results
  end
  info( 'Partial results found. Running %d more configs.',
        #need_to_run )

  for i, elem in ipairs( need_to_run ) do
    local exp_name = assert( elem.exp_name )
    local config = assert( elem.config )
    info( 'running config [%d/%d]: %s...', i, #need_to_run,
          exp_name )
    local result = run_config{
      config=config,
      outfile=outfile,
      processor=processor,
      exp_name=exp_name,
    }
    insert( results, { exp_name=exp_name, result=result } )
  end
  info( 'finished.' )
  return results
end

-----------------------------------------------------------------
-- main.
-----------------------------------------------------------------
local function main( args )
  assert( getenv( 'DISPLAY' ),
          'the DISPLAY environment variable must be set.' )

  local dir = assert( args[1] )

  local processor = require( format( '%s.processor', dir ) )
  assert( processor.experiment_name )
  assert( processor.validate_sav )
  assert( processor.validate_names_txt )
  assert( processor.set_config )
  assert( processor.action )
  assert( processor.collect_results )

  local main_config = assert(
                          require( format( '%s.config', dir ) ) )
  assert( main_config.combinatorial ) -- sanity check.
  assert( main_config.type == 'deterministic' ) -- sanity check.
  local configs = flatten_configs( main_config.combinatorial )

  processor.validate_names_txt( names.parse( names_txt_path() ) )

  validate_template_sav( processor )

  run_configs{ dir=dir, processor=processor, configs=configs }

  pause_dosbox()
  return 0
end

-----------------------------------------------------------------
-- Loader.
-----------------------------------------------------------------
exit( assert( main{ ... } ) )