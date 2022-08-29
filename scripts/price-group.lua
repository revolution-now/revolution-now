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
  rum=1200, --
  cigars=900, --
  cloth=1200, --
  coats=1000 --
}
local INITIAL_GOLD = 0
local INITIAL_CMD = 'e'

-----------------------------------------------------------------
-- State
-----------------------------------------------------------------
local eq_prices = { rum=0, cigars=0, cloth=0, coats=0 }
local prices = { rum=0, cigars=0, cloth=0, coats=0 }
local volumes = { rum=0, cigars=0, cloth=0, coats=0 }

local num_turns = 0
local num_actions = 0
local gold = INITIAL_GOLD
local last_cmd = INITIAL_CMD

-----------------------------------------------------------------
-- Model
-----------------------------------------------------------------

-- Things Learned:
--
--   1. A price simply being low will not push the other prices
--      up; it will only push them up when it gets pushed down.
--   2. It is not the case that selling a good will only push
--      other prices up when its price goes down; selling a good
--      will always push the others up.
--   3. There does not seem to be any overall average-seeking be-
--      havior; the apprearance of that might just be emergent.
--   4. If you start with the four goods at their equilibrium and
--      you pick two of them and buy many of them, not only do
--      their prices not go up, but the others don't move. [This
--      is because buying does not move the equilibrium point
--      up, at least in some cases...].
--   5. [?] Even when you buy/sell something and the price
--      doesn't move, it still records the volume.
--   6. It is possible that the current volume does not actually
--      determine either the price or the direction that the
--      price is moving.
--
-- It is possible that the price seeks toward the equilibrium
-- with a speed that is proportional to distance (with a cap of
-- one movement per evaluation); any oscillating behavior is
-- probably just due to rounding errors and overshooting.
--
-- It appears that there is still an internal volume that is kept
-- for these goods, but it is only bumped when actually buying
-- and selling; e.g. when cloth is sold and it bumps rum, it only
-- bumps rum's equilibrium point, not volume. Observations:
--
-- The following apply to stationary goods, which are assumed to
-- be at their equilibrium point otherwise they'd be moving each
-- turn to get there:
--
--   1. [maybe] When a stationary good has zero volume and you
--      buy it, it appears that it does not push up its equilib-
--      rium price, but instead just pushes up its price, then
--      evolves and the price moves back to the equilibrium point
--      (unless the volatility is high enough in which case the
--      price will float up, then immediately return back down
--      one unit at a time). That said, it will still subtract
--      the quantity from the total volume.
--   2. When a stationary good has zero volume and you sell it,
--      two things appear to happen: 1) it will drive down its
--      equilibrium price by the quantity sold, and 2) it will
--      drive down its actual price (volume)
--
-- There seems to be an asymmetry between buying and selling:
--
--   - [maybe wrong] Buying does not seem to be able to raise
--     prices as strongly as selling (requires volatility to
--     match 'rise').
--   - [probably wrong] Buying does not seem to put downward
--     pressure on the other goods.
--
-- TODO
--   * Buying
--   * The initial set of equilibrium points are given by the bi-
--     nomial distribution, and the prices always seem to sum to
--     about 42-44.

local RF = 4 * 100
local VOL = 0
local MIN = 0
local MAX = 1900

local params = {
  -- LuaFormatter off
  rum    = { min=MIN, max=MAX, rise=RF, fall=RF, attrition=-10, volatility=VOL },
  cigars = { min=MIN, max=MAX, rise=RF, fall=RF, attrition=-10, volatility=VOL },
  cloth  = { min=MIN, max=MAX, rise=RF, fall=RF, attrition=-10, volatility=VOL },
  coats  = { min=MIN, max=MAX, rise=RF, fall=RF, attrition=-10, volatility=VOL },
  -- LuaFormatter on
}

-- Things known for sure:
--
--   1. We know that selling a good manually will bump up the eq
--      prices of the other goods during the sale, since if you
--      sell many of one good in one turn, then start selling an-
--      other in the same turn, you can see that good's price
--      start to rise for the first few sales (as opposed to
--      fall).
--   2. Buying does not always affect eq prices. If you start a
--      new game then you can buy an arbitrary amount of a single
--      good (or multiple goods) and none of the eq prices will
--      be affected.
--

local function copy_table( tbl )
  local res = {}
  for k, v in pairs( tbl ) do res[k] = v end
  return res
end

local function clamp( what, low, high )
  if what < low then return low end
  if what > high then return high end
  return what
end

local function target_price( good )
  assert( good )
  local eq = eq_prices[good]
  local current = prices[good]
  local velocity = (eq - current) / 1.0
  return math.floor( current + velocity )
end

local function update_price( good )
  assert( good )
  local p = params[good]
  local target = target_price( good )
  local current = prices[good]
  local new_price = current
  if target // 100 > current // 100 then
    new_price = current + 100
  elseif target // 100 < current // 100 then
    new_price = current - 100
  end
  if new_price < p.min then new_price = p.min end
  if new_price > p.max then new_price = p.max end
  prices[good] = new_price
end

local function recover( good ) update_price( good ) end

-- Evolve the goods in the way that is done when starting a new
-- turn. But note that this will not simulate interactions with
-- foreign markets because there are none in this simulation.
local function evolve()
  for _, good in ipairs( GOODS ) do recover( good ) end
end

local function int( x )
  if x >= 0 then return math.floor( x ) end
  return -int( -x )
end

local COUPLING_ENHANCEMENT = .2

local function transaction(good, quantity, sign,
                           transaction_price )
  local p = params[good]

  gold = gold + sign * quantity * transaction_price
  volumes[good] = volumes[good] + sign * quantity

  -- We only update prices if there is a net positive volume,
  -- meaning that more has been sold that bought. Note that we
  -- have already changed the volume at this point, so that will
  -- allow selling when the initial volume was zero but not buy-
  -- ing, which appears to conform to the original game.
  if volumes[good] <= 0 then
    update_price( good )
    return
  end

  local this_eq_movement = -sign * quantity

  local eq_velocity = STARTING_EQ_PRICES[good] - eq_prices[good]
  this_eq_movement = this_eq_movement + int( eq_velocity / 8 )
  eq_prices[good] = eq_prices[good] + this_eq_movement

  for _, other_good in ipairs( GOODS ) do
    if other_good ~= good then
      local other_eq_price_movement = -this_eq_movement / 3
      local other_eq_velocity = eq_velocity
      other_eq_price_movement = math.floor(
                                    other_eq_price_movement +
                                        other_eq_velocity *
                                        COUPLING_ENHANCEMENT )
      eq_prices[other_good] = eq_prices[other_good] +
                                  other_eq_price_movement
    end
  end

  do
    -- The only place that the volatility and fall should be used
    -- is together in this manner.
    local price_velocity = (1 << p.volatility) / (p.fall // 100)
    assert( price_velocity > 0 )
    local price_movement =
        math.floor( quantity * price_velocity )
    prices[good] = prices[good] - sign * price_movement
  end

  for _, good in ipairs( GOODS ) do
    eq_prices[good] = clamp( eq_prices[good], params[good].min,
                             params[good].max )
  end

  update_price( good )
end

local function buy( good, quantity )
  local sign = -1
  return transaction( good, quantity, sign, prices[good] + 100 )
end

local function sell( good, quantity )
  local sign = 1
  return transaction( good, quantity, sign, prices[good] )
end

-----------------------------------------------------------------
-- User Interaction
-----------------------------------------------------------------
local prompt = [[
  b1 - buy  rum     s1 - sell rum
  b2 - buy  cigars  s2 - sell cigars
  b3 - buy  cloth   s3 - sell cloth
  b4 - buy  coats   s4 - sell coats
  ba - buy  all     sa - sell all
  e to evolve one turn.
  E to evolve 1000 times.
  r to reset to starting state
  <enter> to repeat last command.
  # <cmd> to run <cmd> # times, e.g. "12 s3".

> ]]

local function reset()
  for _, good in ipairs( GOODS ) do
    eq_prices[good] = STARTING_EQ_PRICES[good]
    prices[good] = eq_prices[good]
    volumes[good] = 0
  end
  num_turns = 0
  num_actions = 0
  gold = INITIAL_GOLD
  last_cmd = INITIAL_CMD
end

local chart = [[
  turns:   %d
  gold:    %d
  actions: %d
  ----------------------------------------------------------
  |     #1      |      #2      |     #3      |     #4      |
  ----------------------------------------------------------
  |  %6d     |  %6d      |  %6d     |  %6d     | <- net volume in europe
  |      %2d     |      %2d      |      %2d     |      %2d     | <- eq prices
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |    %2d/%2d    |    %2d/%2d     |    %2d/%2d    |    %2d/%2d    | <- bid prices
  ----------------------------------------------------------
  last cmd:  %s
]]

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

local function loop()
  clear_screen()
  print( string.format( chart, num_turns, gold, num_actions,
                        volumes.rum, volumes.cigars,
                        volumes.cloth, volumes.coats,
                        eq_prices.rum // 100,
                        eq_prices.cigars // 100,
                        eq_prices.cloth // 100,
                        eq_prices.coats // 100,
                        prices.rum // 100, prices.rum // 100 + 1,
                        prices.cigars // 100,
                        prices.cigars // 100 + 1,
                        prices.cloth // 100,
                        prices.cloth // 100 + 1,
                        prices.coats // 100,
                        prices.coats // 100 + 1,
                        last_cmd or 'none' ) )
  io.write( prompt )

  local choice = io.read()
  local times = 1
  local cmds = split( choice )
  if #cmds > 2 then return end
  if #cmds == 1 then choice = cmds[1] end
  if #cmds == 2 then
    choice = cmds[2]
    times = math.tointeger( cmds[1] )
  end
  for i = 1, times do
    run_cmd( choice )
    print()
  end
end

reset()
while true do loop() end