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
-- duces standard behavior.
local function default_pairs( tbl )
  return function( t )
    local function stateless_iter( t, k )
      local v
      k, v = next( tbl, k )
      if v then return k, v end
    end
    return stateless_iter, t, nil
  end
end

-- Default implementation of the __ipairs metamethod that repro-
-- duces standard behavior.
local function default_ipairs( tbl )
  return function( t )
    local function stateless_iter( t, i )
      i = i + 1
      local v = tbl[i]
      if v then return i, v end
    end
    return stateless_iter, t, 0
  end
end

local function freeze_table( parent, tbl_name )
  local tbl = parent[tbl_name]
  assert( tbl, 'cannot freeze nil table: ' .. tbl_name )
  log.debug( "freezing table \"" .. tbl_name .. "\"." )
  parent[tbl_name] = setmetatable( {}, {
    __index     = tbl,
    __pairs     = default_pairs( tbl ),
    __ipairs    = default_ipairs( tbl ),
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

-- In this case "freeze" means that one cannot change what a
-- global variable name points to; it does not make the global
-- variable contents (i.e., if it is a table) readonly.
local function freeze_existing_globals()
  log.debug( "freezing existing globals." )
  local globals = {}
  -- First save all globals.
  for k, v in pairs( _ENV ) do globals[k] = v end
  -- After clearing out all of _ENV's values then any read/write
  -- to it will always come to these metamethods for the first
  -- time when referring to a variable. That allows us to inter-
  -- cept it. We reject reads from non-existent variables and we
  -- reject writes to variables that already existed prior to the
  -- freezing. However, we allow writing to new global variables
  -- (and reassigning to them).
  setmetatable( _ENV, {
    __index     = function( t, k )
                    if globals[k] == nil then
                      error("attempt to read a nil global variable.")
                    end
                    return globals[k]
                  end,
    __pairs     = default_pairs( globals ),
    __ipairs    = default_ipairs( globals ),
    __newindex  = function( t, k, v )
                    if globals[k] ~= nil then
                      error("attempt to modify a read-only global.")
                    end
                    -- Allow setting new global variables.
                    rawset( t, k, v )
                  end,
    __metatable = false
  } )
  -- Now delete all globals from accessible environment.
  for k, v in pairs( globals ) do _ENV[k] = nil end
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
  all_pairs = all_pairs
}
