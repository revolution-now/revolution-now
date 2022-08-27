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
local prices = { rum=13, cigars=10, cloth=9, coats=10 }

-----------------------------------------------------------------
-- Model
-----------------------------------------------------------------
local params = {
  -- LuaFormatter off
  rum    = { rise=1, fall=1, attrition=-10, volatility=0 },
  cigars = { rise=1, fall=1, attrition=-10, volatility=0 },
  cloth  = { rise=1, fall=1, attrition=-10, volatility=0 },
  coats  = { rise=1, fall=1, attrition=-10, volatility=0 },
  -- LuaFormatter on
}

local volumes = { rum=0, cigars=0, cloth=0, coats=0 }

local function buy( what )
  local p = params[what]
  volumes[what] = volumes[what] - 100
end

local function sell( what )
  local p = params[what]
  volumes[what] = volumes[what] + 100
end

-----------------------------------------------------------------
-- User Interaction
-----------------------------------------------------------------
local prompt = [[
  b1 - buy  rum     s1 - sell rum
  b2 - buy  cigars  s2 - sell cigars
  b3 - buy  cloth   s3 - sell cloth
  b4 - buy  coats   s4 - sell coats
  <enter> to repeat last command.

> ]]

local function clear_screen()
  -- 27 is '\033'.
  io.write( string.char( 27 ) .. '[2J' ) -- clear screen
  io.write( string.char( 27 ) .. '[H' ) -- move cursor to upper left.
end

local function char( str, idx ) return
    string.sub( str, idx, idx ) end

local last_cmd = nil

local function loop()
  clear_screen()
  print'----------------------------------------------------------'
  print'|     #1      |      #2      |     #3      |     #4      |'
  print'----------------------------------------------------------'
  print'|     Rum     |    Cigars    |    Cloth    |    Coats    |'
  print( string.format(
             '|      %2d     |      %2d      |      %2d     |      %2d     | <- bid prices',
             prices.rum, prices.cigars, prices.cloth,
             prices.coats ) )
  print'----------------------------------------------------------'
  print( string.format(
             '|  %6d     |  %6d      |  %6d     |  %6d     | <- net volume in europe',
             volumes.rum, volumes.cigars, volumes.cloth,
             volumes.coats ) )
  print'----------------------------------------------------------'
  print()
  io.write( prompt )

  local choice = io.read()
  -- If the user input is empty then just repeat the last command
  -- if there is one.
  if last_cmd ~= nil and #choice == 0 then choice = last_cmd end
  last_cmd = choice
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