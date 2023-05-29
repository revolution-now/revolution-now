--[[ ------------------------------------------------------------
|
| wait.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-07-01.
|
| Description: Helpers for using Lua coroutines with C++
|              waits.
|
--]] ------------------------------------------------------------
local M = {}

assert( coroutine, 'The coroutine library must be available.' )

function M.await( wait )
  assert( wait )
  assert( type( wait.set_resume ) == 'function',
          'await called on invalid wait type. This may ' ..
              'mean that it is not a wait type or that the ' ..
              'wait type\'s usertype was not registered.' )

  -- DO NOT put anything more before this line.
  local closer<close> = wait

  assert( coroutine.isyieldable(), 'This function can only ' ..
              'called from within a coroutine.' )

  local thread, is_main = coroutine.running()
  assert( not is_main )

  if wait:ready() then return wait:get() end

  wait:set_resume( thread )
  coroutine.yield()
  local err = wait:error()
  -- 2 means to use the source location of the calling function
  -- in the error message, which will likely be more useful.
  if err ~= nil then error( err, 2 ) end
  assert( wait:ready() )
  return wait:get()
end

-- You can wrap a function using this so that you don't have to
-- call await( ... ) on it. However, we still need to have access
-- to the unwrapped versions in general so that we can run mul-
-- tiple coroutines in parallel and await on all of them.
function M.auto_await( f )
  return function( ... ) return M.await( f( ... ) ) end
end

-- This returns a function that, when called, starts running the
-- function f with the given parameters and returns a native
-- wait object. If the resulting object will not be awaited
-- on (i.e., it will run in parallel) then you might want to wrap
-- it in `auto_assert`.
function M.native_coroutine( f )
  return function( ... )
    local pack = table.pack( ... )
    local function run()
      return f( table.unpack( pack, 1, pack.n ) )
    end
    -- This will start running `run` until it suspends, and will
    -- return a native wait object.
    return co_lua.wait_from_lua( run )
  end
end

-- Sometimes we want to catch and propagate any errors that
-- happen in a wait that is running in parallel (not awaited
-- on) but without awaiting on it. For those cases, this function
-- can be used to check the wait and propagate errors. Again,
-- this is only for parallel-running waits that
-- can't/shouldn't be awaited on.
--
-- Note: you may just want to use `auto_assert` below which will
-- automatically call this on waits at scope exit.
function M.assert( w )
  local err = w:error()
  assert( not err, err )
end

-- Takes a wait and will return an object that wraps the
-- wait (and exposes its interface methods) but will check
-- for errors when it is closed an propagate the error. This is
-- only needed for waits that are either never awaited on
-- (run in parallel and are then cancelled) or waits that are
-- at least not awaited on right away (ones that run in parallel
-- and are later joined on some code paths).
function M.auto_assert( w )
  M.assert( w )
  return setmetatable( {}, {
    __close=function()
      local _<close> = w
      M.assert( w )
    end,
    __index=function( _, k )
      if type( w[k] ) ~= 'function' then return w[k] end
      return function( _, ... )
        -- Assume that all function members of wait are
        -- "member functions" that take the wait as first pa-
        -- rameter.
        return w[k]( w, ... )
      end
    end,
  } )
end

return M
