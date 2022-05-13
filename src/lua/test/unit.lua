--[[ ------------------------------------------------------------
|
| unit.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-13.
|
| Description: Lua Unit Testing Framework.
|
--]] ------------------------------------------------------------
local M = {}

function M.ASSERT_EQ( l, r, name )
  if l == r then return end
  error( name .. ' are not equal: ' .. tostring( l ) .. ' != ' ..
             tostring( r ) )
end

function M.new_test_pack()
  -- TODO: add metatable to prevent duplicate test names.
  return {}
end

function M.runner( quiet, pack )
  for name, test in pairs( pack ) do
    if not quiet then
      io.write( 'running test ' .. name .. '...' )
    end
    test()
    if not quiet then io.write( ' passed.\n' ) end
  end
end

return M
