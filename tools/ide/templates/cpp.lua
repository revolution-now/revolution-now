-----------------------------------------------------------------
-- Template tag handlers for the cpp template.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local default = require( 'ide.templates.default' )
local util = require( 'ide.util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local file_exists = util.file_exists

local format = string.format

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local RN_ROOT = util.rn_root_dir()

-----------------------------------------------------------------
-- Handlers.
-----------------------------------------------------------------
local handlers = {
  INCLUDE_MODULE_HEADER=function( _, ctx )
    local module = default( 'MODULE', ctx )
    local file_path = format( '%s/src/%s.hpp', RN_ROOT, module )
    local file_stem = default( 'FILE_STEM', ctx )
    local incl = format( '#include "%s.hpp"', file_stem )
    if not file_exists( file_path ) then incl = '// ' .. incl end
    return incl
  end,
  USING_NAMESPACE_STD=function( _, ctx )
    local module = default( 'MODULE', ctx )
    local header = format( '%s/src/%s.hpp', RN_ROOT, module )
    local using = 'using namespace std;'
    if not file_exists( header ) then using = '// ' .. using end
    return using
  end,
}

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return function( tag, ctx )
  return handlers[tag] and handlers[tag]( handlers, ctx )
end
