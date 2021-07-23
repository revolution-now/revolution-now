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
  assert( type( waitable.set_resume ) == 'function',
          'await called on invalid waitable type. This may ' ..
              'mean that the waitable type\'s usertype was not ' ..
              'registered.' )

  -- DO NOT put anything more before this line.
  local closer<close> = waitable

  assert( coroutine.isyieldable(), 'This function can only ' ..
              'called from within a coroutine.' )

  local thread, is_main = coroutine.running()
  assert( not is_main )

  if waitable:ready() then return waitable:get() end

  waitable:set_resume( thread )
  coroutine.yield()
  local err = waitable:error()
  -- 2 means to use the source location of the calling function
  -- in the error message, which will likely be more useful.
  if err ~= nil then error( err, 2 ) end
  assert( waitable:ready() )
  return waitable:get()
end

-- You can wrap a function using this so that you don't have to
-- call await( ... ) on it. However, we still need to have access
-- to the unwrapped versions in general so that we can run mul-
-- tiple coroutines in parallel and await on all of them.
function M.wrap( f )
  return function( ... ) return M.await( f( ... ) ) end
end

return M
