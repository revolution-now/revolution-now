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

local function foreach( list, f )
  for _, e in ipairs( list ) do
    f( e )
  end
end

local function ls( table )
  for k, v in pairs( table ) do
    log.debug( tostring( k ) .. " = " .. tostring( v ) )
  end
end

package_exports = {
  foreach    = foreach,
  ls         = ls,
}
