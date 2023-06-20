-----------------------------------------------------------------
-- Template tag handlers for the rds-config template.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local default = require( 'ide.templates.default' )

-----------------------------------------------------------------
-- Handlers.
-----------------------------------------------------------------
local handlers = {
  FILE_STEM_IDENTIFIER=function( _, ctx )
    local file_stem = default( 'FILE_STEM', ctx )
    return file_stem:gsub( '-', '_' )
  end,
}

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return function( tag, ctx )
  return handlers[tag] and handlers[tag]( handlers, ctx )
end
