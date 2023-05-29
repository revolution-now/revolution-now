-----------------------------------------------------------------
-- Creates the editor session.
-----------------------------------------------------------------
package.path = package.path --
.. ';/usr/share/lua/5.1/?/init.lua' --
.. ';/usr/share/lua/5.1/?.lua' --
.. ';./tools/?.lua' --
.. ';./tools/?/init.lua' --

package.cpath = package.cpath --
.. ';/usr/lib/x86_64-linux-gnu/lua/5.1/?.so'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format

local mode = ...
if not mode then mode = 'rn' end

require( format( 'ide.edit-%s', mode ) )
