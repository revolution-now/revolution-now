--[[-------------------------------------------------------------
--                  Creates the editor session.
--]] -------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local contents = require( 'ide.contents' )
local util = require( 'ide.util' )
local layout = require( 'ide.layout' )
local win = require( 'ide.win' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local call = vim.call
local keymap = vim.keymap
local format = string.format
local match = string.match
local exists = util.exists

-----------------------------------------------------------------
-- Vim command wrappers.
-----------------------------------------------------------------
-- Source a file.
local source = util.fmt_func_with_file_arg( 'source' )

local function nmap( keys, func )
  -- This will by default have remap=false.
  keymap.set( 'n', keys, func, { silent=true } )
end

-----------------------------------------------------------------
-- Layout Functions.
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

local function open_cpp_module_wide( stem )
  print( '  - ' .. stem )

  -- Components.
  local C = {}
  C.cpp = format( 'src/%s.cpp', stem )
  C.hpp = format( 'src/%s.hpp', stem )
  C.rds = exists( 'src/%s.rds', stem )
  C.rds_impl = exists( 'src/%s-impl.rds', stem )
  C.test = format( 'test/%s.cpp', stem )
  local config_name = match( stem, '^config/(.*)' )
  C.config = config_name and
                 format( 'config/rcl/%s.rcl', config_name )

  local edit_test = function() cpptest_initializer( C.test ) end

  if C.config then
    -- LuaFormatter off
    layout.open {
      type='vsplit',
      what={
        C.rds,
        C.hpp,
        C.cpp,
        C.config,
      }
    }
    -- LuaFormatter on
    return
  end

  -- Assume C++ module (even if it doesn't exist that is ok, we
  -- want to create it).
  -- LuaFormatter off
  local plan = {
    type='vsplit',
    what={
      {}, -- will be filled in
      C.cpp,
      edit_test,
    }
  }
  -- LuaFormatter on
  if C.rds and C.rds_impl then
    -- LuaFormatter off
    plan.what[1] = {
      type='vsplit',
      what={
        {
          type='hsplit',
          what={
            C.rds,
            C.rds_impl
          }
        },
        C.hpp
      }
    }
    -- LuaFormatter on
  elseif C.rds then
    plan.what[1] = { type='vsplit', what={ C.rds, C.hpp } }
  else -- no rds's
    plan.what[1] = C.hpp
  end

  layout.open( plan )
end

local function open_cpp_module_narrow( stem )
  print( '  - ' .. stem )

  -- Components.
  local C = {}
  C.cpp = format( 'src/%s.cpp', stem )
  C.hpp = format( 'src/%s.hpp', stem )
  C.rds = exists( 'src/%s.rds', stem )
  C.rds_impl = exists( 'src/%s-impl.rds', stem )
  C.test = format( 'test/%s.cpp', stem )
  local config_name = match( stem, '^config/(.*)' )
  C.config = config_name and
                 format( 'config/rcl/%s.rcl', config_name )

  local edit_test = function() cpptest_initializer( C.test ) end

  if C.config then
    -- LuaFormatter off
    layout.open {
      type='vsplit',
      what={
        C.rds,
        {
          type='hsplit',
          what={
            C.hpp,
            C.cpp,
          }
        },
        C.config,
      }
    }
    -- LuaFormatter on
    return
  end

  -- Assume C++ module (even if it doesn't exist that is ok, we
  -- want to create it).
  -- LuaFormatter off
  local plan = {
    type='vsplit',
    what={
      {}, -- will be filled in
      C.cpp,
      edit_test,
    }
  }
  -- LuaFormatter on
  if C.rds and C.rds_impl then
    plan.what[1] = {
      type='hsplit',
      what={ C.rds, C.hpp, C.rds_impl }
    }
  elseif C.rds then
    plan.what[1] = { type='hsplit', what={ C.rds, C.hpp } }
  else -- no rds's
    plan.what[1] = C.hpp
  end

  layout.open( plan )
end

local function open_cpp_module( stem )
  local columns = vim.o.columns
  local text_columns_per_split = 65
  local desired_columns_per_split = text_columns_per_split + 1
  if columns >= 4 * desired_columns_per_split then
    return open_cpp_module_wide( stem )
  else
    return open_cpp_module_narrow( stem )
  end
end

local function open_shader_pair( stem )
  print( '  - ' .. stem )
  win.tab( 'src/%s.vert', stem )
  win.vsplit( 'src/%s.frag', stem )
  win.wincmd( 'h' )
end

local function open_module( name )
  if exists( 'src/' .. name .. '.vert' ) then
    open_shader_pair( name )
    return
  end
  open_cpp_module( name )
end

local function open_module_with_input()
  local curline = call( 'getline', '.' )
  call( 'inputsave' )
  local name = util.input( 'Enter name: ' )
  call( 'inputrestore' )
  open_module( name )
end

local function create_layout()
  local total_tabs = #contents.stems
  if contents.OPEN_MAIN then total_tabs = total_tabs + 1 end

  -- This needs to be the number of message output (echo) other-
  -- wise you will get 'Press ENTER to continue...'.
  local lines = 1 + total_tabs
  vim.o.cmdheight = lines

  if contents.OPEN_MAIN then
    print( 'opening main...' )
    win.edit( 'exe/main.cpp' )
  end

  print( 'opening cpps...' )
  for _, s in ipairs( contents.stems ) do open_module( s ) end

  vim.cmd[[tabdo set cmdheight=1]]
  vim.cmd[[tabdo wincmd =]]

  -- When nvim supports it.
  -- vim.cmd[[tabdo set cmdheight=0]]
end

-----------------------------------------------------------------
-- Key maps.
-----------------------------------------------------------------
local function mappings()
  nmap( '<C-p>', open_module_with_input )
  -- Add more here...
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
function M.main()
  source( '.vimrc' )

  -- Create key mappings.
  mappings()

  -- Creates all of the tabs/splits.
  create_layout()
end

return M
