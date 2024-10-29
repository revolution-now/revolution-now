-----------------------------------------------------------------
-- Generic utilities.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local format = string.format
local call = vim.call

local fnamemodify = vim.fn.fnamemodify
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
    if #{ ... } == 0 then return func( fmt ) end
    return func( format( fmt, ... ) )
  end
end

-- Returns the file name if it exists, otherwise nil.
M.file_exists = M.formattable( function( file )
  return vim.fn.filereadable( file ) == 1
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
  -- Add 4 for dividers and number column line.
  local text_columns_per_split = 65 + 4
  local desired_columns_per_split = text_columns_per_split + 1
  if columns >= 4 * desired_columns_per_split then return true end
  return false
end

-- See here:
--   https://superuser.com/questions/345520/vim-number-of-total-buffers
function M.total_buffers() return #getbufinfo{ buflisted=1 } end

local this_script = assert( fnamemodify(
                                require( 'debug' ).getinfo( 1 )
                                    .short_src, ':p' ) )

function M.rn_root_dir()
  return assert(
             this_script:match( '^(.*/revolution.now[^/]*)/' ) )
end

function M.ide_root_dir()
  return assert( this_script:match( '^(.*/ide)/' ) )
end

local function make_command_string( prog, ... )
  assert( prog )
  local c = format( '"%s"', prog )
  assert( not prog:match( ' ' ),
          'program name should not have spaces in it.' )
  for _, arg in ipairs{ ... } do
    arg = arg:gsub( '"', [[\"]] )
    c = format( '%s "%s"', c, arg )
  end
  return c
end

-- Runs a shell command in a subprocess, waits for it to end,
-- then returns the entirety of stdout as a string. If the pro-
-- gram ends with a non-zero error code then an error will be
-- thrown with information about the error. Note that stderr of
-- the subprocess is not captured, so it will just go to stderr
-- of the calling process. If you want to send stderr to stdout
-- it should work to include 2>&1 on the command line. It is ex-
-- pected that the `sh` shell is used.
function M.shell_command( prog, ... )
  local c = make_command_string( prog, ... )
  local file = assert( io.popen( c ) )
  file:flush()
  local stdout = file:read( '*all' )
  local success, termination = file:close()
  if not success then
    error( format( 'system command failed.\n' .. --
                       '  success:     %s\n' .. --
                       '  termination: %s\n' .. --
                       '  command:     %s', --
    success, termination, c ) )
  end
  return stdout
end

return M
