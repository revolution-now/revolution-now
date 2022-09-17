-----------------------------------------------------------------
-- Library for generating a split layout within a tab.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local win = require( 'ide.win' )

-----------------------------------------------------------------
-- Layout
-----------------------------------------------------------------
local Layout = {
  vsplit=function( self, layout )
    local num = #layout
    -- Create all the splits.
    for i = 1, num - 1 do win.vsplit() end
    -- Now we're in the right-most split.
    for i = #layout, 1, -1 do
      self:dispatch( layout[i] )
      if i ~= 1 then win.wincmd( 'h' ) end
    end
  end,

  hsplit=function( self, layout )
    local num = #layout
    -- Create all the splits.
    for i = 1, num - 1 do win.split() end
    -- Now we're in the bottom-most split.
    for i = #layout, 1, -1 do
      self:dispatch( layout[i] )
      if i ~= 1 then win.wincmd( 'k' ) end
    end
  end,

  edit=function( self, name )
    assert( type( name ) == 'string' )
    win.edit( name )
  end,

  -- The user can specify a function as a leaf node (instead of a
  -- string) and we will just call it to do whatever needs to be
  -- done.
  func=function( self, f )
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
      assert( type( layout.what ) == 'table' )
      self[layout.type]( self, layout.what )
    end
  end
}

function M.open( layout )
  -- FIXME: find out how to detect when we don't need to create a
  -- new tab, i.e. when we are opening the first one.
  win.tab()
  Layout:dispatch( layout )
end

return M
