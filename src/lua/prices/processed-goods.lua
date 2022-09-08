--[[ ------------------------------------------------------------
|
| processed-goods.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-05.
|
| Description: A PriceGroup for the four processed goods.
|
--]] ------------------------------------------------------------
local M = {}

local PG = require( 'prices.price-group' )

local VOLUME_INIT_CONFIG = { center=600, window=350 }

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

-----------------------------------------------------------------
-- Public API
-----------------------------------------------------------------
function M.generate_random_euro_volume()
  return PG.generate_random_euro_volume( VOLUME_INIT_CONFIG )
end

function M.ProcessedGoodsPriceGroup( euro_volumes )
  if not euro_volumes then
    euro_volumes = {}
    euro_volumes.rum = M.generate_random_euro_volume()
    euro_volumes.cigars = M.generate_random_euro_volume()
    euro_volumes.cloth = M.generate_random_euro_volume()
    euro_volumes.coats = M.generate_random_euro_volume()
  end
  local config = default_price_group_config()
  config.starting_euro_volumes = euro_volumes
  return PG.PriceGroup( config )
end

return M
