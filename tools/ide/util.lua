-----------------------------------------------------------------
-- Generic utilities.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local posix = require( 'posix' )

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local call = vim.call

local fnamemodify = vim.fn.fnamemodify
local resolve = vim.fn.resolve
local expand = vim.fn.expand
local getbufinfo = vim.fn.getbufinfo

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
function M.input( prompt )
  call( 'inputsave' )
  prompt = prompt or '> '
  local res = vim.call( 'input', prompt )
  call( 'inputrestore' )
  return res
end

function M.map_values( func, m )
  local res = {}
  for k, v in pairs( m ) do res[k] = func( v ) end
  return res
end

function M.map( func, lst )
  local res = {}
  for i, e in ipairs( lst ) do res[i] = func( e ) end
  return res
end

-- func is a function that takes a single string argument. This
-- will return a function that allows calling func with a format
-- string.
function M.formattable( func )
  return function( fmt, ... )
    if fmt == nil then return func() end
    local cmd = format( fmt, ... )
    return func( cmd )
  end
end

-- Returns the file name if it exists, otherwise nil.
M.exists = M.formattable( function( file )
  if posix.stat( file ) == nil then return nil end
  return file
end )

-- Returns a function that will call the named vim command with a
-- string-formatted argument, which will be formatted into a
-- single string and given as the first argument to the command.
function M.fmt_func_with_arg( vim_name )
  return M.formattable( function( formatted )
    if formatted == nil then
      vim.cmd( 'silent ' .. vim_name )
    else
      vim.cmd( 'silent ' .. vim_name .. ' ' .. formatted )
    end
  end )
end

-- Determines if the monitor is considered wide or narrow. "wide"
-- is defined as being able to display at least four vertical
-- splits.
function M.is_wide()
  local columns = vim.o.columns
  local text_columns_per_split = 65
  local desired_columns_per_split = text_columns_per_split + 1
  if columns >= 4 * desired_columns_per_split then return true end
  return false
end

-- See here:
--   https://superuser.com/questions/345520/vim-number-of-total-buffers
function M.total_buffers() return #getbufinfo{ buflisted=1 } end

function M.rn_root_dir()
  return fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )
end

return M
