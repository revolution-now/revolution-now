--[[-------------------------------------------------------------
--                      Loads a config module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'ide.util' )
local layout = require( 'ide.layout' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local match = string.match
local format = string.format

-----------------------------------------------------------------
-- Private Functions.
-----------------------------------------------------------------
local function rcl_path( stem )
  local name = match( stem, '^config/(.*)' )
  return name and format( 'config/rcl/%s.rcl', name )
end

local function files( stem )
  local F = {}
  F.cpp = format( 'src/%s.cpp', stem )
  F.hpp = format( 'src/%s.hpp', stem )
  F.rds = format( 'src/%s.rds', stem )
  F.rcl = rcl_path( stem )
  return F
end

local function layout_wide( stem )
  local rcl = assert( rcl_path( stem ) )
  local F = files( stem )
  -- LuaFormatter off
  layout.open{
    type='vsplit',
    what={
      F.rds,
      F.hpp,
      F.cpp,
      F.rcl,
    }
  }
  -- LuaFormatter on
end

local function layout_narrow( stem )
  local rcl = assert( rcl_path( stem ) )
  local F = files( stem )
  -- LuaFormatter off
  layout.open {
    type='vsplit',
    what={
      F.rds,
      {
        type='hsplit',
        what={
          F.hpp,
          F.cpp,
        }
      },
      F.rcl,
    }
  }
  -- LuaFormatter on
end

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.matches( stem ) return rcl_path( stem ) ~= nil end

function M.open( stem )
  if util.is_wide() then
    layout_wide( stem )
  else
    layout_narrow( stem )
  end
end

return M
