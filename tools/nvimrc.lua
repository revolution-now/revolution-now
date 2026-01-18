-----------------------------------------------------------------
-- Non-ide lua setup for revolution-now.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local vim = _G['vim']
local format = string.format

local o = vim.o

local fnamemodify = vim.fn.fnamemodify
local resolve = vim.fn.resolve

local autocmd = vim.api.nvim_create_autocmd
local augroup = vim.api.nvim_create_augroup

-----------------------------------------------------------------
-- Compute rn root directory.
-----------------------------------------------------------------
local this_file = require( 'debug' ).getinfo( 1 ).short_src
this_file = resolve( fnamemodify( this_file, ':p' ) )

local RN_ROOT = fnamemodify( this_file, ':h:h' )

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
-- Extra syntax files.
-----------------------------------------------------------------
-- This is so that the Rds syntax file gets picked up.
o.runtimepath = o.runtimepath .. ',' .. RN_ROOT .. '/src/rds' ..
                    ',' .. RN_ROOT .. '/src/rcl'

-----------------------------------------------------------------
-- Filetypes.
-----------------------------------------------------------------
vim.filetype.add{
  extension={ rds='rds', rcl='rcl' },
  pattern={ ['.*/doc/.*design.txt']='markdown' },
}

-----------------------------------------------------------------
-- Templates.
-----------------------------------------------------------------
template.enable()

-----------------------------------------------------------------
-- Auto-format on save.
-----------------------------------------------------------------
-- Controls format on save.
local FORMAT_ON_SAVE = false

fmt.enable_autoformat_on_save( function( path )
  if not FORMAT_ON_SAVE then return end
  -- We should auto-format-on-save but only depending on the file
  -- path. E.g., we don't want to auto-format files in extern.
  local is_src = path:match( 'revolution.now/src' )
  local is_exe = path:match( 'revolution.now/exe' )
  local is_test = path:match( 'revolution.now/test' )
  local is_tools = path:match( 'revolution.now/tools' )
  return is_src or is_exe or is_test or is_tools
end )

-----------------------------------------------------------------
-- Experimental.
-----------------------------------------------------------------
-- Our version of Lua may have a `continue` keyword which normal
-- Lua does not. This may not have any effect though if the oper-
-- ative syntax highlighter considers it a syntax error and
-- changes the color of it.
vim.cmd[[syntax keyword luaStatement continue]]
