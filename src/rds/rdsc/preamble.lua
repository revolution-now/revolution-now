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
local type = type

-- Erase all globals. Need to grab a reference to _G first be-
-- cause at some point in the loop it will be destroyed.
local _G = _G
for k, _ in pairs( _G ) do _G[k] = nil end

-- require( 'printer' )
rds = { includes={}, items={} } -- results will be put here.

local ns = '' -- current namespace.
local push = function( tbl, o ) tbl[#tbl + 1] = o end

local tbl_keyword = function( type_name )
  return setmetatable( {}, {
    __index=function( _, k )
      return function( tbl )
        push( rds.items,
              { type=type_name, ns=ns, name=k, obj=tbl } )
      end
    end,
  } )
end

local cmd = {
  include=function( what ) push( rds.includes, what ) end,
  namespace=function( what ) ns = what end,
  config=tbl_keyword( 'config' ),
  sumtype=tbl_keyword( 'sumtype' ),
  enum=tbl_keyword( 'enum' ),
  struct=tbl_keyword( 'struct' ),
}

local function trim( o )
  if type( o ) ~= 'string' then return o end
  -- This will remove all new lines and condense spaces.
  -- LuaFormatter off
  return o:gsub( '^ +', '' )  -- remove spaces from beginning.
          :gsub( '\n +', '' ) -- remove newlines and spaces that follow them.
          :gsub( '%s+', ' ' ) -- keep remaining spaces, but condensed.
  -- LuaFormatter on
end

setmetatable( _G, {
  __index=function( _, k )
    local f = function( o ) return { name=k, obj=trim( o ) } end
    return cmd[k] or f
  end,
  __newindex=function() error( 'no setting globals' ) end,
} )
