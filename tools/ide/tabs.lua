-----------------------------------------------------------------
-- Helpers for working with tabs.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local call = vim.call

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
function M.num_tabs() return call( 'tabpagenr', '$' ) end

-- 1-based, so compatible with Lua by default.
function M.current_tab() return call( 'tabpagenr' ) end

-- Selects the nth tab, 1-based.
function M.set_selected_tab( n ) vim.cmd( 'tabn ' .. n ) end

-- Given the buffer number, get the file name.
local function buffer_name( n )
  return call( 'bufname', assert( n ) )
end

-- Returns a list of buffer indices (integers) that are open in
-- the given tab page (which starts at 1).
local function tab_page_buffer_list( n )
  return call( 'tabpagebuflist', n )
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
--      buffers = {
--        1: {
--          buffer_idx = 123,
--          path = "/some/path/to/file.cpp"
--        },
--        2: {
--          buffer_idx = 567,
--          path = "/another/file.hpp"
--        }
--        ...
--      }
--    },
--
--    2: {
--      active = true,
--      idx = 2,
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
    for _, buf_idx in ipairs( buf_list ) do
      local buffer = {}
      table.insert( tab.buffers, buffer )
      buffer.buffer_idx = buf_idx
      buffer.path = buffer_name( buf_idx )
    end
  end
  return res
end

-- Takes a function that takes a list of buffer tables (see
-- above) and returns a name for the tab.
local function construct_tabline( namer )
  local tab_config = M.tab_config()
  local list = {}
  for idx, tab in ipairs( tab_config ) do
    local tab_fmt
    -- Select the highlighting.
    if idx == M.current_tab() then
      tab_fmt = '%#TabLineSel#'
    else
      tab_fmt = '%#TabLine#'
    end
    -- Set the tab page number (for mouse clicks).
    tab_fmt = tab_fmt .. format( '%%%dT', idx )
    -- Set the tab label.
    tab_fmt = tab_fmt .. format( ' %s ', namer( tab.buffers ) )
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
