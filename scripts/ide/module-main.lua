--[[-------------------------------------------------------------
--                      Loads main() module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.matches( stem )
  return (stem == 'exe/main')
end

function M.create( stem )
  -- No distinction between wide/narrow here since we only have
  -- two files.
  return 'exe/main.cpp'
end

return M
