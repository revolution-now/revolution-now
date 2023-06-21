-----------------------------------------------------------------
-- Template Substitutionator.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'ide.util' )
local default_handler = require( 'ide.templates.default' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local file_exists = util.file_exists
local format = string.format

local fnamemodify = vim.fn.fnamemodify

local autocmd = vim.api.nvim_create_autocmd
local augroup = vim.api.nvim_create_augroup

local IDE_ROOT = util.ide_root_dir()

-----------------------------------------------------------------
-- Implementation.
-----------------------------------------------------------------
local function tmpl_file_for( s )
  return format( '%s/templates/%s.template', IDE_ROOT, s )
end

local function open_tmpl_and_apply( tmpl_name, file_path, handler )
  local tmpl_path = tmpl_file_for( tmpl_name )
  local f = assert( io.open( tmpl_path, 'r' ) )
  local body = assert( f:read( '*a' ) )
  f:close()
  local ctx = {
    path=file_path,
    author=os.getenv( 'USER' ) or 'unknown',
  }
  handler = handler or function() end
  local function handler_sub( tag )
    -- If the handler returns nil then the gsub will not substi-
    -- tute that instance, which is what we want.
    return handler( tag, ctx ) or default_handler( tag, ctx )
  end
  body = body:gsub( '{{([%w_0-9]+)}}', handler_sub )
  -- Now get rid of any tags that are alone on a new line in a
  -- way that removes the line itself.
  body = body:gsub( '\n *{{[%w_0-9]+}}\n', '\n' )
  -- tag that is on its own line is substituted to nothing.
  -- Now get rid of any double newlines, which can remain if a
  -- tag that is on its own line is substituted to nothing.
  body = body:gsub( '\n[\n]+', '\n\n' )
  return body
end

local function apply_to_buf( tmpl_name, buf, handler )
  local file_path = vim.api.nvim_buf_get_name( buf )
  if file_exists( file_path ) then return end
  if file_path == '' then return end
  local new_body = open_tmpl_and_apply( tmpl_name, file_path,
                                        handler )
  vim.api.nvim_buf_set_lines( buf, 0, -1, false,
                              vim.fn.split( new_body, '\n' ) )
end

local function get_handler_for_tmpl( tmpl_name )
  local success, handler = pcall( require,
                                  'ide.templates.' .. tmpl_name )
  return success and handler or default_handler
end

local function choose_tmpl_for_file( buf )
  local file_path = vim.api.nvim_buf_get_name( buf )
  assert( #file_path > 0 )

  -- Special filename patterns.
  if file_path:match( '-test.cpp' ) then return 'cpptest' end

  if file_path:match( '.*/config/.*%.rds' ) then
    return 'rds-config'
  end

  -- Extension gets next priority, e.g. so that we can distin-
  -- guish hpp from cpp.
  local ext = fnamemodify( file_path, ':e' )
  if ext ~= '' then
    if file_exists( tmpl_file_for( ext ) ) then return ext end
  end

  -- Lastly, fall back to filetype.
  local ft = vim.bo[buf].ft
  if not ft or ft == '' then
    -- Sometimes the filetype has not been set yet when this
    -- method is called (e.g. when called from a BufNewFile
    -- event, since the nvim filetype mechanism (filetype.lua)
    -- uses those same events to set the filetype. Manually in-
    -- voke filetype.lua to get the ft.
    ft = vim.filetype.match{ filename=file_path, buf=buf }
  end
  if ft and ft ~= '' then
    if file_exists( tmpl_file_for( ft ) ) then return ft end
  end

  -- No luck.
  return nil
end

-- `name` would be e.g. "cpptest". The ctx file is needed some
-- handlers will behave differently depending on the file name or
-- path into which the template will eventually be inserted.
function M.load( tmpl_name, ctx_file )
  assert( ctx_file )
  local handler = assert( get_handler_for_tmpl( tmpl_name ) )
  return open_tmpl_and_apply( tmpl_name, ctx_file, handler )
end

local function on_new_file( ev )
  assert( ev )
  assert( ev.buf )
  local tmpl_name = choose_tmpl_for_file( ev.buf )
  if not tmpl_name then return end
  local handler = assert( get_handler_for_tmpl( tmpl_name ) )
  -- print( format( 'applying template "%s"...', tmpl_name ) )
  apply_to_buf( tmpl_name, ev.buf, handler )
end

-----------------------------------------------------------------
-- Autocommands.
-----------------------------------------------------------------
function M.enable()
  autocmd( 'BufNewFile', {
    group=augroup( 'IdeTemplates', { clear=true } ),
    callback=on_new_file,
  } )
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
