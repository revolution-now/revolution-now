-----------------------------------------------------------------
--                 Creates the RN editor session.
------------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local rn = require( 'ide.contents.rn' )
local layout = require( 'ide.layout' )
local module_cpp = require( 'ide.module-cpp' )
local tabs = require( 'ide.tabs' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local call = vim.call
local fnamemodify = vim.fn.fnamemodify
local keymap = vim.keymap

-----------------------------------------------------------------
-- Layout Functions.
-----------------------------------------------------------------
local module_types = {
  require( 'ide.module-config' ), --
  require( 'ide.module-main' ), --
  require( 'ide.module-shader' ) --
}

local function open_module( stem )
  for _, module_type in ipairs( module_types ) do
    if module_type.matches( stem ) then
      return layout.open( module_type.create( stem ) )
    end
  end
  -- If the module doesn't exist as any recognized type then it
  -- is likely that we want to create a new C++ module.
  return layout.open( module_cpp.create( stem ) )
end

local function open_module_with_input()
  local stem = util.input( 'Enter name: ' )
  if stem == nil or #stem == 0 then return end
  open_module( stem )
end

local function create_tabs()
  for _, stem in ipairs( rn ) do
    open_module( stem )
    -- This is optional but allows us to see the tabs appearing
    -- as they are opened, which is cool.
    vim.cmd[[redraw!]]
  end
end

-----------------------------------------------------------------
-- Tabline.
-----------------------------------------------------------------
-- This function contains the logic that determines the name of a
-- tab given the list of buffers that are currently visible in
-- it.
--
-- buffer_list will be a list of buffer tables, e.g.:
--
--   buffers = {
--     1: {
--       buffer_idx = 123,
--       path = "/some/path/to/file.cpp"
--     },
--     2: {
--       buffer_idx = 567,
--       path = "/another/file.hpp"
--     }
--     ...
--   }
--
local function tab_namer( buffer_list )
  for _, buffer in ipairs( buffer_list ) do
    local path = buffer.path
    local ext = fnamemodify( path, ':e' )
    local stem = fnamemodify( path, ':t:r' )
    if ext == 'cpp' or ext == 'hpp' then
      path = fnamemodify( path, ':s|^src/||' )
      path = fnamemodify( path, ':s|^exe/||' )
      path = fnamemodify( path, ':s|^test/|../test/|' )
      path = fnamemodify( path, ':r' )
      return path
    end
    if ext == 'txt' then return 'doc/' .. stem end
    if ext == 'lua' then return 'lua/' .. stem end
    if ext == 'vert' then return 'shaders:' .. stem end
    if ext == 'frag' then return 'shaders:' .. stem end
  end
  -- We could not determine a name from any of the buffers, so
  -- just use the path of the filename of the first buffer.
  return buffer_list[1].path
end

-----------------------------------------------------------------
-- Key maps.
-----------------------------------------------------------------
local function nmap( keys, func )
  -- This will by default have remap=false.
  keymap.set( 'n', keys, func, { silent=true } )
end

local function mappings()
  nmap( '<C-p>', open_module_with_input )
  -- Add more here...
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
local function main()
  -- Create key mappings.
  mappings()

  -- Creates all of the tabs/splits.
  tabs.set_tab_namer( tab_namer )
  create_tabs()

  -- In case anyone changed it.
  vim.cmd[[tabdo set cmdheight=1]]
  vim.cmd[[tabdo wincmd =]]
  -- When nvim supports it.
  -- vim.cmd[[tabdo set cmdheight=0]]
end

main()
