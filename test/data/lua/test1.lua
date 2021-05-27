--[[ ------------------------------------------------------------
|
| test.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-03-06.
|
| Description: Used for testing.
|
--]] ------------------------------------------------------------
M = {}

function M.foo( i ) return 'hello world: ' .. tostring( i ) end

package_exports = {}

return M