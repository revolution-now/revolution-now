--[[ ------------------------------------------------------------
|
| freeze.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Helpers for freezing globals.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local fmt = string.format

-----------------------------------------------------------------
-- API
-----------------------------------------------------------------
-- This will prevent creating new globals and modifying existing
-- globals.  Example usage at top of module:
--
--   local freeze = require( 'freeze' )
--   local _ENV = freeze.globals( _ENV )
--
--   print = 5 -- error!
--   xyz = 4 -- error!
--
function M.globals( env )
  return setmetatable( {}, {
    __index=env,
    __newindex=function( _, key, _ )
      error( 'attempt to modify global "' .. key ..
                 '" which is not permitted.' )
    end,
  } )
end

function M.block_invalid_reads( tbl )
  setmetatable( tbl, {
    __index=function( _, key )
      error( 'key ' .. key .. ' does not exist.', 2 )
    end,
  } )
  return tbl
end

function M.secure_options( options, default )
  assert( default )
  options = options or {}
  -- Check that we don't have any unrecognized options.
  for k, _ in pairs( options ) do
    if default[k] == nil then
      error( fmt( 'invalid option %s', k ), 2 )
    end
  end
  -- Fill in any missing options with their default values.
  for k, v in pairs( default ) do
    if options[k] == nil then options[k] = v end
  end
  M.block_invalid_reads( options )
  return options
end

-- This will do the following on the table:
--
--   1. Prevent reading non-existent fields (though this one can
--      be turned off via options, in which case an invalid field
--      just yields nil).
--   2. Prevent adding new fields.
--   3. Prevent modifying existing fields.
--   4. Apply these same rules recursively.
--
-- Return value: new table that is hardened. The input table is
-- not changed.
function M.harden( tbl, options )
  options = options or {}
  -- Capture any options that we need to save in functions, since
  -- we don't want to hang onto the options table itself just in
  -- case that gets mutated.
  local harden_reads = options.harden_reads or true
  local frozen = {}
  for k, v in pairs( tbl ) do
    assert( k ~= '_G' )
    if type( v ) == 'table' then
      frozen[k] = M.harden( v, options )
    else
      frozen[k] = v
    end
  end
  -- Try to preserve any existing metamethods (e.g. __tostring)
  -- that we won't be overriding.
  local mt = getmetatable( tbl )
  if mt == false then
    io.write( debug.traceback() )
    error( 'metatable is hidden; cannot harden table' )
  end
  mt = mt or {}
  mt.__index = function( _, k )
    local v = frozen[k]
    if harden_reads and v == nil then
      error(
          'attempt to read non-existent key: ' .. tostring( k ),
          2 )
    end
    return v
  end
  mt.__pairs = function( _ ) return pairs( frozen ) end
  mt.__ipairs = function( _ ) return ipairs( frozen ) end
  mt.__newindex = function( _, _, _ )
    error( 'attempt to modify a read-only table.', 2 )
  end
  mt.__metatable = false
  return setmetatable( {}, mt )
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

-- Redefine the global builtin function rawset since the real
-- version is unsafe.
local real_rawset = assert( rawset )
rawset = nil

-- In this case "freeze" means that one cannot change what a
-- global variable name points to; it does not make the global
-- variable contents (i.e., if it is a table) readonly.
local function freeze_existing_globals()
  log.debug( 'freezing existing globals.' )
  local old_globals = {}
  -- First save all globals.
  for k, v in pairs( _G ) do old_globals[k] = v end
  -- After clearing out all of _G's values then any read/write to
  -- it will always come to these metamethods for the first time
  -- when referring to a variable. That allows us to intercept
  -- it. We reject writes to variables that already existed prior
  -- to the freezing. However, we allow writing to new global
  -- variables (and reassigning to them).
  setmetatable( _G, {
    __index=old_globals,
    __pairs=pairs_two_tables_override( old_globals, _G ),
    __newindex=function( t, k, v )
      if old_globals[k] ~= nil then
        error( 'attempt to modify a read-only global ' .. '(' ..
                   tostring( k ) .. ')' )
      end
      -- Allow setting new global variables.
      real_rawset( t, k, v )
    end,
    __metatable=false,
  } )
  -- Now delete all globals from accessible environment.
  for k, _ in pairs( old_globals ) do _G[k] = nil end
end

function M.freeze_environment()
  -- Freeze global tables.
  local new_globals = {}
  local exempt = {
    _G=true, -- this would take us in circles.
    package=true, -- this has _G in it.
  }
  local harden_reads = {
    config=true, --
  }
  local opts = { harden_reads=false }
  for k, v in pairs( _G ) do
    if type( v ) == 'table' and not exempt[k] then
      opts.harden_reads = harden_reads[k] or false
      new_globals[k] = M.harden( v, opts )
    end
  end
  for k, v in pairs( new_globals ) do _G[k] = v end
  -- Freeze existing globals.
  freeze_existing_globals()
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
