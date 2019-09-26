--[[-------------------------------------------------------------
|
| util.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2019-09-19.
|
| Description: Utilities.
|
--]]-------------------------------------------------------------

function Coord( arg )
  return {x=arg.x, y=arg.y}
end

local function starts_with( str, start )
   return str:sub( 1, #start ) == start
end

local function foreach( list, f )
  for _, e in ipairs( list ) do
    f( e )
  end
end

local function print_passthrough( arg )
  print( tostring( arg ) )
  return arg
end

local function ls( table )
  if type( table ) == "userdata" then
    -- For sol2 userdata. Unfortunately the types of the members
    -- all show as functions.
    table = getmetatable( table ) or {}
  end
  for k, v in pairs( table ) do
    if not starts_with( tostring( k ), '__' ) then
      log.console( tostring( k ) .. ": " .. tostring( v ) )
    end
  end
end

package_exports = {
  foreach           = foreach,
  ls                = ls,
  starts_with       = starts_with,
  print_passthrough = print_passthrough,
}
