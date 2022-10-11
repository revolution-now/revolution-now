--[[ ------------------------------------------------------------
|
| limits.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-19.
|
| Description: Helpers for working with limits on numbers.
|
--]] ------------------------------------------------------------
local M = {}

-- Enforces that n is in [min, max].
function M.clamp( n, min, max )
  if n < min then return min end
  if n > max then return max end
  return n
end

return M
