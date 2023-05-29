-----------------------------------------------------------------
-- Non-ide lua setup for revolution-now.
-----------------------------------------------------------------
-- These are things that we want to have in place regardless of
-- whether we're running the full ide scripts or not.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local format = require( 'dsicilia.format' )

-----------------------------------------------------------------
-- Auto-format on save.
-----------------------------------------------------------------
-- Return whether we should auto-format-on-save depending on the
-- file path. E.g., we don't want to auto-format files in extern.
format.enable_autoformat_on_save(
    function( path )
      local is_src = path:match( 'revolution.now/src' )
      local is_exe = path:match( 'revolution.now/exe' )
      local is_test = path:match( 'revolution.now/test' )
      local is_tools = path:match( 'revolution.now/tools' )
      return is_src or is_exe or is_test or is_tools
    end )
