local M = {}

-----------------------------------------------------------------
-- List of modules to open.
-----------------------------------------------------------------
-- LuaFormatter off

M.stems = {
  'ss/market',
  'config/market',
  'price-group',
  'market',
  'harbor-view-market',
}

-- LuaFormatter on

-----------------------------------------------------------------
-- Tunables.
-----------------------------------------------------------------
M.OPEN_MAIN = true

return M
