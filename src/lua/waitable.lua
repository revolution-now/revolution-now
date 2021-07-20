--[[ ------------------------------------------------------------
|
| waitable.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-07-01.
|
| Description: Helpers for using Lua coroutines with C++
|              waitables.
|
--]] ------------------------------------------------------------
local M = {}

assert( coroutine, 'The coroutine library must be available.' )

function M.await( waitable )
  assert( type( waitable ) == 'userdata',
          'await should only be called on native waitable types.' )
  -- Must be first.
  local closer<close> = waitable

  local thread, is_main = coroutine.running()
  assert( not is_main )

  if waitable:ready() then return waitable:get() end

  waitable:set_resume( thread )
  assert( coroutine.isyieldable() )
  coroutine.yield()
  local err = waitable:error()
  -- 2 means to use the source location of the calling function
  -- in the error message, which will likely be more useful.
  if err ~= nil then error( err, 2 ) end
  assert( waitable:ready() )
  return waitable:get()
end

-- TODO: rewrite this in C++.
function M.runner( set_result, set_error, func, ... )
  local success, result = pcall( func, ... )
  if not success then
    set_error( tostring( result ) )
    return
  end
  set_result( result )
end

-- You can wrap a function using this so that you don't have to
-- call await( ... ) on it. However, we still need to have access
-- to the unwrapped versions in general so that we can run mul-
-- tiple coroutines in parallel and await on all of them.
function M.wrap( f )
  return function( ... ) return M.await( f( ... ) ) end
end

return M
