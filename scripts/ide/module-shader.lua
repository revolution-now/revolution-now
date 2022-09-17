--[[-------------------------------------------------------------
--                      Loads a shader module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local layout = require( 'ide.layout' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local exists = util.exists

-----------------------------------------------------------------
-- Private Functions.
-----------------------------------------------------------------
local function files( stem )
  local F = {}
  F.vert = format( 'src/%s.vert', stem )
  F.frag = format( 'src/%s.frag', stem )
  return F
end

local function layout_all( stem )
  local F = files( stem )
  -- LuaFormatter off
  layout.open {
    type='vsplit',
    what={
      F.vert,
      F.frag,
    }
  }
  -- LuaFormatter on
end

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.matches( stem )
  return exists( 'src/' .. stem .. '.vert' )
end

function M.open( stem )
  -- No distinction between wide/narrow here since we only have
  -- two files.
  layout_all( stem )
end

return M
