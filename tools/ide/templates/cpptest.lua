-----------------------------------------------------------------
-- Template tag handlers for the cpptest template.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local default = require( 'ide.templates.default' )
local util = require( 'ide.util' )
local template = require( 'ide.template' )

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
-- Helpers.
-----------------------------------------------------------------
local function need_fake_world( module )
  return not module:match( '/' )
end

-----------------------------------------------------------------
-- Handlers.
-----------------------------------------------------------------
local handlers = {
  INCLUDE_MODULE_HEADER=function( _, ctx )
    local module = default( 'MODULE', ctx )
    local file_path = format( '%s/src/%s.hpp', RN_ROOT, module )
    local file_incl = format( 'src/%s.hpp', module )
    local incl = format( '#include "%s"', file_incl )
    if not file_exists( file_path ) then incl = '// ' .. incl end
    return incl
  end,
  INCLUDE_FAKE_WORLD=function( _, ctx )
    local module = default( 'MODULE', ctx )
    if not need_fake_world( module ) then return nil end
    return '// Testing.\n#include "test/fake/world.hpp"'
  end,
  FAKE_WORLD_SETUP=function( _, ctx )
    local module = default( 'MODULE', ctx )
    if not need_fake_world( module ) then return nil end
    -- This will load the template and substitute any tags from
    -- the fake-world.lua file, if present.
    return template.load( 'fake-world', ctx.path )
  end,
  WORLD_INST=function( _, ctx )
    local module = default( 'MODULE', ctx )
    if not need_fake_world( module ) then return nil end
    return 'World W;'
  end,
}

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return function( tag, ctx )
  return handlers[tag] and handlers[tag]( handlers, ctx )
end
