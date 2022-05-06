--[[ ------------------------------------------------------------
|
| util.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-19.
|
| Description: Utilities.
|
--]] ------------------------------------------------------------
local M = {}

-- FIXME: global
function Coord( arg )
  return setmetatable( { x=arg.x, y=arg.y }, {
    __tostring=function( self )
      return 'Coord{x=' .. tostring( self.x ) .. ',y=' ..
                 tostring( self.y ) .. '}'
    end,
    __eq=function( lhs, rhs )
      return lhs.x == rhs.x and lhs.y == rhs.y
    end
  } )
end

-- FIXME: global
function Delta( arg )
  return setmetatable( { w=arg.w, h=arg.h }, {
    __tostring=function( self )
      return 'Delta{w=' .. tostring( self.w ) .. ',h=' ..
                 tostring( self.h ) .. '}'
    end,
    __eq=function( lhs, rhs )
      return lhs.w == rhs.w and lhs.h == rhs.h
    end
  } )
end

-- FIXME: global
function assert_eq( lhs, rhs )
  if lhs ~= rhs then
    error( 'assertion failed: ' .. tostring( lhs ) .. ' != ' ..
               tostring( rhs ) .. '.' )
  end
end

function M.starts_with( str, start )
  return str:sub( 1, #start ) == start
end

function M.foreach( list, f )
  for _, e in ipairs( list ) do f( e ) end
end

function M.print_passthrough( arg )
  print( tostring( arg ) )
  return arg
end

function M.ls( table )
  if type( table ) == 'userdata' then
    -- For userdata. Unfortunately the types of the members all
    -- show as functions.
    table = getmetatable( table ) or {}
  end
  for k, v in pairs( table ) do
    if not M.starts_with( tostring( k ), '__' ) then
      log.console( tostring( k ) .. ': ' .. tostring( v ) )
    end
  end
end

return M
