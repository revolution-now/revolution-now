--[[ ------------------------------------------------------------
|
| price-group.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Logic for evolving the equilibrium market prices
|              for commodities that are part of a "price group".
|
--]] ------------------------------------------------------------
local M = {}

local freeze = require( 'util.freeze' )
local tables = require( 'util.tables' )

-- Don't allow this module to set globals.
local _ENV = freeze.globals( _ENV )

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local floor = math.floor
local max = math.max
local copy_table = tables.copy_table

-----------------------------------------------------------------
-- Helpers
-----------------------------------------------------------------
local function is_nan( x ) return (x ~= x) end

local function round( x ) return floor( x + .5 ) end

local function clamp( what, low, high )
  if what < low then return low end
  if what > high then return high end
  return what
end

local function clamp_price( group, tbl, good )
  tbl[good] = clamp( tbl[good], group.config.min,
                     group.config.max )
end

local function sum_values( tbl )
  local res = 0
  for _, v in pairs( tbl ) do res = res + v end
  return res
end

-----------------------------------------------------------------
-- Price Group Model
-----------------------------------------------------------------
-- Description of model: TODO
--
-- Define the price group object interface.
local PriceGroup = {}
PriceGroup.__index = PriceGroup
PriceGroup.__metatable = false

function PriceGroup:on_all( f )
  for _, good in ipairs( self.config.names ) do f( good ) end
end

-- This is the function that takes the two volumes (intrinsic
-- volume and traded volume) and from them derives the equilib-
-- rium prices (the prices toward which the actual player-visible
-- prices will tend toward). The basic idea is is that the equi-
-- librium prices are proportional to the inverses of the total
-- volumes. The OG appears to do this:
--
--   1. Get the total volume for each good, ignoring negative
--      traded volumes, as usual.
--   2. Normalize them so that their average is one, which will
--      allow scaling them to our desired target value.
--   3. Take the inverse of that normalized volume, which is pro-
--      portional to the price, then scale it up to the target
--      value (which is fixed at 12).
--
-- This process ensures that a couple of things:
--
--   1. The price of a good will generally go down when the vol-
--      umes go up, which makes sense because this is a basic
--      supply/demand mechanic that one would expect. This is
--      achieved by letting the prices be proportional to the in-
--      verse of the volumes.
--   2. But no matter what is bought/sold, the average price of
--      the four goods will (approximately) remain constant; if
--      one falls, the others will rise. This is achieved by nor-
--      malizing the volumes and then scaling them up to the
--      target value. This implements what was likely the design-
--      er's goal of encouraging the production and sale of all
--      four goods (or at least the more the better) by allowing
--      the prices to remain stable no matter how much is sold,
--      so long as multiple of the goods (the more, the better)
--      are sold. If the player only sells one, its price will
--      quickly drop and the others will rise. If the player
--      sells all four of them in alternation, none of the prices
--      will drop, no matter how much is sold. This way, if the
--      player can always rely on earning a lot of gold so long
--      as they are producing all four goods, and the prices will
--      never drop.
--
-- It seems like, ideally, step 3 (taking the inverse) would be
-- done before step 2 (normalizing), since presumably we want the
-- average *price* of the goods to remain approximately constant
-- and not their inverses (volumes) per se. But, that is what the
-- original game appears to do.
--
-- There are some additional technicalities, such as ignoring
-- negative values in some cases, and rounding/nan behavior, that
-- may not have reasons per se, but they are just what the orig-
-- inal game appears to do.
--
-- This function appears to reproduces the OG's numbers *exact-
-- ly*, despite the complex behaviors of the prices, and is thus
-- quite astonishing.
function PriceGroup:equilibrium_prices()
  local res = {}
  local total_volumes = {}
  self:on_all( function( good )
    total_volumes[good] = self.intrinsic_volumes[good] +
                              max( self.traded_volumes[good], 0 )
  end )
  local avg_total_volume = sum_values( total_volumes ) /
                               #self.config.names
  self:on_all( function( good )
    -- When all the total volumes are equal then this will be 1,
    -- and then the below will yield the target price.
    local normalized_volume = max( total_volumes[good], 0 ) /
                                  avg_total_volume
    res[good] = self.config.target_price / normalized_volume
    -- nan can happen if both the numerator and denominator in
    -- the above are both zero, which is not expected to happen
    -- in normal game play, but just in case let's handle it.
    if is_nan( res[good] ) then res[good] = math.huge end
    -- The original game seems to use floor here and not round,
    -- and it makes a difference.
    res[good] = floor( res[good] )
    clamp_price( self, res, good )
  end )
  return res
end

-- This is the function that will evolve the intrinsic volumes.
-- It is done at the start of each turn, and also when buying
-- selling a good in the harbor (unless the player is dutch).
--
-- This function appears to reproduces the OG's numbers *exact-
-- ly*, despite the complex behaviors of the prices, and is thus
-- quite astonishing.
local function evolve_intrinsic_volume( group, good )
  local r = group.intrinsic_volumes[good]
  -- The original game basically seems to ignore the traded
  -- volume in all cases if it is negative (meaning that more of
  -- the good has been bought than sold).
  local vol = max( group.traded_volumes[good], 0 )

  -- The OG evolves the volumes each turn by multiplying the
  -- total volume (intrinsic + traded) by .99. That said, it
  -- never modifies the traded volumes, and so the evolution of
  -- total volume is only reflected in the intrinsic volume.
  -- Hence why we add in the traded volume, then multiply by .99,
  -- then remove it again.
  --
  -- Although the OG likely wanted to scale r down by 1%, it
  -- likely uses a fixed point representation with 8 decimal
  -- bits. In that representation, .9921875 is the closest one
  -- can get to .99.
  --
  -- The addition of .5 is needed to make the numbers match the
  -- empirical data exactly. Not sure if the OG explicitly does
  -- this or if it some kind of artifact of the way it does
  -- floating point math. One possibility is that adding this
  -- bias will prevent the intrinsic volumes from drifting all
  -- the way to zero, which would not be good because this model
  -- does not behave well when they hit zero.
  r = ((r + vol + .5) * .9921875) - vol

  -- The original game represents the intrinsic volumes as inte-
  -- gers, and so to simulate that we need to round the intrinsic
  -- volume after every evolution step.
  group.intrinsic_volumes[good] = round( r )
end

local function evolve_intrinsic_volumes( group )
  group:on_all( function( good )
    evolve_intrinsic_volume( group, good )
  end )
end

function PriceGroup:evolve()
  local eqs = self:equilibrium_prices()
  evolve_intrinsic_volumes( self )
end

-- The sign of `quantity` should represent the change in net
-- volume in europe.
local function transaction( group, good, quantity )
  group.traded_volumes[good] = group.traded_volumes[good] +
                                   quantity
  if group.traded_volumes[good] >= 0 then
    -- This is one of the benefits that the Dutch get.
    if not group.config.dutch then
      evolve_intrinsic_volumes( group )
    end
  end
end

function PriceGroup:buy( good, quantity )
  transaction( self, good, -quantity )
end

function PriceGroup:sell( good, quantity )
  transaction( self, good, quantity )
end

-- Create a new PriceGroup object.
local function new_price_group( config )
  local o = setmetatable( {}, PriceGroup )
  o.config = config

  o.intrinsic_volumes = {}
  o.traded_volumes = {}
  o.prices = {}
  o:on_all( function( good )
    o.intrinsic_volumes[good] = o.config
                                    .starting_intrinsic_volumes[good]
    o.prices[good] = 0
    o.traded_volumes[good] = 0
  end )

  if config.starting_traded_volumes then
    o.traded_volumes =
        copy_table( config.starting_traded_volumes )
  end
  return o
end

-----------------------------------------------------------------
-- Public API
-----------------------------------------------------------------
-- This will generate a random starting intrinsic volume for one
-- of the processed goods (rum, cigars, cloth, coats). It will
-- choice based on a uniform distribution over a certain window,
-- and, because of the algorithm used in this model for trans-
-- lating volumes to prices, this will lead to the characteristic
-- "skewed hill" distribution that governs the starting prices in
-- the OG.
function M.generate_random_intrinsic_volume( config )
  local bottom = config.center - config.window / 2
  return math.floor( math.random() * config.window + bottom )
end

function M.PriceGroup( config ) return new_price_group( config ) end

return M
