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
local insert = table.insert

-----------------------------------------------------------------
-- Private Functions.
-----------------------------------------------------------------
local function files( dir, stem )
  local F = {}
  F.cpp = format( '%s/%s.cpp', dir, stem )
  F.hpp = format( '%s/%s.hpp', dir, stem )
  F.rds = format( '%s/%s.rds', dir, stem )
  F.rds_impl = format( '%s/%s-impl.rds', dir, stem )
  F.rds_iface = format( '%s/i%s.rds', dir, stem )
  F.test = format( 'test/%s-test.cpp', stem )
  return F
end

-- For large monitors.
local function layout_wide( dir, stem )
  local F = files( dir, stem )
  local plan = vsplit{
    {}, -- will be filled out.
    F.cpp, F.test,
  }
  local new_module = not exists( F.hpp ) and not exists( F.cpp )
  local rds = {}
  if exists( F.rds ) or new_module then insert( rds, F.rds ) end
  if exists( F.rds_iface ) then insert( rds, F.rds_iface ) end
  if exists( F.rds_impl ) then insert( rds, F.rds_impl ) end
  if #rds > 0 then
    plan[1] = vsplit{ hsplit( rds ), F.hpp }
  else
    plan[1] = F.hpp
  end
  return plan
end

-- For small monitors.
local function layout_narrow( dir, stem )
  local F = files( dir, stem )
  local plan = vsplit{
    {}, -- will be filled out.
    F.cpp, F.test,
  }
  local new_module = not exists( F.hpp ) and not exists( F.cpp )
  local left = {}
  if exists( F.rds ) or new_module then insert( left, F.rds ) end
  if exists( F.hpp ) or new_module then insert( left, F.hpp ) end
  if exists( F.rds_iface ) then insert( left, F.rds_iface ) end
  if exists( F.rds_impl ) then insert( left, F.rds_impl ) end
  plan[1] = hsplit( left )
  return plan
end

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.create( stem )
  local dir = 'src'
  if stem == 'main' then dir = 'exe' end
  if util.is_wide() then
    return layout_wide( dir, stem )
  else
    return layout_narrow( dir, stem )
  end
end

return M
