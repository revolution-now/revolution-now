--[[-------------------------------------------------------------
--                      Loads a shader module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
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
  return {
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

function M.create( stem )
  -- No distinction between wide/narrow here since we only have
  -- two files.
  return layout_all( stem )
end

return M
