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
