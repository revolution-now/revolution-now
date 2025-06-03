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
-- TODO

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
