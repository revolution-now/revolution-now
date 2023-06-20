-----------------------------------------------------------------
-- Template tag handlers for the hpp template.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local default = require( 'ide.templates.default' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format

local file_exists = util.file_exists

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local RN_ROOT = util.rn_root_dir()

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function config_header_for_module( module )
  local library = module:match( '^([^/]+)/' )
  if not library then return 'core-config.hpp' end
  local configs = {
    base='config.hpp', --
  }
  return configs[library] or nil
end

-----------------------------------------------------------------
-- Handlers.
-----------------------------------------------------------------
local handlers = {
  INCLUDE_CONFIG=function( _, ctx )
    local module = default( 'MODULE', ctx )
    local config = config_header_for_module( module )
    if not config then return nil end
    return format( '#include "%s"', config )
  end,
  INCLUDE_RDS=function( _, ctx )
    local module = default( 'MODULE', ctx )
    local rds = format( '%s/src/%s.rds', RN_ROOT, module )
    if not file_exists( rds ) then return end
    local file_stem = default( 'FILE_STEM', ctx )
    return format( '#include "%s.rds.hpp"', file_stem )
  end,
}

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return function( tag, ctx )
  return handlers[tag] and handlers[tag]( handlers, ctx )
end
