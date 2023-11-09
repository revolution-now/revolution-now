--[[ ------------------------------------------------------------
|
| json-transcode.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: Reads and writes JSON.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local lunajson = require( 'lunajson' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
-- local format = string.format
local yield = coroutine.yield

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local JNULL = {}

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
-- Decode a string of json.
function M.decode( json_string )
  -- Start decoding from the start of the string.
  local pos = 0
  -- `null` inside the json will be decoded as this sentinel.
  local nullv = JNULL
  -- Store the length of an array in array[0]. This can be used
  -- to distinguish empty arrays from empty objects.
  local arraylen = true
  local tbl =
      lunajson.decode( json_string, pos, nullv, arraylen )
  assert( tbl )
  return tbl
end

-- Coroutine generator function that pretty-prints a layout in
-- conforming JSON while preserving key ordering. Each time a
-- line is produced it will yield it.
local function pprint_ordered_impl( append, o, prefix, spaces )
  spaces = spaces or ''
  assert( o ~= JNULL )
  if type( o ) == 'table' and not o[0] then
    -- Object.
    if #spaces == 0 then append( prefix ) end
    append( '{' )
    append()
    spaces = spaces .. '  '
    local keys = {}
    if o.__key_order then
      for _, k in ipairs( o.__key_order ) do
        assert( type( k ) == 'string' )
        if not k:match( '^_' ) then
          table.insert( keys, k )
        end
      end
    else
      for k, _ in pairs( o ) do
        assert( type( k ) == 'string' )
        if not k:match( '^_' ) then
          table.insert( keys, k )
        end
      end
      table.sort( keys )
    end
    for i, k in ipairs( keys ) do
      assert( o[k] ~= nil )
      local v = o[k]
      k = '"' .. k .. '"'
      append( prefix .. spaces .. k .. ': ' )
      pprint_ordered_impl( append, v, prefix, spaces )
      if i ~= #keys then append( ',' ) end
      append()
    end
    append( prefix .. string.sub( spaces, 3 ) .. '}' )
  elseif type( o ) == 'table' and o[0] then
    -- Array.
    if #spaces == 0 then append( prefix ) end
    append( '[' )
    if #o == 0 then
      append( ']' )
      return
    end
    append()
    spaces = spaces .. '  '
    for i, e in ipairs( o ) do
      append( prefix .. spaces )
      pprint_ordered_impl( append, e, prefix, spaces )
      if i ~= #o then append( ',' ) end
      append()
    end
    append( prefix .. string.sub( spaces, 3 ) .. ']' )
  elseif type( o ) == 'string' then
    local s = ''
    if #spaces == 0 then s = prefix end
    append( s .. '"' .. tostring( o ) .. '"' )
  else
    local s = ''
    if #spaces == 0 then s = prefix end
    append( s .. tostring( o ) )
  end
end

local function appender()
  local res = ''
  return function( segment )
    if not segment then
      -- flush line.
      yield( res )
      res = ''
    else
      res = res .. segment
    end
  end
end

-- Returns a coroutine generator that pretty-prints a layout in
-- conforming JSON while preserving key ordering. Each time a
-- line is produced it will yield it.
function M.pprint_ordered( o )
  local append = appender()
  local printer = coroutine.wrap( function()
    pprint_ordered_impl( append, o, '', '' )
    append() -- emit final brace.
  end )
  return printer
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
