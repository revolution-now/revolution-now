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
    starting_intrinsic_volumes=nil,
    starting_traded_volumes=nil, -- zeroes.
    min=1,
    max=20,
    target_price=12,
  }
end

-----------------------------------------------------------------
-- Public API
-----------------------------------------------------------------
function M.generate_random_intrinsic_volume()
  return
      PG.generate_random_intrinsic_volume( VOLUME_INIT_CONFIG )
end

function M.ProcessedGoodsPriceGroup( intrinsic_volumes )
  if not intrinsic_volumes then
    intrinsic_volumes = {}
    intrinsic_volumes.rum = M.generate_random_intrinsic_volume()
    intrinsic_volumes.cigars =
        M.generate_random_intrinsic_volume()
    intrinsic_volumes.cloth =
        M.generate_random_intrinsic_volume()
    intrinsic_volumes.coats =
        M.generate_random_intrinsic_volume()
  end
  local config = default_price_group_config()
  config.starting_intrinsic_volumes = intrinsic_volumes
  return PG.PriceGroup( config )
end

return M
