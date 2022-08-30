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
local STARTING_EQ_PRICES = {
  rum=12, --
  cigars=9, --
  cloth=14, --
  coats=8 --
}
local INITIAL_GOLD = 0
local INITIAL_CMD = 'e'

-----------------------------------------------------------------
-- State
-----------------------------------------------------------------
local eq_prices = { rum=0.0, cigars=0.0, cloth=0.0, coats=0.0 }
local prices = { rum=0, cigars=0, cloth=0, coats=0 }
local volumes = { rum=0, cigars=0, cloth=0, coats=0 }

local num_turns = 0
local num_actions = 0
local gold = INITIAL_GOLD
local last_cmd = INITIAL_CMD
local last_input = nil

-----------------------------------------------------------------
-- Model
-----------------------------------------------------------------

-- Things known for sure:
--
--   1. We know that selling a good manually will bump up the eq
--      prices of the other goods during the sale, since if you
--      sell many of one good in one turn, then start selling an-
--      other in the same turn, you can see that good's price
--      start to rise for the first few sales (as opposed to
--      fall).
--
-- TODO
--   * The initial set of equilibrium points are given by the bi-
--     nomial distribution, and the prices always seem to sum to
--     about 42-44.

local RF = 4
local VOL = 0
local MIN = 0
local MAX = 19
-- The only place that the volatility and fall should be used is
-- together in this manner.
local ALPHA = (1 << VOL) / RF

-- Used to simulate fixed point arithmetic rounding.
local PRECISION_BITS = 6

local floor = math.floor
local abs = math.abs
local min = math.min
local max = math.max

-----------------------------------------------------------------
-- Update Equilibrium Prices.
-----------------------------------------------------------------
local function clamp( what, low, high )
  if what < low then return low end
  if what > high then return high end
  return what
end

local function clamp_price( tbl, good )
  tbl[good] = clamp( tbl[good], MIN, MAX )
end

local function display_price( good )
  local hundreds = prices[good]
  local rounded = floor( hundreds )
  return rounded
end

local function fake_round( input )
  return input
  -- return floor( input * (1 << PRECISION_BITS) ) /
  --            (1 << PRECISION_BITS)
end

local function on_all( f )
  for _, good in ipairs( GOODS ) do f( good ) end
end

local function on_all_except( skip, f )
  for _, good in ipairs( GOODS ) do
    if good ~= skip then f( good ) end
  end
end

local function scale_cap( tbl, good, by )
  local delta = tbl[good] * by - tbl[good]
  -- TODO: see if we need these caps
  -- delta = clamp( delta, -1, 1 )
  tbl[good] = tbl[good] + delta
end

-----------------------------------------------------------------
-- Price Evolution
-----------------------------------------------------------------
local function target_price( good )
  assert( good )
  local eq = eq_prices[good]
  local current = prices[good]
  local velocity = (eq - current) / 1.0
  velocity = clamp( velocity, -1, 1 )
  return current + velocity
end

local function update_price( good )
  assert( good )
  local new_price = target_price( good )
  if new_price < MIN then new_price = MIN end
  if new_price > MAX then new_price = MAX end
  prices[good] = new_price
end

-- Evolve the goods in the way that is done when starting a new
-- turn. But note that this will not simulate interactions with
-- foreign markets because there are none in this simulation.
local function evolve() on_all( update_price ) end

-----------------------------------------------------------------
-- Buy/Sell logic.
-----------------------------------------------------------------
-- The sign of `quantity` should represent the change in net
-- volume in europe.
local function transaction( good, quantity, unit_price )
  gold = floor( gold + quantity * unit_price )
  volumes[good] = volumes[good] + quantity

  -- We only update prices if there is a net positive volume,
  -- meaning that more has been sold that bought, since that's
  -- what the original game seems to do. Note that we have al-
  -- ready changed the volume at this point, so that will allow
  -- updating prices when the volume is at its initial value (0)
  -- and we are selling, which the original game does.
  if volumes[good] <= 0 then return end

  ---------------------------------------------------------------
  -- Update Equilibrium Prices.
  ---------------------------------------------------------------
  local Q = quantity / 100

  eq_prices[good] = eq_prices[good] - Q
  on_all_except( good, function( other_good )
    eq_prices[other_good] = eq_prices[other_good] + Q / 3
  end )

  local D = max( STARTING_EQ_PRICES[good] - eq_prices[good], 0 )
  assert( D >= 0 )

  eq_prices[good] = eq_prices[good] + abs( Q ) * (D / 6)
  on_all_except( good, function( other_good )
    eq_prices[other_good] = eq_prices[other_good] + Q * (D / 18)
  end )

  on_all( function( good ) clamp_price( eq_prices, good ) end )

  ---------------------------------------------------------------
  -- Perturb prices.
  ---------------------------------------------------------------
  local price_movement = (quantity / 100) * ALPHA
  prices[good] = prices[good] - price_movement
  clamp_price( prices, good )
end

local function buy( good, quantity )
  transaction( good, -quantity, prices[good] + 1 )
  update_price( good )
end

local function sell( good, quantity )
  transaction( good, quantity, prices[good] )
  update_price( good )
end

-----------------------------------------------------------------
-- User Interaction
-----------------------------------------------------------------
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
  |      %2d     |      %2d      |      %2d     |      %2d     | <- eq prices
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |    %2d/%2d    |    %2d/%2d     |    %2d/%2d    |    %2d/%2d    | <- bid prices
  ----------------------------------------------------------
  last input: %s
  last cmd:   %s (will be repeated by hitting enter)
]]

local function reset()
  on_all( function( good )
    eq_prices[good] = STARTING_EQ_PRICES[good]
    prices[good] = eq_prices[good]
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
  if last_cmd ~= nil and #cmd == 0 then cmd = last_cmd end
  last_cmd = cmd
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
  print( string.format( chart, num_turns, gold, num_actions,
                        STARTING_EQ_PRICES.rum,
                        STARTING_EQ_PRICES.cigars,
                        STARTING_EQ_PRICES.cloth,
                        STARTING_EQ_PRICES.coats, volumes.rum,
                        volumes.cigars, volumes.cloth,
                        volumes.coats, floor( eq_prices.rum ),
                        floor( eq_prices.cigars ),
                        floor( eq_prices.cloth ),
                        floor( eq_prices.coats ),
                        display_price( 'rum' ),
                        display_price( 'rum' ) + 1,
                        display_price( 'cigars' ),
                        display_price( 'cigars' ) + 1,
                        display_price( 'cloth' ),
                        display_price( 'cloth' ) + 1,
                        display_price( 'coats' ),
                        display_price( 'coats' ) + 1, last_input,
                        last_cmd or 'none' ) )
  io.write( prompt )

  local choice = io.read()
  last_input = choice
  if choice == nil then os.exit() end
  local cmds = parse_cmds( choice )
  if #cmds == 0 and last_cmd then
    run_cmd( last_cmd )
    print()
  end
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