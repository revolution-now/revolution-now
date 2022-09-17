-----------------------------------------------------------------
-- Vim window/tab/split command wrappers.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Functions.
-----------------------------------------------------------------
M.source = util.fmt_func_with_file_arg( 'source' )
M.edit = util.fmt_func_with_file_arg( 'edit' )
M.tab = util.fmt_func_with_file_arg( 'tabnew' )
M.vsplit = util.fmt_func_with_file_arg( 'vsplit' )
M.split = util.fmt_func_with_file_arg( 'split' )
M.vnew = util.fmt_func_with_file_arg( 'vnew' )

-- :wincmd <what>
function M.wincmd( what ) vim.cmd( 'wincmd ' .. what ) end

return M
