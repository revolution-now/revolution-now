-----------------------------------------------------------------
-- Some helpers for more terse configs.
-----------------------------------------------------------------
local M = {}

function M.hsplit( tbl )
  tbl.type = 'hsplit'
  return tbl
end

function M.vsplit( tbl )
  tbl.type = 'vsplit'
  return tbl
end

return M