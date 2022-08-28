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
  rum=1000, --
  cigars=1000, --
  cloth=1000, --
  coats=1000 --
}
local INITIAL_GOLD = 0
-- By initializing this to 'e', which means "evolve", the user
-- can just run the program and immediately start hitting <enter>
-- to start evolving the initial prices without buying or selling
-- anything.
local INITIAL_CMD = 'e'

-----------------------------------------------------------------
-- State
-----------------------------------------------------------------
local eq_prices = { rum=0, cigars=0, cloth=0, coats=0 }
local prices = { rum=0, cigars=0, cloth=0, coats=0 }
local volumes = { rum=0, cigars=0, cloth=0, coats=0 }

local num_turns = 0
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
-- Current theory:
--
-- We need to keep the volumes as we were before. Then, at any
-- given time, we can derive a price equilibrium point from the
-- volume and the relative volumes of the other three goods, and
-- then the price seeks to this equilibrium as usual (can over-
-- shoot with high volatility as usual). If we can get this model
-- to account for the selling behavior that we've accounted for
-- until now, then it might be a winner since it will allow us to
-- account for the fact that when we buy large amounts of a
-- single good at max price, we need to sell all of them back be-
-- fore the price starts dropping.  The three players are:
--
--   * price: tracks equilibrium price
--   * eq price: can be derived from static values of the volumes
--     of the goods.
--   * volume: net traded volume in europe.
--
-- ************** vvvvv
-- It is possible that the volume is only changed when the equi-
-- librium price reaches an extreme. If that is the case, then in
-- fact maybe we don't need the volume if we allow the equilib-
-- rium points to go above 19 and below 0.
-- ************** ^^^^^
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
--   1. When a good has a negative volume (bought more than sold)
--      and you sell it, the price will not move, only the volume
--      will decrease. When the volume gets to zero and you keep
--      selling it then the volume continues to go down and prob-
--      ably the equilibrium point is pushed down. The equilib-
--      rium point is probably pushed down by three times the
--      amount that each of the other goods' equilibrium points
--      are pushed up. The selling of the good probably does not
--      cause any change in volume to the other goods, only
--      changes their equilibrium points to make them want to
--      rise.
--   2. [maybe] When a stationary good has zero volume and you
--      buy it, it appears that it does not push up its equilib-
--      rium price, but instead just pushes up its price, then
--      evolves and the price moves back to the equilibrium point
--      (unless the volatility is high enough in which case the
--      price will float up, then immediately return back down
--      one unit at a time). That said, it will still subtract
--      the quantity from the total volume.
--   3. When a stationary good has zero volume and you sell it,
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
local VOL = 1
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

local function buy( good_to_buy, quantity )
  error( 'buy not implemented' )
end

local function sell( good, quantity )
  local p = params[good]
  gold = gold + quantity * (prices[good] // 100)

  local eq_velocity = eq_prices[good] - 950 -- STARTING_EQ_PRICES[good]
  local this_eq_price_movement =
      math.floor( quantity + eq_velocity / 4 )

  for _, other_good in ipairs( GOODS ) do
    if other_good ~= good then
      local other_eq_price_movement = -this_eq_price_movement / 3
      local other_eq_velocity = eq_velocity
      other_eq_price_movement = math.floor(
                                    other_eq_price_movement +
                                        other_eq_velocity / 5 )
      eq_prices[other_good] = eq_prices[other_good] -
                                  other_eq_price_movement
    end
  end

  eq_prices[good] = eq_prices[good] - this_eq_price_movement

  do
    -- The only place that the volatility and fall should be used
    -- is together in this manner.
    local price_velocity = (1 << p.volatility) / (p.fall // 100)
    assert( price_velocity > 0 )
    local price_movement =
        math.floor( quantity * price_velocity )
    prices[good] = prices[good] - price_movement
  end

  update_price( good )
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
  r to reset to starting state
  <enter> to repeat last command.

> ]]

local function reset()
  for _, good in ipairs( GOODS ) do
    eq_prices[good] = STARTING_EQ_PRICES[good]
    prices[good] = eq_prices[good]
    volumes[good] = 0
  end
  num_turns = 0
  gold = INITIAL_GOLD
  last_cmd = INITIAL_CMD
end

local chart = [[
  turns: %d
  gold:  %d
  ----------------------------------------------------------
  |     #1      |      #2      |     #3      |     #4      |
  ----------------------------------------------------------
  |  %6d     |  %6d      |  %6d     |  %6d     | <- net volume in europe
  |  %6d     |  %6d      |  %6d     |  %6d     | <- eq prices
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |  %4d/%4d  |  %4d/%4d   |  %4d/%4d  |  %4d/%4d  | <- bid prices
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

local function loop()
  clear_screen()
  print( string.format( chart, num_turns, gold, volumes.rum,
                        volumes.cigars, volumes.cloth,
                        volumes.coats, eq_prices.rum,
                        eq_prices.cigars, eq_prices.cloth,
                        eq_prices.coats, prices.rum,
                        prices.rum + 100, prices.cigars,
                        prices.cigars + 100, prices.cloth,
                        prices.cloth + 100, prices.coats,
                        prices.coats + 100, last_cmd or 'none' ) )
  print()
  io.write( prompt )

  local choice = io.read()
  if last_cmd ~= nil and #choice == 0 then choice = last_cmd end
  last_cmd = choice
  if choice == 'e' then
    evolve()
    num_turns = num_turns + 1
    return
  end
  if choice == 'r' then
    reset()
    return
  end
  if #choice ~= 2 then return end
  local what = char( choice, 2 )
  local goods_inputted = {}
  if what == 'a' then
    goods_inputted = GOODS
  else
    local good = GOODS[tonumber( what )]
    if good == nil then return end
    table.insert( goods_inputted, good )
  end
  local buy_sell = char( choice, 1 )
  print()
  for _, good in ipairs( goods_inputted ) do
    if buy_sell == 'b' then
      buy( good, 100 )
    elseif buy_sell == 's' then
      sell( good, 100 )
    end
  end
  print()
end

reset()
while true do loop() end