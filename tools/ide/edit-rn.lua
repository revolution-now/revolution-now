-----------------------------------------------------------------
--                 Creates the RN editor session.
------------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local layout = require( 'ide.layout' )
local module_cpp = require( 'ide.module-cpp' )
local tabs = require( 'ide.tabs' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local fnamemodify = vim.fn.fnamemodify
local keymap = vim.keymap
local glob = vim.fn.glob
local format = string.format

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local TABS_FILE = 'tools/ide/contents/rn.lua'
local TABS_MODULE = 'ide.contents.rn'

-----------------------------------------------------------------
-- Layout Functions.
-----------------------------------------------------------------
local module_types = {
  require( 'ide.module-config' ), --
  require( 'ide.module-main' ), --
  require( 'ide.module-shader' ), --
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

-----------------------------------------------------------------
-- Tabs.
-----------------------------------------------------------------
local function create_tabs()
  for _, stem in ipairs( require( TABS_MODULE ) ) do
    open_module( stem )
    -- This is optional but allows us to see the tabs appearing
    -- as they are opened, which is cool.
    vim.cmd[[redraw!]]
  end
end

-- This function contains the logic that determines the name of a
-- tab given the list of buffers that are visible in it.
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

local function save_tabs()
  local tab_list = tabs.tab_config()
  local out = assert( io.open( TABS_FILE, 'w' ) )
  local function writeln( line )
    out:write( tostring( line or '' ) .. '\n' )
  end
  writeln( '-- List of modules to open when we edit RN.' )
  writeln( 'return {' )
  for _, tab in ipairs( tab_list ) do
    writeln( '  \'' .. tab_namer( tab.buffers ) .. '\', --' )
  end
  writeln( '}' )
  out:close()
end

local function find_all_modules()
  -- Extensions of files that, if found, mean that their stem
  -- should be added to the pool of module names.
  local EXTS = { '?pp', 'rds', 'vert', 'frag' }
  local rn = util.rn_root_dir()
  local modules = {}
  local function add_glob( ext )
    local files = glob( format( '%s/src/**/*.%s', rn, ext ) )
    for file in files:gmatch( '[^\r\n]+' ) do
      local module_name = file:match( '.*/src/(.+)%..*' )
      modules[module_name] = true
    end
  end
  for _, ext in ipairs( EXTS ) do add_glob( ext ) end
  local res = {}
  for k, _ in pairs( modules ) do table.insert( res, k ) end
  table.sort( res )
  return res
end

local function module_opener( stem )
  open_module( stem )
  save_tabs()
end

local function open_module_with_telescope()
  -- This is non-blocking, so this function will return right
  -- away, while the picker window is still open.
  local tl = require( 'ide.telescope' )
  tl.pick_module( find_all_modules(), module_opener )
end

-----------------------------------------------------------------
-- Quitting.
-----------------------------------------------------------------
local function quit_all_and_save_tabs()
  save_tabs()
  -- Close all tabs and quit. This will refuse to exit if there
  -- are unsaved changes in some buffer.
  vim.cmd[[qa]]
end

local function close_tab_and_save_tabs()
  require( 'dsicilia.tabs' ).close_current_tab()
  save_tabs()
end

-----------------------------------------------------------------
-- Key maps.
-----------------------------------------------------------------
local function nmap( keys, func )
  -- This will by default have remap=false.
  keymap.set( 'n', keys, func, { silent=true } )
end

local function mappings()
  nmap( '<C-p>', open_module_with_telescope )
  nmap( 'Q', quit_all_and_save_tabs )
  nmap( 'E', close_tab_and_save_tabs )
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
local function main()
  -- Create key mappings.
  mappings()

  -- Add snippets.
  require( 'ide.snippets' )

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
