--[[ ------------------------------------------------------------
|
| price-group.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-08-27.
|
| Description: Console UI tool for testing the processed goods
|              price group model from the original game.
|
--]] ------------------------------------------------------------
package.path = 'src/lua/?.lua'

-----------------------------------------------------------------
-- Imports
-----------------------------------------------------------------
local price_group = require( 'prices.price-group' )

local floor = math.floor
local format = string.format

-----------------------------------------------------------------
-- Config
-----------------------------------------------------------------
local GOODS = { 'rum', 'cigars', 'cloth', 'coats' }
local VOLUME_INIT_CONFIG = { center=700, window=380 }
local TARGET_PRICE = 12

-- local STARTING_PRICES = { rum=11, cigars=10, cloth=14, coats=9 }
local STARTING_EURO_VOLUMES = {
  -- 11/12 10/11 14/15 9/10
  rum=0x02a9,
  cigars=0x02c6,
  cloth=0x0224,
  coats=0x033c

  -- 12/13 9/10 14/15 8/9
  -- rum=0x1f3,
  -- cigars=0x277,
  -- cloth=0x1c6,
  -- coats=0x2b5
}
local INITIAL_CMD = 'e'
local DUTCH = false

local PRICE_GROUP_CONFIG = {
  names={ 'rum', 'cigars', 'cloth', 'coats' },
  dutch=DUTCH,
  starting_euro_volumes=STARTING_EURO_VOLUMES,
  min=1,
  max=20,
  rise_fall=4,
  volatility=1,
  target_price=12
}

-----------------------------------------------------------------
-- UI State
-----------------------------------------------------------------
local gold = 0
local num_turns = 0
local num_actions = 0
local last_cmd = INITIAL_CMD

local function reset( group )
  group:reset()
  num_turns = 0
  num_actions = 0
  last_cmd = INITIAL_CMD
  gold = 0
end

-----------------------------------------------------------------
-- Prompt
-----------------------------------------------------------------
local chart = [[
  turns:   %d
  actions: %d
  gold:    %d
  ----------------------------------------------------------
  |     #1      |      #2      |     #3      |     #4      |
  ----------------------------------------------------------
  |      %2d     |      %2d      |      %2d     |      %2d     | < initial prices
  |    %s    |    %s     |    %s    |    %s    | < euro volumes
  |  %6d     |  %6d      |  %6d     |  %6d     | < traded volumes
  |     %4s    |     %4s     |     %4s    |     %4s    | < eq prices
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |    %2d/%2d    |    %2d/%2d     |    %2d/%2d    |    %2d/%2d    | < prices
  ----------------------------------------------------------
  last cmd: %s (will be repeated by hitting enter)

  b1: buy rum    s1: sell rum    b2: buy cigars  s2: sell cigars
  b3: buy cloth  s3: sell cloth  b4: buy coats   s4: sell coats
  ba: buy all    sa: sell all
  e to evolve one turn          E to evolve 1000 times.
  r to reset to starting state  <enter> to repeat last command.
  # <cmd> to run <cmd> # times, e.g. "12 s3".

> ]]

local function format_hex32( x )
  local res = '+'
  if x < 0 then
    res = '-'
    x = -x
  end
  x = floor( x )
  return res .. format( '%04X', x )
end

local function clear_screen()
  -- 27 is '\033'.
  io.write( string.char( 27 ) .. '[2J' ) -- clear screen
  io.write( string.char( 27 ) .. '[H' ) -- move cursor to upper left.
end

local function redraw( group )
  clear_screen()
  local eqs = group:equilibrium_prices()
  local starting_prices = group.config.starting_prices
  local euro_volumes = group.euro_volumes
  local traded_volumes = group.traded_volumes
  local prices = group.prices
  io.write( format( chart, num_turns, num_actions, gold,
                    floor( starting_prices.rum ),
                    floor( starting_prices.cigars ),
                    floor( starting_prices.cloth ),
                    floor( starting_prices.coats ),
                    format_hex32( euro_volumes.rum ),
                    format_hex32( euro_volumes.cigars ),
                    format_hex32( euro_volumes.cloth ),
                    format_hex32( euro_volumes.coats ),
                    traded_volumes.rum, traded_volumes.cigars,
                    traded_volumes.cloth, traded_volumes.coats,
                    format( '%.1f', eqs.rum ),
                    format( '%.1f', eqs.cigars ),
                    format( '%.1f', eqs.cloth ),
                    format( '%.1f', eqs.coats ), prices.rum - 1,
                    prices.rum, prices.cigars - 1, prices.cigars,
                    prices.cloth - 1, prices.cloth,
                    prices.coats - 1, prices.coats,
                    last_cmd or 'none' ) )
end

-----------------------------------------------------------------
-- Command Interpreter
-----------------------------------------------------------------
local function char( str, idx ) return
    string.sub( str, idx, idx ) end

local function split( str )
  local words = {}
  for word in str:gmatch( '%S+' ) do table.insert( words, word ) end
  return words
end

local function split_commas( str )
  local words = {}
  for word in str:gmatch( '[^,]+' ) do
    table.insert( words, word )
  end
  return words
end

local function parse_cmds( str )
  local res = {}
  local cmds = split_commas( str )
  for _, cmd in ipairs( cmds ) do
    local words = split( cmd )
    assert( type( words ) == 'table' )
    table.insert( res, words )
  end
  return res
end

local function run_cmd( group, cmd )
  if cmd == 'e' then
    group:evolve()
    num_turns = num_turns + 1
    return
  end
  if cmd == 'E' then
    for i = 1, 1000 do
      group:evolve()
      num_turns = num_turns + 1
    end
    return
  end
  if cmd == 'r' then
    reset( group )
    return
  end
  if #cmd ~= 2 then return end
  local what = char( cmd, 2 )
  local goods_inputted = {}
  if what == 'a' then
    goods_inputted = group.config.names
  else
    local good = group.config.names[tonumber( what )]
    if good == nil then return end
    table.insert( goods_inputted, good )
  end
  local buy_sell = char( cmd, 1 )
  print()
  for _, good in ipairs( goods_inputted ) do
    if buy_sell == 'b' then
      group:buy( good, 100 )
      num_actions = num_actions + 1
    elseif buy_sell == 's' then
      group:sell( good, 100 )
      num_actions = num_actions + 1
    end
  end
end

local function looped( group )
  redraw( group )
  local choice = io.read()
  if choice == nil then os.exit() end
  if #choice == 0 and last_cmd then choice = last_cmd end
  last_cmd = choice
  local cmds = parse_cmds( choice )
  for _, cmd in ipairs( cmds ) do
    local to_run
    local times = 1
    local pieces = cmd
    if #pieces > 2 then return end
    if #pieces == 1 then to_run = pieces[1] end
    if #pieces == 2 then
      times = math.tointeger( pieces[1] )
      to_run = pieces[2]
    end
    for i = 1, times do
      run_cmd( group, to_run )
      print()
    end
  end
end

-----------------------------------------------------------------
-- Main
-----------------------------------------------------------------
local function main()
  local group = price_group.new_price_group( PRICE_GROUP_CONFIG )
  reset( group )
  while true do looped( group ) end
end

main()