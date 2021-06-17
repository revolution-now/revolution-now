--[[ ------------------------------------------------------------
|
| header-dependency-gen.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-06-17.
|
| Description: Scans headers for dependencies in the same folder.
|
--]] ------------------------------------------------------------
re = require( 're' )

start = arg[1] or
            error( 'need starting file as first argument.' )

pattern = re.compile( [=['#include "'{[a-z.-]+}'"']=] )

function extract_file( subject )
  return (re.match( subject, pattern ))
end

re.match( '#include "xyz.hpp"', pattern )

to_search = {}
to_search[start] = false

function num_to_search()
  local n = 0
  for file, done in pairs( to_search ) do
    if done == false then
      n = n + 1
    end
  end
  return n
end

-- digraph graphname {
--     a -> b -> c;
--     b -> d;
-- }

print( 'digraph dependencies {' )

while num_to_search() > 0 do
  for file, done in pairs( to_search ) do
    if done then goto continue end
    -- print( '=== scanning', file, '===' )
    for line in io.lines( file ) do
      local header = extract_file( line )
      if header ~= nil then
        print( '  ' .. file .. ' -> ' .. header .. ';' )
        if to_search[header] == nil then
          to_search[header] = false
        end
      end
    end
    to_search[file] = true
    ::continue::
  end
end

print( '}' )