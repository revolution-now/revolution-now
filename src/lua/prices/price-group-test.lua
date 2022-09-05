--[[ ------------------------------------------------------------
|
| price-group-test.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Unit tests for the price-group module.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports
-----------------------------------------------------------------
local price_group = require( 'prices.price-group' )
local U = require( 'test.unit' )

local ASSERT_EQ = U.ASSERT_EQ
local format = string.format
local PriceGroup = price_group.PriceGroup

local Test = U.new_test_pack()

-----------------------------------------------------------------
-- Default test configs.
-----------------------------------------------------------------
local function default_price_group_config()
  return {
    names={ 'rum', 'cigars', 'cloth', 'coats' },
    dutch=false,
    starting_euro_volumes=nil,
    starting_traded_volumes=nil, -- zeroes.
    starting_prices=nil, -- default to eq prices.
    min=1,
    max=20,
    rise_fall=4,
    volatility=1,
    target_price=12
  }
end

-- 11/12  10/11  14/15  9/10
local STARTING_11_10_14_9 = {
  rum=0x02a9,
  cigars=0x02c6,
  cloth=0x0224,
  coats=0x033c
}

-- 12/13  9/10  14/15  8/9
local STARTING_12_9_14_8 = {
  rum=0x1f3,
  cigars=0x277,
  cloth=0x1c6,
  coats=0x2b5
}

-----------------------------------------------------------------
-- Scenario Runner.
-----------------------------------------------------------------
local function assert_price( step_idx, good, eq_prices, expected )
  ASSERT_EQ( eq_prices[good], expected[good],
             format( 'equilibrium price for commodity %s at ' ..
                         'step %d. actual=%f, expected=%f.',
                     good, step_idx, eq_prices[good],
                     expected[good] ) )
end

local function run_action( group, info )
  for i = 1, info.action.count do
    if info.action.type == 'sell' then
      group:sell( info.action.what, 100 )
    elseif info.action.type == 'buy' then
      group:buy( info.action.what, 100 )
    elseif info.action.type == 'sell_all' then
      group:sell( 'rum', 100 )
      group:sell( 'cigars', 100 )
      group:sell( 'cloth', 100 )
      group:sell( 'coats', 100 )
    else
      error( 'unrecognized action: ' .. info.action.type )
    end
  end
end

local function run_scenario( scenario )
  local pg_config = default_price_group_config()
  pg_config.starting_euro_volumes =
      scenario.starting_euro_volumes
  pg_config.starting_traded_volumes =
      scenario.starting_traded_volumes

  -- This is kind of slow (quadratic in the number of steps), but
  -- we need to do this because the empirical data for each step
  -- was found by running all of the steps up to that point and
  -- then evolving for about 20 turns. So then when we move on to
  -- the next step we have to start over because the 20 turns of
  -- evolution would mess it up.
  for final_step = 1, #scenario.steps do
    local group = PriceGroup( pg_config )
    for i = 1, final_step do
      local step = scenario.steps[i]
      run_action( group, step )
    end
    for j = 1, 20 do group:evolve() end
    local eq_prices = group:equilibrium_prices()
    local step = scenario.steps[final_step]
    assert_price( final_step, 'rum', eq_prices, step.expected )
    assert_price( final_step, 'cigars', eq_prices, step.expected )
    assert_price( final_step, 'cloth', eq_prices, step.expected )
    assert_price( final_step, 'coats', eq_prices, step.expected )
  end
end

-----------------------------------------------------------------
-- Test Cases.
-----------------------------------------------------------------
function Test.eq_prices_scenario_0()
  local STARTING_11_10_14_9_plus_30_new_ships =
      { rum=0x0295, cigars=0x02b2, cloth=0x0214, coats=0x0324 }
  run_scenario{
    starting_euro_volumes=STARTING_11_10_14_9_plus_30_new_ships,
    steps={
      {
        action={ count=30, type='sell_all', what=nil },
        expected={ rum=12, cigars=11, cloth=12, coats=11 }
      }
    }
  }
end

function Test.eq_prices_scenario_1()
  -- 11/12  10/11  14/15  9/10
  local STARTING_11_10_14_9_plus_more_cotton =
      { rum=0x0295, cigars=0x02b2, cloth=0x0214, coats=0x0324 }

  run_scenario{
    starting_euro_volumes=STARTING_11_10_14_9_plus_more_cotton,
    steps={
      {
        action={ count=28, type='sell', what='cloth' },
        expected={ rum=20, cigars=20, cloth=4, coats=20 }
      }, {
        action={ count=28, type='buy', what='cloth' },
        expected={ rum=8, cigars=8, cloth=20, coats=7 }
      }
    }
  }
end

function Test.eq_prices_scenario_2()
  run_scenario{
    starting_euro_volumes=STARTING_11_10_14_9,
    steps={
      {
        action={ count=18, type='sell', what='cigars' },
        expected={ rum=20, cigars=5, cloth=20, coats=16 }
      }, {
        action={ count=12, type='buy', what='coats' },
        expected={ rum=20, cigars=5, cloth=20, coats=16 }
      }, {
        action={ count=1, type='sell', what='rum' },
        expected={ rum=17, cigars=5, cloth=20, coats=17 }
      }, {
        action={ count=1, type='buy', what='rum' },
        expected={ rum=20, cigars=5, cloth=20, coats=16 }
      }
    }
  }
end

function Test.eq_prices_scenario_3()
  run_scenario{
    starting_euro_volumes=STARTING_11_10_14_9,
    steps={
      {
        action={ count=6, type='sell', what='rum' },
        expected={ rum=7, cigars=14, cloth=18, coats=12 }
      }, {
        action={ count=8, type='sell', what='cloth' },
        expected={ rum=9, cigars=17, cloth=9, coats=15 }
      }, {
        action={ count=3, type='sell', what='coats' },
        expected={ rum=10, cigars=19, cloth=9, coats=11 }
      }, {
        action={ count=8, type='buy', what='rum' },
        expected={ rum=19, cigars=16, cloth=8, coats=10 }
      }, {
        action={ count=10, type='sell', what='cigars' },
        expected={ rum=20, cigars=7, cloth=10, coats=13 }
      }, {
        action={ count=8, type='buy', what='cloth' },
        expected={ rum=20, cigars=6, cloth=20, coats=10 }
      }
    }
  }
end

function Test.eq_prices_scenario_4()
  run_scenario{
    starting_euro_volumes=STARTING_11_10_14_9,
    steps={
      {
        action={ count=6, type='sell', what='cloth' },
        expected={ rum=14, cigars=14, cloth=8, coats=12 }
      }, {
        action={ count=3, type='buy', what='cloth' },
        expected={ rum=13, cigars=12, cloth=10, coats=11 }
      }, {
        action={ count=12, type='sell', what='cigars' },
        expected={ rum=19, cigars=6, cloth=15, coats=15 }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expected={ rum=20, cigars=5, cloth=18, coats=18 }
      }, {
        action={ count=6, type='sell', what='cloth' },
        expected={ rum=20, cigars=6, cloth=11, coats=20 }
      }, {
        action={ count=6, type='sell', what='coats' },
        expected={ rum=20, cigars=7, cloth=12, coats=12 }
      }, {
        action={ count=6, type='sell', what='coats' },
        expected={ rum=20, cigars=8, cloth=14, coats=9 }
      }, {
        action={ count=18, type='buy', what='cigars' },
        expected={ rum=20, cigars=20, cloth=9, coats=5 }
      }
    }
  }
end

function Test.eq_prices_scenario_5()
  run_scenario{
    starting_euro_volumes=STARTING_12_9_14_8,
    steps={
      {
        action={ count=6, type='sell', what='cigars' },
        expected={ rum=17, cigars=7, cloth=19, coats=12 }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expected={ rum=20, cigars=5, cloth=20, coats=15 }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expected={ rum=20, cigars=4, cloth=20, coats=18 }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expected={ rum=20, cigars=4, cloth=20, coats=20 }
      }
    }
  }
end

-- This one tests that nan can be handled, which will emerge in
-- the model calculations if all of the volumes are zero.
function Test.eq_prices_all_zeros()
  local pg_config = default_price_group_config()
  local starting = { rum=0, cigars=0, cloth=0, coats=0 }
  pg_config.starting_euro_volumes = starting
  local group = PriceGroup( pg_config )
  local eq_prices = group:equilibrium_prices()
  ASSERT_EQ( eq_prices.rum, 20, 'equilibrium price of rum' )
  ASSERT_EQ( eq_prices.cigars, 20, 'equilibrium price of rum' )
  ASSERT_EQ( eq_prices.cloth, 20, 'equilibrium price of rum' )
  ASSERT_EQ( eq_prices.coats, 20, 'equilibrium price of rum' )
end

return Test