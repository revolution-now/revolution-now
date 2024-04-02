--[[-------------------------------------------------------------
--                       Loads a C++ module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'ide.util' )
local LU = require( 'ide.layout-util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local exists = util.file_exists
local vsplit = LU.vsplit
local hsplit = LU.hsplit

-----------------------------------------------------------------
-- Private Functions.
-----------------------------------------------------------------
local function files( stem )
  local F = {}
  F.cpp = format( 'src/%s.cpp', stem )
  F.hpp = format( 'src/%s.hpp', stem )
  F.rds = format( 'src/%s.rds', stem )
  F.rds_impl = format( 'src/%s-impl.rds', stem )
  F.test = format( 'test/%s-test.cpp', stem )
  return F
end

-- For the non-wide monitors.
-- LuaFormatter off
local function layout_wide( stem )
  local F = files( stem )
  local plan = vsplit {
    {}, -- will be filled out.
    F.cpp,
    F.test,
  }
  local new_module = not exists( F.hpp ) and not exists( F.cpp )
  if exists( F.rds ) and exists( F.rds_impl ) then
    plan[1] = vsplit {
      hsplit {
        F.rds,
        F.rds_impl
      },
      F.hpp
    }
  elseif not exists( F.rds ) and exists( F.rds_impl ) then
    plan[1] = vsplit {
      F.rds_impl,
      F.hpp
    }
  elseif exists( F.rds ) or new_module then
    plan[1] = vsplit {
      F.rds,
      F.hpp
    }
  else
    plan[1] = F.hpp
  end
  return plan
end
-- LuaFormatter on

-- For the non-wide monitors.
-- LuaFormatter off
local function layout_narrow( stem )
  local F = files( stem )
  local plan = vsplit {
    {}, -- will be filled out.
    F.cpp,
    F.test,
  }
  local new_module = not exists( F.hpp ) and not exists( F.cpp )
  if exists( F.rds ) and exists( F.rds_impl ) then
    plan[1] = hsplit {
      F.rds,
      F.hpp,
      F.rds_impl
    }
  elseif not exists( F.rds ) and exists( F.rds_impl ) then
    plan[1] = hsplit {
      F.hpp,
      F.rds_impl
    }
  elseif exists( F.rds ) or new_module then
    plan[1] = hsplit {
      F.rds,
      F.hpp
    }
  else
    plan[1] = F.hpp
  end
  return plan
end
-- LuaFormatter on

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.create( stem )
  if util.is_wide() then
    return layout_wide( stem )
  else
    return layout_narrow( stem )
  end
end

return M
