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
local lsp_comp = require( 'dsicilia.lsp-completion' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local augroup = vim.api.nvim_create_augroup
local autocmd = vim.api.nvim_create_autocmd
local buf_get_name = vim.api.nvim_buf_get_name
local fnamemodify = vim.fn.fnamemodify
local getbufinfo = vim.fn.getbufinfo
local keymap = vim.keymap
local glob = vim.fn.glob
local format = string.format
local file_exists = util.file_exists
local find_quoted_header = lsp_comp.find_quoted_header
local setreg = vim.fn.setreg
local expand = vim.fn.expand

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

local function file_to_module( path )
  local ext = fnamemodify( path, ':e' )
  local stem = fnamemodify( path, ':t:r' )
  if ext == 'cpp' or ext == 'hpp' then
    path = fnamemodify( path, ':s|^src/||' )
    path = fnamemodify( path, ':s|^exe/||' )
    path = fnamemodify( path, ':s|^test/|../test/|' )
    path = fnamemodify( path, ':r:r' )
    return path
  end
  if ext == 'vert' or ext == 'frag' then
    path = fnamemodify( path, ':s|^src/||' )
    path = fnamemodify( path, ':r' )
    return path
  end
  if ext == 'txt' then return 'doc/' .. stem end
  if ext == 'lua' then return 'lua/' .. stem end
end

-----------------------------------------------------------------
-- Tabs.
-----------------------------------------------------------------
local function create_tabs()
  local ok, modules = pcall( require, TABS_MODULE )
  modules = ok and modules or { 'exe/main' }
  for _, stem in ipairs( modules ) do
    open_module( stem )
    -- This is optional but allows us to see the tabs appearing
    -- as they are opened, which is cool.
    vim.cmd[[redraw!]]
  end
  local curr_tab = modules.current_tab or 1
  return curr_tab
end

-- This function contains the logic that determines the name of a
-- tab given the list of buffers that are visible in it.
local function tab_namer( buffer_list )
  for _, buffer in ipairs( buffer_list ) do
    local path = buffer.path
    local module = file_to_module( path )
    if module then return module end
  end
  -- We could not determine a name from any of the buffers, so
  -- just use the path of the filename of the first buffer.
  return buffer_list[1].path
end

local function save_tabs()
  local current_tab = tabs.current_tab()
  local tab_list = tabs.tab_config()
  local out = assert( io.open( TABS_FILE, 'w' ) )
  local function writeln( line )
    out:write( tostring( line or '' ) .. '\n' )
  end
  writeln( '-- List of modules to open when we edit RN.' )
  writeln( 'return {' )
  for idx, tab in ipairs( tab_list ) do
    writeln( '  \'' .. tab_namer( tab.buffers ) .. '\', -- ' ..
                 tostring( idx ) )
  end
  local curr_tab_name =
      tab_namer( tab_list[current_tab].buffers )
  writeln()
  writeln( '  -- 1-based.' )
  writeln( '  current_tab=' .. current_tab .. ', -- ' ..
               curr_tab_name )
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
-- Module Finder.
-----------------------------------------------------------------
local function open_module_from_hover()
  local header = find_quoted_header()
  if not header then return end
  local identifier = expand( '<cword>' )
  local module = file_to_module( header )
  module_opener( module )
  setreg( '/', format( [[\<%s\>]], identifier ) )
end

-----------------------------------------------------------------
-- Quitting.
-----------------------------------------------------------------
local function quit_all_and_save_tabs()
  save_tabs()
  -- Close all tabs and quit. This will refuse to exit if there
  -- are unsaved changes in some buffer.
  require( 'dsicilia.quitting' ).quit_all()
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
  nmap( '<Leader>lm', open_module_from_hover )
end

-----------------------------------------------------------------
-- Rds file change detection and auto-recompilation.
-----------------------------------------------------------------
-- Returns a buffer table corresponding to some random hpp/cpp
-- buffer that is currently loaded in the editor.
local function find_any_loaded_cpp_src()
  local bufs = getbufinfo{ buflisted=1, bufloaded=1 }
  for _, buf in ipairs( bufs ) do
    if buf.name:match( '.*%.[ch]pp$' ) then return buf end
  end
  return nil
end

-- This produces a command for running rdsc. It should be kept in
-- sync with whatever the actual command is in the cmake files.
local function cmd_to_recompile_rds( rds_name )
  assert( rds_name )
  local PREAMBLE = 'src/rds/rdsc/preamble.lua'
  local RDSC = 'src/rds/rdsc/rdsc'
  local root = util.rn_root_dir()
  local build = ('%s/.builds/current'):format( root )
  local rdsc = ('%s/%s'):format( build, RDSC )
  local preamble = ('%s/%s'):format( root, PREAMBLE )
  local out_path = rds_name:sub( #root + 2 )
  local hpp = ('%s/%s.hpp'):format( build, out_path )
  local function check_exists( f )
    if not file_exists( f ) then
      error( format( '%s does not exist.', f ) )
    end
  end
  check_exists( rdsc )
  -- Command and args.
  return { rdsc, rds_name, preamble, hpp }
end

local function on_rds_written( buf )
  -- We need to find some (any) cpp/hpp source file that is
  -- loaded because it will be attached to the lsp, and so we can
  -- use it to get neovim to send events to the lsp for us, even
  -- though it is the rds file that has changed.
  local cppbuf = find_any_loaded_cpp_src()
  if not cppbuf then return end
  local rds_name = assert( buf_get_name( buf ) )
  local rds_cmd = cmd_to_recompile_rds( rds_name )
  util.shell_command( unpack( rds_cmd ) )
  -- This will trigger a BufWritePost callback defined in the
  -- neovim lsp lua module that will send a textDocument/didSave
  -- to the lsp for the given cpp buffer which, for clangd, will
  -- cause it to check all open buffers for recompilation, which
  -- is what we want.
  vim.api.nvim_exec_autocmds( 'BufWritePost', {
    buffer=cppbuf.bufnr,
    modeline=false,
  } )
end

-- This will setup some callbacks so that each time we save
-- changes to an rds file, it will automatically get recompiled
-- using rdsc and then the LSP client will be told to check for
-- recompilation on all of the open cpp source files.
local function setup_rds_change_detector()
  autocmd( 'BufWritePost', {
    group=augroup( 'RdsChange', { clear=true } ),
    pattern={ '*.rds' },
    desc='recompiles rds files when changed and notifies the lsp.',
    callback=function( ctx )
      local ok, err = pcall( on_rds_written, ctx.buf )
      if not ok then
        print( 'error compiling rds: ' .. err .. '\n' )
      end
    end,
  } )
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
local function main()
  -- Create key mappings.
  mappings()

  -- Add snippets.
  require'ide.snippets'

  -- Creates all of the tabs/splits.
  tabs.set_tab_namer( tab_namer )
  local curr_tab = create_tabs()

  -- In case anyone changed it.
  vim.cmd[[tabdo wincmd =]]
  -- When nvim supports it.
  vim.cmd[[tabdo set cmdheight=0]]

  -- Enable rds auto-recompilation.
  setup_rds_change_detector();

  tabs.set_selected_tab( curr_tab )
end

main()
