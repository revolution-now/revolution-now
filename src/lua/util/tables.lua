--[[ ------------------------------------------------------------
|
| tables.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-09-04.
|
| Description: Helpers for dealing with tables.
|
--]] ------------------------------------------------------------
local M = {}

-- Clones a table.  But note that this is not a deep clone.
function M.copy_table( tbl )
  local res = {}
  for k, v in pairs( tbl ) do res[k] = v end
  return res
end

return M