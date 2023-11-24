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

-- Takes a table and converts it to a list of {k=<key>, v=<val>}
-- tables where the list of sorted by key.
function M.sort_by_key( tbl )
  local res = {}
  for k, v in pairs( tbl ) do table.insert( res, { k=k, v=v } ) end
  table.sort( res, function( l, r ) return l.k < r.k end )
  return res
end

return M
