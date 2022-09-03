--[[ ------------------------------------------------------------
|
| price-group.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-08-27.
|
| Description: Simulation of the processed goods price group
|              model from the original game.
|
--]] ------------------------------------------------------------
local GOODS = { 'rum', 'cigars', 'cloth', 'coats' }
-- local STARTING_PRICES = { rum=11, cigars=10, cloth=14, coats=9 }
local STARTING_RATIOS = {
  -- 12/13 9/10 14/15 8/9
  -- rum=0x1f3,
  -- cigars=0x277,
  -- cloth=0x1c6,
  -- coats=0x2b5

  -- 11/12 10/11 14/15 9/10
  rum=0x200,
  cigars=0x21c,
  cloth=0x19d,
  coats=0x261
}
local STARTING_PRICES = { rum=12, cigars=11, cloth=15, coats=10 }
local INITIAL_GOLD = 0
local INITIAL_CMD = 'e'

-----------------------------------------------------------------
-- Model State
-----------------------------------------------------------------
local ratios = { rum=512, cigars=512, cloth=512, coats=512 }
local volumes = { rum=0, cigars=0, cloth=0, coats=0 }
local prices = { rum=0, cigars=0, cloth=0, coats=0 }
local gold = INITIAL_GOLD

-----------------------------------------------------------------
-- Model
-----------------------------------------------------------------
-- TODO
--   * The initial set of equilibrium points are given by the bi-
--     nomial distribution, and the prices always seem to sum to
--     about 42-44.

local RF = 4
local VOL = 0
local MIN = 1
local MAX = 20
-- The only place that the volatility and fall should be used is
-- together in this manner.
local ALPHA = (1 << VOL) / RF

local floor = math.floor
local abs = math.abs
local min = math.min
local max = math.max
local pow = math.pow

-----------------------------------------------------------------
-- Update Equilibrium Prices.
-----------------------------------------------------------------
local function clamp( what, low, high )
  if what < low then return low end
  if what > high then return high end
  return what
end

local function clamp_key( tbl, key, min, max )
  tbl[key] = clamp( tbl[key], min, max )
end

local function clamp_price( tbl, good )
  clamp_key( tbl, good, MIN, MAX )
end

local function add_to_val( tbl, key, what )
  tbl[key] = tbl[key] + what
end

local function display_price( good )
  local hundreds = prices[good]
  local rounded = floor( hundreds )
  return rounded
end

local function on_all( f )
  for _, good in ipairs( GOODS ) do f( good ) end
end

-----------------------------------------------------------------
-- Price Evolution
-----------------------------------------------------------------
local function eq_prices_calc()
  --
  -- a, b, c, d
  --
  -- y/x = a/b
  -- z/x = a/c
  -- w/x = a/d
  --
  -- x + y + z + w = 48
  --
  -- x + x*(a/b) + x*(a/c) + x*(a/d) = 48
  --
  -- x( 1 + a/b + a/c + a/d ) = 48
  --
  -- x = 48/(1 + a/b + a/c + a/d)
  -- y = 48/(1 + a/b + a/c + a/d)*(a/b)
  -- z = 48/(1 + a/b + a/c + a/d)*(a/c)
  -- w = 48/(1 + a/b + a/c + a/d)*(a/d)
  --
  local eq_prices = {}

  local a = max( 0, ratios.rum + volumes.rum )
  local b = max( 0, ratios.cigars + volumes.cigars )
  local c = max( 0, ratios.cloth + volumes.cloth )
  local d = max( 0, ratios.coats + volumes.coats )

  local beta = 48 / (1 + a / b + a / c + a / d)
  eq_prices.rum = beta
  eq_prices.cigars = beta * (a / b)
  eq_prices.cloth = beta * (a / c)
  eq_prices.coats = beta * (a / d)

  on_all( function( good )
    -- Test for nan.  Not sure where it comes from...
    if eq_prices[good] ~= eq_prices[good] then
      eq_prices[good] = 20
    end
    clamp_price( eq_prices, good )
  end )
  print( eq_prices.rum, eq_prices.cigars, eq_prices.cloth,
         eq_prices.coats )
  return eq_prices
end

local function eq_prices_walk()
  local eq_prices = {}

  local m = math.huge

  on_all( function( good )
    local p = ratios[good]
    if volumes[good] > 0 then p = p + volumes[good] end
    p = max( p, 0 )
    p = 1 / p
    m = min( m, p )
    eq_prices[good] = p
  end )

  local INCREMENT = 1
  local increments = {}
  on_all( function( good )
    increments[good] = INCREMENT * eq_prices[good] / m
  end )

  while true do
    local sum = 0
    on_all( function( good )
      sum = sum + min( eq_prices[good], MAX )
    end )
    -- print( 'sum: ', sum )
    -- assert( sum >= 0 )
    if sum >= 48 then break end
    on_all( function( good )
      add_to_val( eq_prices, good, increments[good] )
    end )
  end

  on_all( function( good ) clamp_price( eq_prices, good ) end )
  return eq_prices
end

local eq_prices = eq_prices_walk

local function evolve_ratios()
  on_all( function( good )
    local vol = volumes[good]
    local r = ratios[good]
    -- if vol > 0 then r = r + vol end
    r = r + vol
    -- Simulate the original game's apparent inability to scale
    -- down an integer when the difference between the result and
    -- the starting value would be less than one. This critical
    -- point, which is ~128, is given by 1/(1-.9921875). The
    -- number .9921875, in turn, is the closest representation of
    -- .99 that we can have in a fixed point representation with
    -- 8 fractional bits, which is likely what the original game
    -- used.
    if r >= 128 or r <= -128 then r = r * .9921875 end
    -- if vol > 0 then r = r - vol end
    r = r - vol
    ratios[good] = r
  end )
end

local function update_price( good, target )
  -- Evolve price.
  local new_price = floor( target + .5 )
  if new_price < MIN then new_price = MIN end
  if new_price > MAX then new_price = MAX end
  prices[good] = new_price
end

local function evolve()
  local eqs = eq_prices()
  on_all( function( good ) update_price( good, eqs[good] ) end )
  evolve_ratios()
end

-----------------------------------------------------------------
-- Buy/Sell logic.
-----------------------------------------------------------------
-- The sign of `quantity` should represent the change in net
-- volume in europe.
local function transaction( good, quantity, unit_price )
  gold = floor( gold + quantity * unit_price )
  volumes[good] = volumes[good] + quantity
  evolve_ratios()

  ---------------------------------------------------------------
  -- Perturb prices.
  ---------------------------------------------------------------
  -- local price_movement = (quantity / 100) * ALPHA
  -- prices[good] = prices[good] - price_movement
  -- clamp_price( prices, good )
  prices[good] = eq_prices()[good]
end

local function buy( good, quantity )
  transaction( good, -quantity, prices[good] + 1 )
end

local function sell( good, quantity )
  transaction( good, quantity, prices[good] )
end

-----------------------------------------------------------------
-- User Interaction
-----------------------------------------------------------------
local num_turns = 0
local num_actions = 0
local last_cmd = INITIAL_CMD

local prompt = [[
  b1: buy rum    s1: sell rum    b2: buy cigars  s2: sell cigars
  b3: buy cloth  s3: sell cloth  b4: buy coats   s4: sell coats
  ba: buy all    sa: sell all
  e to evolve one turn          E to evolve 1000 times.
  r to reset to starting state  <enter> to repeat last command.
  # <cmd> to run <cmd> # times, e.g. "12 s3".

> ]]

local chart = [[
  turns:   %d
  gold:    %d
  actions: %d
  ----------------------------------------------------------
  |     #1      |      #2      |     #3      |     #4      |
  ----------------------------------------------------------
  |      %2d     |      %2d      |      %2d     |      %2d     | <- initial prices
  |  %6d     |  %6d      |  %6d     |  %6d     | <- net volume in europe
  |     %04X    |     %04X     |     %04X    |     %04X    | <- ratios
  |     %.1f    |     %.1f     |     %.1f    |     %.1f    | <- eq prices
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |    %2d/%2d    |    %2d/%2d     |    %2d/%2d    |    %2d/%2d    | <- prices
  ----------------------------------------------------------
  last cmd: %s (will be repeated by hitting enter)
]]

local function reset()
  on_all( function( good )
    ratios[good] = STARTING_RATIOS[good]
    prices[good] = STARTING_PRICES[good]
    volumes[good] = 0
  end )
  num_turns = 0
  num_actions = 0
  gold = INITIAL_GOLD
  last_cmd = INITIAL_CMD
end

local function clear_screen()
  -- 27 is '\033'.
  io.write( string.char( 27 ) .. '[2J' ) -- clear screen
  io.write( string.char( 27 ) .. '[H' ) -- move cursor to upper left.
end

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

local function run_cmd( cmd )
  if cmd == 'e' then
    evolve()
    num_turns = num_turns + 1
    return
  end
  if cmd == 'E' then
    for i = 1, 1000 do
      evolve()
      num_turns = num_turns + 1
    end
    return
  end
  if cmd == 'r' then
    reset()
    return
  end
  if #cmd ~= 2 then return end
  local what = char( cmd, 2 )
  local goods_inputted = {}
  if what == 'a' then
    goods_inputted = GOODS
  else
    local good = GOODS[tonumber( what )]
    if good == nil then return end
    table.insert( goods_inputted, good )
  end
  local buy_sell = char( cmd, 1 )
  print()
  for _, good in ipairs( goods_inputted ) do
    if buy_sell == 'b' then
      buy( good, 100 )
      num_actions = num_actions + 1
    elseif buy_sell == 's' then
      sell( good, 100, --[[sign=]] 1, prices[good] )
      num_actions = num_actions + 1
    end
  end
end

local function looped()
  clear_screen()
  local eqs = eq_prices()
  print( string.format( chart, num_turns, gold, num_actions,
                        floor( STARTING_PRICES.rum ),
                        floor( STARTING_PRICES.cigars ),
                        floor( STARTING_PRICES.cloth ),
                        floor( STARTING_PRICES.coats ),
                        volumes.rum, volumes.cigars,
                        volumes.cloth, volumes.coats,
                        floor( ratios.rum ),
                        floor( ratios.cigars ),
                        floor( ratios.cloth ),
                        floor( ratios.coats ), eqs.rum,
                        eqs.cigars, eqs.cloth, eqs.coats,
                        display_price( 'rum' ) - 1,
                        display_price( 'rum' ),
                        display_price( 'cigars' ) - 1,
                        display_price( 'cigars' ),
                        display_price( 'cloth' ) - 1,
                        display_price( 'cloth' ),
                        display_price( 'coats' ) - 1,
                        display_price( 'coats' ),
                        last_cmd or 'none' ) )
  io.write( prompt )

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
      run_cmd( to_run )
      print()
    end
  end
end

reset()
while true do looped() end