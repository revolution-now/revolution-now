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
local goods = { 'rum', 'cigars', 'cloth', 'coats' }
local initial_prices = { rum=10, cigars=10, cloth=10, coats=10 }

local prices = {
  rum=initial_prices.rum,
  cigars=initial_prices.cigars,
  cloth=initial_prices.cloth,
  coats=initial_prices.coats
}

-----------------------------------------------------------------
-- Model
-----------------------------------------------------------------
local params = {
  -- LuaFormatter off
  rum    = { rise=100, fall=100, attrition=-10, volatility=0 },
  cigars = { rise=100, fall=100, attrition=-10, volatility=0 },
  cloth  = { rise=100, fall=100, attrition=-10, volatility=0 },
  coats  = { rise=100, fall=100, attrition=-10, volatility=0 },
  -- LuaFormatter on
}

local volumes = { rum=0, cigars=0, cloth=0, coats=0 }

local function target_price( good )
  assert( good )
  local p = params[good]
  local v = volumes[good]
  local initial = initial_prices[good]
  local res = initial
  if v > 0 then res = res - (v // p.rise) end
  -- The // integer division operator does weird things when the
  -- numerator is negative, so we'll make it positive.
  if v < 0 then res = res + ((-v) // p.fall) end
  return res
end

local function update_price( good )
  assert( good )
  local target = target_price( good )
  local current = prices[good]
  local new_price = current
  if target > current then
    new_price = current + 1
  elseif target < current then
    new_price = current - 1
  end
  if new_price < 0 then new_price = 0 end
  if new_price > 19 then new_price = 19 end
  prices[good] = new_price
end

local function buy( good )
  local p = params[good]
  local quantity = 100
  quantity = quantity * (1 << p.volatility)

  -- TODO: apply correlations.

  volumes[good] = volumes[good] - quantity
  update_price( good )
end

local function sell( good )
  local p = params[good]
  local quantity = 100
  quantity = quantity * (1 << p.volatility)

  -- TODO: apply correlations.

  volumes[good] = volumes[good] + quantity
  update_price( good )
end

-- Evolve the goods in the way that is done when starting a new
-- turn. But note that this will not simulate interactions with
-- foreign markets because there are none in this simulation.
local function evolve()
  --
  for _, good in ipairs( goods ) do update_price( good ) end
end

-----------------------------------------------------------------
-- User Interaction
-----------------------------------------------------------------
local prompt = [[
  b1 - buy  rum     s1 - sell rum
  b2 - buy  cigars  s2 - sell cigars
  b3 - buy  cloth   s3 - sell cloth
  b4 - buy  coats   s4 - sell coats
  e to evolve one turn.
  <enter> to repeat last command.

> ]]

local chart = [[
  turns: %d
  ----------------------------------------------------------
  |     #1      |      #2      |     #3      |     #4      |
  ----------------------------------------------------------
  |  %6d     |  %6d      |  %6d     |  %6d     | <- net volume in europe
  ----------------------------------------------------------
  |     Rum     |    Cigars    |    Cloth    |    Coats    |
  |    %2d/%2d    |    %2d/%2d     |    %2d/%2d    |    %2d/%2d    | <- bid prices
  ----------------------------------------------------------
]]

local function clear_screen()
  -- 27 is '\033'.
  io.write( string.char( 27 ) .. '[2J' ) -- clear screen
  io.write( string.char( 27 ) .. '[H' ) -- move cursor to upper left.
end

local function char( str, idx ) return
    string.sub( str, idx, idx ) end

local last_cmd = nil

local num_turns = 0

local function loop()
  clear_screen()
  print( string.format( chart, num_turns, volumes.rum,
                        volumes.cigars, volumes.cloth,
                        volumes.coats, prices.rum,
                        prices.rum + 1, prices.cigars,
                        prices.cigars + 1, prices.cloth,
                        prices.cloth + 1, prices.coats,
                        prices.coats + 1 ) )
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
  if #choice ~= 2 then return end
  local good = goods[tonumber( char( choice, 2 ) )]
  if good == nil then return end
  local buy_sell = char( choice, 1 )
  print()
  if buy_sell == 'b' then
    buy( good )
  elseif buy_sell == 's' then
    sell( good )
  end
  print()
end

while true do loop() end