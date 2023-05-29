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

-- Counts the number of key/value pairs in a table.
function M.table_size( tbl )
  local count = 0
  for k, v in pairs( tbl ) do count = count + 1 end
  return count
end

return M
