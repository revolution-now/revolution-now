--[[ ------------------------------------------------------------
|
| preamble.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-07-27.
|
| Description: Helpers needed by RDS files.
|
--]] ------------------------------------------------------------
rds = {}

rds.includes = {}
rds.current_namespace = ''
rds.entities = {}

-- All special keys must start with __ so that they don't get
-- confused with member values.
features_key = '__features'
meta_key = '__meta'

local function append( tbl, what ) tbl[#tbl + 1] = what end

function include( f ) append( rds.includes, f ) end
function namespace( nm ) rds.current_namespace = nm end

function var( tbl )
  assert( #tbl == 0, 'var parameters should only be key/value.' )
  local res = {}
  local n = 0
  for k, v in pairs( tbl ) do
    res.var_name = k
    res.var_type = v
    n = n + 1
  end
  assert( n == 1 )
  return res
end

function alt( tbl )
  assert( #tbl == 0, 'alt parameters should only be key/value.' )
  local res = {}
  local n = 0
  for k, v in pairs( tbl ) do
    res.alt_name = k
    res.alt_vars = v
    n = n + 1
  end
  assert( n == 1 )
  return res
end

function sumtype( tbl )
  tbl[meta_key] = tbl[meta_key] or {}
  tbl[meta_key].namespace = rds.current_namespace
  tbl[meta_key].type = 'sumtype'
  append( rds.entities, tbl )
  return tbl
end

function template( tbl )
  tbl.sumtype = function( tbl2 )
    tbl2[meta_key] = tbl2[meta_key] or {}
    tbl2[meta_key].template = tbl
    return sumtype( tbl2 )
  end
  return tbl
end

function enum( tbl )
  tbl[meta_key] = tbl[meta_key] or {}
  tbl[meta_key].namespace = rds.current_namespace
  tbl[meta_key].type = 'enum'
  append( rds.entities, tbl )
  return tbl
end

-- Freeze all globals -------------------------------------------
--
-- Redefine the global builtin function rawset since the real
-- version is unsafe.
local real_rawset = rawset
rawset = nil

local globals = {}
-- First save all globals.
for k, v in pairs( _G ) do globals[k] = v end

-- After clearing out all of _G's values then any read/write to
-- it will always come to these metamethods for the first time
-- when referring to a variable. That allows us to intercept
-- it. We reject writes to variables that already existed prior
-- to the freezing. However, we allow writing to new global
-- variables (and reassigning to them).
setmetatable( _G, {
  __index=globals,
  __newindex=function( t, k, v )
    if globals[k] ~= nil then
      error( 'attempt to modify a read-only global ' .. '(' ..
                 tostring( k ) .. ')' )
    end
    -- This part is specific to this RDS mechanism.
    if type( v ) == 'table' and type( v[meta_key] ) == 'table' then
      v[meta_key].name = k
    end
    -- Allow setting new global variables.
    real_rawset( t, k, v )
  end,
  __metatable=false
} )

-- Now delete all globals from accessible environment.
for k, v in pairs( globals ) do _G[k] = nil end
