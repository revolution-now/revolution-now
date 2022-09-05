--[[ ------------------------------------------------------------
|
| price-group.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Logic for evolving market prices for commodities
|              that are part of a "price group".
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

local function set_price( group, good, price )
  group.prices[good] = round( price )
end

-----------------------------------------------------------------
-- Price Group Model
-----------------------------------------------------------------
-- Description of model: TODO
--
-- The original game basically seems to ignore the traded volume
-- in all cases if it is negative (meaning that more of the good
-- has been bought than sold).

-- Define the price group object interface.
local PriceGroup = {}
PriceGroup.__index = PriceGroup
PriceGroup.__metatable = false

function PriceGroup:on_all( f )
  for _, good in ipairs( self.config.names ) do f( good ) end
end

function PriceGroup:equilibrium_prices()
  local res = {}
  local avg_total_volume = 0
  self:on_all( function( good )
    avg_total_volume =
        avg_total_volume + self.euro_volumes[good] +
            max( self.traded_volumes[good], 0 )
  end )
  avg_total_volume = avg_total_volume / #self.config.names
  self:on_all( function( good )
    local q = self.euro_volumes[good] +
                  max( self.traded_volumes[good], 0 )
    -- The original game seems to ues floor here and not round.
    local normalized = max( q, 0 ) / avg_total_volume
    res[good] = floor( self.config.target_price / normalized )
    if is_nan( res[good] ) then res[good] = math.huge end
    clamp_price( self, res, good )
  end )
  return res
end

local function evolve_euro_volume( group, good )
  local r = group.euro_volumes[good]
  local vol = max( group.traded_volumes[good], 0 )
  -- The OG evolves the volumes each turn by multiplying the
  -- total volume (euro + traded) by .99. That said, it never
  -- modifies the traded volumes, and so the evolution of total
  -- volume is only reflected in the euro volume. Hence why we
  -- add in the traded volume, then multiply by .99, then remove
  -- it again.
  r = r + vol
  -- The OG stores volumes as integers, and because of that it is
  -- unable to scale down small numbers that are below the
  -- threshold where 1% ~ 1. Also, while it likely wanted to
  -- scale r down by 1%, it likely uses a fixed point representa-
  -- tion with 8 decimal bits. In that representation, .9921875
  -- is the closest one can get to .99. And the cutoff (128) is
  -- given by 1/(1-.9921875). This is also convenient because
  -- this evolution algorithm becomes unstable if r were allowed
  -- to approach zero (which can happen if you just evolve the
  -- volumes for a few hundred turns without selling anything);
  -- this prevents that.
  if r < -128 or r > 128 then r = r * .9921875 end
  r = r - vol
  -- The original game represents the euro volumes as integers.
  -- We do that here too because it can have noticeable effects.
  group.euro_volumes[good] = floor( r )
end

local function evolve_euro_volumes( group )
  group:on_all( function( good )
    evolve_euro_volume( group, good )
  end )
end

local function price_eq_push( group, good, target )
  target = round( target )
  local p = group.prices[good]
  if p > target then
    p = p - 1
    if p < target then p = target end
  elseif p < target then
    p = p + 1
    if p > target then p = target end
  end
  local push = p - group.prices[good]
  push = clamp( push, -1, 1 )
  return push
end

function PriceGroup:evolve()
  local eqs = self:equilibrium_prices()
  self:on_all( function( good )
    set_price( self, good, self.prices[good] +
                   price_eq_push( self, good, eqs[good] ) )
    clamp_price( self, self.prices, good )
  end )
  evolve_euro_volumes( self )
end

-- The sign of `quantity` should represent the change in net
-- volume in europe.
local function transaction( group, good, quantity, unit_price )
  group.traded_volumes[good] = group.traded_volumes[good] +
                                   quantity
  if group.traded_volumes[good] >= 0 then
    -- This is one of the benefits that the Dutch get. TODO: need
    -- to research more about the dutch benefits.
    if not group.config.dutch then
      evolve_euro_volumes( group )
    end
  end
  -- The only place that the volatility and fall should be used
  -- is together in this manner.
  local volatility_push = -(quantity / 100) *
                              (1 << group.config.volatility) /
                              group.config.rise_fall
  local eq_push = price_eq_push( group, good,
                                 group:equilibrium_prices()[good] )
  local net_push = eq_push + volatility_push
  net_push = clamp( net_push, -1, 1 )
  set_price( group, good, group.prices[good] + net_push )
  clamp_price( group, group.prices, good )
end

function PriceGroup:buy( good, quantity )
  transaction( self, good, -quantity, self.prices[good] + 1 )
end

function PriceGroup:sell( good, quantity )
  transaction( self, good, quantity, self.prices[good] )
end

-- Create a new PriceGroup object.
local function new_price_group( config )
  local o = setmetatable( {}, PriceGroup )
  o.config = config

  o.euro_volumes = {}
  o.traded_volumes = {}
  o.prices = {}
  o:on_all( function( good )
    o.euro_volumes[good] = o.config.starting_euro_volumes[good]
    o.prices[good] = 0
    o.traded_volumes[good] = 0
  end )

  if config.starting_traded_volumes then
    o.traded_volumes =
        copy_table( config.starting_traded_volumes )
  end

  local starting_prices = config.starting_prices or
                              o:equilibrium_prices()
  o:on_all( function( good )
    set_price( o, good, starting_prices[good] )
  end )

  return o
end

-----------------------------------------------------------------
-- Public API
-----------------------------------------------------------------
-- This will generate a random starting euro volume for one of
-- the processed goods (rum, cigars, cloth, coats). It will
-- choice based on a uniform distribution over a certain window,
-- and, because of the algorithm used in this model for trans-
-- lating volumes to prices, this will lead to the characteristic
-- "skewed hill" distribution that governs the starting prices in
-- the OG.
function M.generate_random_euro_volume( config )
  local bottom = config.center - config.window / 2
  return math.floor( math.random() * config.window + bottom )
end

function M.PriceGroup( config ) return new_price_group( config ) end

return M
