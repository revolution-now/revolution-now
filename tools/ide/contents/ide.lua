-- Describes what opens when we edit the IDE scripts themselves.
local LU = require( 'ide.layout-util' )
local vsplit, hsplit = LU.vsplit, LU.hsplit
return

-----------------------------------------------------------------
-- Layout
-----------------------------------------------------------------
vsplit {
  hsplit {
    'tools/ide/contents/ide.lua',
    'tools/ide/contents/rn.lua',
    'tools/ide/util.lua',
  },
  'tools/ide/edit-rn.lua',
  'tools/ide/layout.lua',
  hsplit {
    'tools/ide/module-cpp.lua',
    'tools/ide/win.lua',
  }
}