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

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
function M.input( prompt )
  prompt = prompt or '> '
  return vim.call( 'input', prompt )
end

function M.map_values( func, m )
  local res = {}
  for k, v in pairs( m ) do res[k] = func( v ) end
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
function M.fmt_func_with_file_arg( vim_name )
  return M.formattable( function( file )
    if file == nil then
      vim.cmd( 'silent ' .. vim_name )
    else
      vim.cmd( 'silent ' .. vim_name .. ' ' .. file )
    end
  end )
end

return M
