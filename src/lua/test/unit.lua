--[[ ------------------------------------------------------------
|
| unit.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-13.
|
| Description: Lua Unit Testing Framework.
|
--]] ------------------------------------------------------------
local M = {}

local NORMAL = string.char( 27 ) .. '[00m'
local GREEN = string.char( 27 ) .. '[32m'
local RED = string.char( 27 ) .. '[31m'
local YELLOW = string.char( 27 ) .. '[93m'
local BOLD = string.char( 27 ) .. '[1m'
local UNDER = string.char( 27 ) .. '[4m'

function M.ASSERT_EQ( l, r, desc )
  if l == r then return end
  error( desc .. ' are not equal: ' .. tostring( l ) .. ' != ' ..
             tostring( r ) )
end

function M.ASSERT_LE( l, r, desc )
  if l <= r then return end
  error( desc .. ' are not less-or-equal: ' .. tostring( l ) ..
             ' > ' .. tostring( r ) )
end

function M.new_test_pack()
  -- Disallow duplicate test names to catch mistakes.
  local store = {}
  return setmetatable( {}, {
    __index=function( _, k ) return store[k] end,
    __newindex=function( _, k, v )
      assert( not store[k], 'duplicate test name: ' .. k )
      store[k] = v
    end,
    __pairs=function() return pairs( store ) end
  } )
end

function M.runner( quiet, pack )
  local sorted_names = {}
  local longest_length = 0
  for name in pairs( pack ) do
    if #name > longest_length then longest_length = #name end
    table.insert( sorted_names, name )
  end
  table.sort( sorted_names )
  for _, name in ipairs( sorted_names ) do
    if not quiet then
      io.write( 'running test ' .. name .. '...' )
      io.flush()
      for i = 1, longest_length - #name do io.write( ' ' ) end
    end
    pack[name]()
    if not quiet then
      io.write( GREEN .. ' passed' .. NORMAL .. '.\n' )
    end
  end
end

return M
