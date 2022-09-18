-- Describes what opens when we edit the IDE scripts themselves.
local LU = require( 'ide.layout-util' )
local vsplit, hsplit = LU.vsplit, LU.hsplit
return

-----------------------------------------------------------------
-- Layout
-----------------------------------------------------------------
vsplit {
  hsplit {
    'scripts/ide/contents.lua',
    'scripts/ide/util.lua',
  },
  'scripts/ide/edit-rn.lua',
  'scripts/ide/layout.lua',
  hsplit {
    'scripts/ide/module-cpp.lua',
    'scripts/ide/win.lua',
  }
}