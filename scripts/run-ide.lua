-----------------------------------------------------------------
-- Creates the editor session.
-----------------------------------------------------------------
package.path = package.path --
.. ';/usr/share/lua/5.1/?/init.lua' --
.. ';/usr/share/lua/5.1/?.lua' --
.. ';./scripts/?.lua' --
.. ';./scripts/?/init.lua' --

package.cpath = package.cpath --
.. ';/usr/lib/x86_64-linux-gnu/lua/5.1/?.so'

local ide = require( 'ide' )

ide.build.main()
