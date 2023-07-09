-----------------------------------------------------------------
-- Library for generating a split layout within a tab.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local win = require( 'ide.win' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Layout
-----------------------------------------------------------------
local Layout = {
  vsplit=function( self, layout )
    local num = #layout
    -- Create all the splits.
    for _ = 1, num - 1 do win.vsplit() end
    -- Now we're in the right-most split.
    for i = #layout, 1, -1 do
      self:dispatch( layout[i] )
      if i ~= 1 then win.wincmd( 'h' ) end
    end
  end,

  hsplit=function( self, layout )
    local num = #layout
    -- Create all the splits.
    for _ = 1, num - 1 do win.split() end
    -- Now we're in the bottom-most split.
    for i = #layout, 1, -1 do
      self:dispatch( layout[i] )
      if i ~= 1 then win.wincmd( 'k' ) end
    end
  end,

  edit=function( _, name )
    assert( type( name ) == 'string' )
    win.edit( name )
  end,

  -- The user can specify a function as a leaf node (instead of a
  -- string) and we will just call it to do whatever needs to be
  -- done.
  func=function( _, f )
    assert( type( f ) == 'function' )
    f()
  end,

  dispatch=function( self, layout )
    if type( layout ) == 'string' then
      self:edit( layout )
    elseif type( layout ) == 'function' then
      self:func( layout )
    else
      assert( type( layout ) == 'table' )
      assert( layout.type )
      self[layout.type]( self, layout )
    end
  end,
}

function M.open( layout )
  -- If there are multiple buffers or if there is only one buffer
  -- but it contains a file open in it, then open a new tab
  -- first. Otherwise, we have only one tab/buffer open and it is
  -- the initial (empty) one, so we don't need to open a new tab.
  if util.total_buffers() > 1 or #vim.fn.bufname( 1 ) > 0 then
    win.tab()
  end
  Layout:dispatch( layout )
end

return M
