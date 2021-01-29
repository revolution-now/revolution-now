--[[-------------------------------------------------------------
|
| meta.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-23.
|
| Description: Meta utilities.
|
--]]-------------------------------------------------------------

local function type_of_child( parent, child_name )
  return type( parent[child_name] )
end

-- Takes a table and just returns a new table with all the same
-- key/value pairs that come from the pairs(...) iterator. This
-- is useful if e.g. a table's pairs are produced by a __pairs
-- metafunction that we either can't call (sol2 doesn't seem to
-- call it when iterating over a table, possibly a bug) or don't
-- want to call (maybe it's expensive).
local function all_pairs( tbl )
  res = {}
  for k, v in pairs( tbl ) do
    res[k] = v
  end
  return res
end

-- Default implementation of the __pairs metamethod that repro-
-- duces standard behavior, but where we hardcode a table to use.
local function pairs_table_override( tbl )
  return function( _ )
    local function stateless_iter( _, k )
      local v
      k, v = next( tbl, k )
      if v then return k, v end
    end
    return stateless_iter, tbl, nil
  end
end

-- Default implementation of the __ipairs metamethod that repro-
-- duces standard behavior, but where we hardcode a table to use.
local function ipairs_table_override( tbl )
  return function( _ )
    local function stateless_iter( _, i )
      i = i + 1
      local v = tbl[i]
      if v then return i, v end
    end
    return stateless_iter, tbl, 0
  end
end

-- Returns a pairs iterator (generator) with two tables
-- hard-coded: iterates over the first table, then the second.
local function pairs_two_tables_override( tbl1, tbl2 )
  return function( _ )
    local which_table = tbl1
    local function stateless_iter( _, k )
      local v
      k, v = next( which_table, k )
      if v then return k, v end
      if which_table ~= tbl2 then
        which_table = tbl2
        return stateless_iter( _, nil )
      end
    end
    return stateless_iter, tbl1, nil
  end
end

local function freeze_table( parent, tbl_name )
  local tbl = parent[tbl_name]
  assert( tbl, 'cannot freeze nil table: ' .. tbl_name )
  log.trace( "freezing table \"" .. tbl_name .. "\"." )
  parent[tbl_name] = setmetatable( {}, {
    __index     = tbl,
    __pairs     = pairs_table_override( tbl ),
    __ipairs    = ipairs_table_override( tbl ),
    __newindex  = function( t, k, v )
                    error("attempt to modify a read-only table.")
                  end,
    __metatable = false
  } );
end

local function freeze_table_members( tbl )
  for k, v in pairs( tbl ) do
    if type( v ) == "table" then
      freeze_table( tbl, k )
    end
  end
end

-- Redefine the global builtin function rawset since the real
-- version is unsafe.
local real_rawset = rawset
rawset = nil

-- In this case "freeze" means that one cannot change what a
-- global variable name points to; it does not make the global
-- variable contents (i.e., if it is a table) readonly.
local function freeze_existing_globals()
  log.debug( "freezing existing globals." )
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
    __index     = function( t, k )
                    return globals[k]
                  end,
    __pairs     = pairs_two_tables_override( _G, globals ),
    __newindex  = function( t, k, v )
                    if globals[k] ~= nil then
                      error("attempt to modify a read-only global "
                            .. "(" .. tostring( k ) .. ")")
                    end
                    -- Allow setting new global variables.
                    real_rawset( t, k, v )
                  end,
    __metatable = false
  } )
  -- Now delete all globals from accessible environment.
  for k, v in pairs( globals ) do _G[k] = nil end
end

local function freeze_all()
  -- Freeze modules.
  for k, v in pairs( modules ) do
    freeze_table( _G, k )
  end
  freeze_table( _G, "modules" )
  -- Freeze enums.
  freeze_table_members( _G["e"] )
  freeze_table( _G, "e" )
  -- Freeze existing globals.
  freeze_existing_globals()
end

package_exports = {
  freeze_all = freeze_all,
  all_pairs = all_pairs,
  type_of_child = type_of_child,
}
