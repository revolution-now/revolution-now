--[[-------------------------------------------------------------
--                      Loads main() module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local win = require( 'ide.win' )

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.matches( stem )
  return (stem == 'exe/main')
end

function M.open( stem )
  -- No distinction between wide/narrow here since we only have
  -- two files.
  win.edit( 'exe/main.cpp' )
end

return M
