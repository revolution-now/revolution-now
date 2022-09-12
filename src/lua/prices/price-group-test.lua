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
    min=1,
    max=20,
    rise_fall=4,
    volatility=1,
    bid_ask_spread=1,
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
local function assert_price( step_idx, good, eq_prices, expect_eq )
  ASSERT_EQ( eq_prices[good], expect_eq[good],
             format( 'equilibrium price for commodity %s at ' ..
                         'step %d. actual=%f, expect_eq=%f.',
                     good, step_idx, eq_prices[good],
                     expect_eq[good] ) )
end

local function assert_volume(step_idx, good, euro_volumes,
                             expect_vol )
  expect_vol = expect_vol[good]
  -- The volume will be in the form 0xNNNN, i.e. a signed 16 bit
  -- hex number, since that is what is seen when looking at the
  -- save files in a hex editor (i.e., easy comparison). But
  -- since they could be negative, we need to transform them back
  -- to what Lua understands.
  if expect_vol >= 0x8000 then expect_vol = expect_vol - 0x10000 end
  ASSERT_EQ( euro_volumes[good], expect_vol,
             format( 'euro volume for commodity %s at ' ..
                         'step %d. actual=%f, expect_vol=%f.',
                     good, step_idx, euro_volumes[good],
                     expect_vol ) )
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
    assert_price( final_step, 'rum', eq_prices, step.expect_eq )
    assert_price( final_step, 'cigars', eq_prices, step.expect_eq )
    assert_price( final_step, 'cloth', eq_prices, step.expect_eq )
    assert_price( final_step, 'coats', eq_prices, step.expect_eq )
    local euro_volumes = group.euro_volumes
    assert_volume( final_step, 'rum', euro_volumes,
                   step.expect_vol )
    assert_volume( final_step, 'cigars', euro_volumes,
                   step.expect_vol )
    assert_volume( final_step, 'cloth', euro_volumes,
                   step.expect_vol )
    assert_volume( final_step, 'coats', euro_volumes,
                   step.expect_vol )
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
        expect_eq={ rum=12, cigars=11, cloth=12, coats=11 },
        expect_vol={
          rum=0xFBB2,
          cigars=0xFBCB,
          cloth=0xFBA4,
          coats=0xFC09
        }

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
        expect_eq={ rum=20, cigars=20, cloth=4, coats=20 },
        expect_vol={
          rum=0x01DB,
          cigars=0x01ED,
          cloth=0xFEEE,
          coats=0x023D
        }
      }, {
        action={ count=28, type='buy', what='cloth' },
        expect_eq={ rum=8, cigars=8, cloth=20, coats=7 },
        expect_vol={
          rum=0x0187,
          cigars=0x0199,
          cloth=0xFF94,
          coats=0x01D9
        }
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
        expect_eq={ rum=20, cigars=5, cloth=20, coats=16 },
        expect_vol={
          rum=0x0208,
          cigars=0x00AC,
          cloth=0x01A8,
          coats=0x0275
        }
      }, {
        action={ count=12, type='buy', what='coats' },
        expect_eq={ rum=20, cigars=5, cloth=20, coats=16 },
        expect_vol={
          rum=0x0208,
          cigars=0x00AC,
          cloth=0x01A8,
          coats=0x0275
        }
      }, {
        action={ count=1, type='sell', what='rum' },
        expect_eq={ rum=17, cigars=5, cloth=20, coats=17 },
        expect_vol={
          rum=0x01F7,
          cigars=0x009D,
          cloth=0x01A5,
          coats=0x0271
        }
      }, {
        action={ count=1, type='buy', what='rum' },
        expect_eq={ rum=20, cigars=5, cloth=20, coats=16 },
        expect_vol={
          rum=0x01FF,
          cigars=0x008E,
          cloth=0x01A2,
          coats=0x026D
        }
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
        expect_eq={ rum=7, cigars=14, cloth=18, coats=12 },
        expect_vol={
          rum=0x01D2,
          cigars=0x024F,
          cloth=0x01CC,
          coats=0x02AF
        }
      }, {
        action={ count=8, type='sell', what='cloth' },
        expect_eq={ rum=9, cigars=17, cloth=9, coats=15 },
        expect_vol={
          rum=0x0194,
          cigars=0x022F,
          cloth=0x0127,
          coats=0x0287
        }
      }, {
        action={ count=3, type='sell', what='coats' },
        expect_eq={ rum=10, cigars=19, cloth=9, coats=11 },
        expect_vol={
          rum=0x017F,
          cigars=0x0223,
          cloth=0x010F,
          coats=0x024A
        }
      }, {
        action={ count=8, type='buy', what='rum' },
        expect_eq={ rum=19, cigars=16, cloth=8, coats=10 },
        expect_vol={
          rum=0x01BD,
          cigars=0x020B,
          cloth=0x00DF,
          coats=0x0226
        }
      }, {
        action={ count=10, type='sell', what='cigars' },
        expect_eq={ rum=20, cigars=7, cloth=10, coats=13 },
        expect_vol={
          rum=0x019F,
          cigars=0x0136,
          cloth=0x0099,
          coats=0x01EA
        }
      }, {
        action={ count=8, type='buy', what='cloth' },
        expect_eq={ rum=20, cigars=6, cloth=20, coats=10 },
        expect_vol={
          rum=0x0187,
          cigars=0x00EA,
          cloth=0x00F1,
          coats=0x01BE
        }
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
        expect_eq={ rum=14, cigars=14, cloth=8, coats=12 },
        expect_vol={
          rum=0x0238,
          cigars=0x024F,
          cloth=0x0166,
          coats=0x02AF
        }
      }, {
        action={ count=3, type='buy', what='cloth' },
        expect_eq={ rum=13, cigars=12, cloth=10, coats=11 },
        expect_vol={
          rum=0x022C,
          cigars=0x0243,
          cloth=0x0181,
          coats=0x02A0
        }
      }, {
        action={ count=12, type='sell', what='cigars' },
        expect_eq={ rum=19, cigars=6, cloth=15, coats=15 },
        expect_vol={
          rum=0x01FC,
          cigars=0x0133,
          cloth=0x0147,
          coats=0x0269
        }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expect_eq={ rum=20, cigars=5, cloth=18, coats=18 },
        expect_vol={
          rum=0x01EA,
          cigars=0x008C,
          cloth=0x012F,
          coats=0x0251
        }
      }, {
        action={ count=6, type='sell', what='cloth' },
        expect_eq={ rum=20, cigars=6, cloth=11, coats=20 },
        expect_vol={
          rum=0x01D8,
          cigars=0x0036,
          cloth=0x00B3,
          coats=0x0239
        }
      }, {
        action={ count=6, type='sell', what='coats' },
        expect_eq={ rum=20, cigars=7, cloth=12, coats=12 },
        expect_vol={
          rum=0x01C6,
          cigars=0xFFE3,
          cloth=0x0083,
          coats=0x01BA
        }
      }, {
        action={ count=6, type='sell', what='coats' },
        expect_eq={ rum=20, cigars=8, cloth=14, coats=9 },
        expect_vol={
          rum=0x01B4,
          cigars=0xFF95,
          cloth=0x0058,
          coats=0x0129
        }
      }, {
        action={ count=18, type='buy', what='cigars' },
        expect_eq={ rum=20, cigars=20, cloth=9, coats=5 },
        expect_vol={
          rum=0x017E,
          cigars=0x002F,
          cloth=0xFFDE,
          coats=0x006C
        }
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
        expect_eq={ rum=17, cigars=7, cloth=19, coats=12 },
        expect_vol={
          rum=0x01A5,
          cigars=0x01A8,
          cloth=0x017A,
          coats=0x0242
        }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expect_eq={ rum=20, cigars=5, cloth=20, coats=15 },
        expect_vol={
          rum=0x0193,
          cigars=0x0118,
          cloth=0x016E,
          coats=0x022A
        }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expect_eq={ rum=20, cigars=4, cloth=20, coats=18 },
        expect_vol={
          rum=0x0181,
          cigars=0x0071,
          cloth=0x0162,
          coats=0x0212
        }
      }, {
        action={ count=6, type='sell', what='cigars' },
        expect_eq={ rum=20, cigars=4, cloth=20, coats=20 },
        expect_vol={
          rum=0x0174,
          cigars=0xFFB8,
          cloth=0x0156,
          coats=0x01FB
        }
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