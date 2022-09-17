--[[-------------------------------------------------------------
--                       Loads a C++ module.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'ide.util' )
local layout = require( 'ide.layout' )
local win = require( 'ide.win' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local exists = util.exists

-----------------------------------------------------------------
-- Private Functions.
-----------------------------------------------------------------
-- Ensures that if the test doesn't exist then it gets
-- template-initialized properly.
local function cpptest_initializer( file )
  if exists( file ) then
    win.edit( file )
    return
  end
  -- Turn off auto template initialization, create the file, then
  -- initailize it with the unit test template. If we don't do it
  -- this way then the file will be created and auto initialized
  -- with the regular cpp template.
  local old = vim.g.tmpl_auto_initialize
  vim.g.tmpl_auto_initialize = false
  win.edit( file )
  vim.g.tmpl_auto_initialize = old
  vim.cmd[[TemplateInit cpptest]]
  -- Not sure how to do this in lua.
  vim.cmd[[set nomodified]]
end

local function files( stem )
  local F = {}
  F.cpp = format( 'src/%s.cpp', stem )
  F.hpp = format( 'src/%s.hpp', stem )
  F.rds = format( 'src/%s.rds', stem )
  F.rds_impl = format( 'src/%s-impl.rds', stem )
  F.test = function()
    cpptest_initializer( format( 'test/%s.cpp', stem ) )
  end
  return F
end

-- For the non-wide monitors.
local function layout_wide( stem )
  local F = files( stem )
  -- LuaFormatter off
  local plan = {
    type='vsplit',
    what={
      {}, -- will be filled out.
      F.cpp,
      F.test,
    }
  }
  -- LuaFormatter on
  if exists( F.rds ) and exists( F.rds_impl ) then
    -- LuaFormatter off
    plan.what[1] = {
      type='vsplit',
      what={
        {
          type='hsplit',
          what={
            F.rds,
            F.rds_impl
          }
        },
        F.hpp
      }
    }
    -- LuaFormatter on
  elseif exists( F.rds ) then
    -- LuaFormatter off
    plan.what[1] = {
      type='vsplit',
      what={
        F.rds,
        F.hpp
      }
    }
    -- LuaFormatter on
  else
    plan.what[1] = F.hpp
  end
  layout.open( plan )
end

-- For the non-wide monitors.
local function layout_narrow( stem )
  local F = files( stem )
  -- LuaFormatter off
  local plan = {
    type='vsplit',
    what={
      {}, -- will be filled out.
      F.cpp,
      F.test,
    }
  }
  -- LuaFormatter on
  if exists( F.rds ) and exists( F.rds_impl ) then
    -- LuaFormatter off
    plan.what[1] = {
      type='hsplit',
      what={
        F.rds,
        F.hpp,
        F.rds_impl
      }
    }
    -- LuaFormatter on
  elseif exists( F.rds ) then
    -- LuaFormatter off
    plan.what[1] = {
      type='hsplit',
      what={
        F.rds,
        F.hpp
      }
    }
    -- LuaFormatter on
  else
    plan.what[1] = F.hpp
  end
  layout.open( plan )
end

-----------------------------------------------------------------
-- Public API.
-----------------------------------------------------------------
function M.open( stem )
  if util.is_wide() then
    layout_wide( stem )
  else
    layout_narrow( stem )
  end
end

return M
