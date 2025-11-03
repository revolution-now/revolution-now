--[[ ------------------------------------------------------------
|
| meta.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-23.
|
| Description: Meta utilities.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
-- Takes a table and just returns a new table with all the same
-- key/value pairs that come from the pairs(...) iterator. This
-- is useful if e.g. a table's pairs are produced by a __pairs
-- metafunction that we either can't call (luapp doesn't cur-
-- rently call it when iterating over a table) or don't want to
-- call (maybe it's expensive).
function M.all_pairs( tbl )
  local res = {}
  for k, v in pairs( tbl ) do res[k] = v end
  return res
end

return M
