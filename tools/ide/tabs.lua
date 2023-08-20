-----------------------------------------------------------------
-- Helpers for working with tabs.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local dslsp = require( 'dsicilia.lsp' )
local colors = require( 'dsicilia.colors' )
local palette = require( 'gruvbox.palette' )
local dsstatus = require( 'dsicilia.status-bar' )

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local autocmd = vim.api.nvim_create_autocmd
local augroup = vim.api.nvim_create_augroup

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
function M.num_tabs() return vim.fn.tabpagenr( '$' ) end

-- 1-based, so compatible with Lua by default.
function M.current_tab() return vim.fn.tabpagenr() end

-- Selects the nth tab, 1-based.
function M.set_selected_tab( n ) vim.cmd( 'tabn ' .. n ) end

-- Given the buffer number, get the file name.
local function buffer_name( n )
  return vim.fn.bufname( assert( n ) )
end

-- Returns a list of buffer indices (integers) that are open in
-- the given tab page (which starts at 1).
local function tab_page_buffer_list( n )
  return vim.fn.tabpagebuflist( n )
end

-- Returns a data structure describing the currently open tabs
-- that looks like the following. Note that it doesn't include
-- any information on the pane structure within each tab because
-- this is used only for tab naming purposes.
--
--  {
--    1: {
--      active = false,
--      idx = 1,
--      diagnostics = { errors=1, warnings=2, infos=0, hints=0 },
--      compiling = false,
--      buffers = {
--        1: {
--          buffer_idx = 123,
--          path = "/some/path/to/file.cpp",
--          compiling = false,
--          diagnostics = { errors=1, warnings=0, infos=0, hints=0 },
--        },
--        2: {
--          buffer_idx = 567,
--          path = "/another/file.hpp",
--          compiling = false,
--          diagnostics = { errors=0, warnings=2, infos=0, hints=0 },
--        }
--        ...
--      }
--    },
--
--    2: {
--      active = true,
--      idx = 2,
--      diagnostics = { errors=0, warnings=0, infos=0, hints=0 },
--      compiling = true,
--      buffers = {
--        ...
--      }
--    },
--
--    ...
--  }
--
function M.tab_config()
  local res = {}
  local curr = M.current_tab()
  for i = 1, M.num_tabs() do
    local tab = {}
    res[i] = tab
    tab.active = (i == curr)
    tab.idx = i
    -- Fill in buffers.
    tab.buffers = {}
    local buf_list = tab_page_buffer_list( i )
    tab.diagnostics = { errors=0, warnings=0, infos=0, hints=0 }
    tab.compiling = false
    for _, buf_idx in ipairs( buf_list ) do
      local buffer = {}
      table.insert( tab.buffers, buffer )
      buffer.buffer_idx = buf_idx
      buffer.path = buffer_name( buf_idx )
      buffer.diagnostics =
          dslsp.diagnostics_for_buffer( buf_idx )
      for k, v in pairs( buffer.diagnostics ) do
        tab.diagnostics[k] = tab.diagnostics[k] + v
      end
      buffer.compiling = dsstatus.is_buffer_compiling( buf_idx )
      tab.compiling = tab.compiling or buffer.compiling
    end
  end
  return res
end

colors.hl_setter( 'IdeTabColors', function( hi )
  local P = palette.colors
  hi.TabLineError = { fg=P.bright_red, bg=P.dark1,
                      underline=true }
  hi.TabLineWarning = {
    fg=P.bright_yellow,
    bg=P.dark1,
    underline=true,
  }
  hi.TabLineCompiling = {
    fg=P.bright_orange,
    bg=P.dark1,
    underline=false,
  }
end )

-- Takes a function that takes a list of buffer tables (see
-- above) and returns a name for the tab.
local function construct_tabline( namer )
  local tab_config = M.tab_config()
  local list = {}
  for idx, tab in ipairs( tab_config ) do
    local tab_fmt
    -- Select the highlighting. Note that some of these have the
    -- space on the left and some on the right. That is because
    -- for the ones that underline the text we don't want them to
    -- include the space otherwise the space will be underlined
    -- which does not look good.
    if idx == M.current_tab() then
      tab_fmt = '%#TabLineSel# '
    elseif tab.compiling then
      tab_fmt = ' %#TabLineCompiling#'
    elseif tab.diagnostics.errors > 0 then
      tab_fmt = ' %#TabLineError#'
    elseif tab.diagnostics.warnings > 0 then
      tab_fmt = ' %#TabLineWarning#'
    else
      tab_fmt = '%#TabLine# '
    end
    -- Set the tab page number (for mouse clicks).
    tab_fmt = tab_fmt .. format( '%%%dT', idx )
    -- Set the tab label.
    tab_fmt = tab_fmt .. namer( tab.buffers )

    -- Similar idea as above regarding spaces.
    if idx == M.current_tab() then
      tab_fmt = tab_fmt .. ' %#TabLine#'
    elseif tab.diagnostics.errors > 0 then
      tab_fmt = tab_fmt .. '%#TabLine# '
    elseif tab.diagnostics.warnings > 0 then
      tab_fmt = tab_fmt .. '%#TabLine# '
    else
      tab_fmt = tab_fmt .. ' %#TabLine#'
    end
    table.insert( list, tab_fmt )
  end

  -- After the last tab fill with TabLineFill and reset tab page
  -- number.
  table.insert( list, '%#TabLineFill#%T' )

  -- Right-align the label to close the current tab page.
  if M.num_tabs() > 1 then
    table.insert( list, '%=%#TabLine#%999XX' )
  end

  return table.concat( list )
end

-- This holds the function used to generate the tabline.
M._ide_tabline_generator = nil

autocmd( 'DiagnosticChanged', {
  group=augroup( 'IdeTabs', { clear=true } ),
  callback=function( _ ) vim.cmd.redrawtabline() end,
} )

-- This allows us to redraw the status bar whenever the status of
-- a buffer changes to/from "compiling", which is not always cap-
-- tured by DiagnosticChanged (though sometimes it seems to be).
dsstatus.register_filestatus_hook( function()
  vim.cmd.redrawtabline()
end )

-- Takes a tab namer (i.e., a function that takes a list of
-- buffer tables and returns a name for the tab) and sets the
-- tabline such that it will use that namer function on each tab
-- to set the tab names.
function M.set_tab_namer( tab_namer )
  if tab_namer == nil then
    M._ide_tabline_generator = nil
    vim.opt.tabline = nil;
    return
  end
  assert( type( tab_namer ) == 'function' )
  M._ide_tabline_generator = function()
    return construct_tabline( tab_namer )
  end
  vim.opt.tabline =
      '%!v:lua.require\'ide.tabs\'._ide_tabline_generator()'
end

return M
