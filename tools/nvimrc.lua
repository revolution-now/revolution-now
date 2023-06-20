-----------------------------------------------------------------
-- Non-ide lua setup for revolution-now.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local vim = _G['vim']
local fnamemodify = vim.fn.fnamemodify
local format = string.format

-----------------------------------------------------------------
-- Compute rn root directory.
-----------------------------------------------------------------
-- These are things that we want to have in place regardless of
-- whether we're running the full ide scripts or not.
local this_script = fnamemodify(
                        require( 'debug' ).getinfo( 1 ).short_src,
                        ':p' )
local RN_ROOT = this_script:match( '^(.*/revolution.now[^/]*)/' )

-----------------------------------------------------------------
-- Make sure that we can import the IDE.
-----------------------------------------------------------------
package.path = package.path --
.. format( ';%s/tools/?.lua', RN_ROOT ) --
.. format( ';%s/tools/?/init.lua', RN_ROOT ) --

-- This will allow us to import e.g. the posix library.
--[[
package.path = package.path --
.. ';/usr/share/lua/5.1/?/init.lua' --
.. ';/usr/share/lua/5.1/?.lua' --

package.cpath = package.cpath --
.. ';/usr/lib/x86_64-linux-gnu/lua/5.1/?.so'
]] --

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local fmt = require( 'dsicilia.format' )
local template = require( 'ide.template' )

-----------------------------------------------------------------
-- Templates.
-----------------------------------------------------------------
template.enable()

-----------------------------------------------------------------
-- Auto-format on save.
-----------------------------------------------------------------
-- Return whether we should auto-format-on-save depending on the
-- file path. E.g., we don't want to auto-format files in extern.
fmt.enable_autoformat_on_save( function( path )
  local is_src = path:match( 'revolution.now/src' )
  local is_exe = path:match( 'revolution.now/exe' )
  local is_test = path:match( 'revolution.now/test' )
  local is_tools = path:match( 'revolution.now/tools' )
  return is_src or is_exe or is_test or is_tools
end )
