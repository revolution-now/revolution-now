--[[ ------------------------------------------------------------
|
| preamble.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-02-18.
|
| Description: Runs in the global lua state before each rds file.
|
--]] ------------------------------------------------------------
local setmetatable = setmetatable
local error = error

-- Erase all globals. Need to grab a reference to _G first be-
-- cause at some point in the loop it will be destroyed.
local _G = _G
for k, v in pairs( _G ) do _G[k] = nil end

-- require( 'printer' )
rds = { includes={}, items={} } -- results will be put here.

local ns = '' -- current namespace.
local push = function( tbl, o ) tbl[#tbl + 1] = o end

local tbl_keyword = function( type )
  return setmetatable( {}, {
    __index=function( _, k )
      return function( tbl )
        push( rds.items, { type=type, ns=ns, name=k, obj=tbl } )
      end
    end
  } )
end

local cmd = {
  include=function( what ) push( rds.includes, what ) end,
  namespace=function( what ) ns = what end,
  sumtype=tbl_keyword( 'sumtype' ),
  enum=tbl_keyword( 'enum' ),
  struct=tbl_keyword( 'struct' )
}

setmetatable( _G, {
  __index=function( _, k )
    return cmd[k] or function( o ) return { name=k, obj=o } end
  end,
  __newindex=function() error( 'no setting globals' ) end
} )
