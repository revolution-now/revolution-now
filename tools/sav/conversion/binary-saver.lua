--[[ ------------------------------------------------------------
|
| binary-saver.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-11-04.
|
| Description: Writes binary SAV files.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local structure_parser = require( 'structure-parser' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local char = string.char
local StructureParser = structure_parser.StructureParser
local insert = table.insert

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function parse_hex_digit( digit )
  assert( type( digit ) == 'string' )
  digit = digit:lower()
  if digit:match( '[a-f]' ) then
    return string.byte( digit ) - string.byte( 'a' ) + 10
  elseif digit:match( '[0-9]' ) then
    return string.byte( digit ) - string.byte( '0' )
  else
    error( 'unexpected hex digit: ' .. digit )
  end
end

local function parse_hex_byte( b )
  assert( type( b ) == 'string' )
  assert( #b == 2 )
  b = b:lower()
  local dh, dl = b:match( '([0-9a-f])([0-9a-f])' )
  assert( dh )
  assert( dl )
  dh = assert( parse_hex_digit( dh ) )
  dl = assert( parse_hex_digit( dl ) )
  return dh * 16 + dl
end

local function parse_hex_bytes( bs )
  assert( type( bs ) == 'string' )
  bs = bs:lower()
  local res = {}
  bs:gsub( '([0-9a-f][0-9a-f])', function( byte_str )
    insert( res, parse_hex_byte( byte_str ) )
  end )
  return res
end

local function parse_binary_digits( bits )
  assert( type( bits ) == 'string' )
  local res = 0
  bits:gsub( '[01]', function( bit )
    res = res << 1
    res = res + tonumber( bit )
  end )
  return res
end

-----------------------------------------------------------------
-- BinarySaver.
-----------------------------------------------------------------
-- Structure Handler. Implements an interface to handler re-
-- sponding to the various components of the structure document.
local BinarySaver = {}

M.BinarySaver = BinarySaver

function BinarySaver:_byte( b )
  assert( not self.byte_ )
  assert( type( b ) == 'number' )
  assert( self.sav_:write( string.char( b ) ) == self.sav_ )
end

function BinarySaver:_bit( b )
  if not self.byte_ then self.byte_ = { byte=0, nbits=0 } end
  local byte = self.byte_
  assert( byte )
  assert( byte.nbits < 8 )
  byte.byte = byte.byte + (b << byte.nbits)
  byte.nbits = byte.nbits + 1
  if byte.nbits == 8 then
    local to_write = byte.byte
    self.byte_ = false
    self:_byte( to_write )
  end
end

function BinarySaver:bit_field( tbl )
  self:dbg( 'emitting bit field...' )
  if not tbl.type then
    local new_tbl = {}
    for k, v in pairs( tbl ) do new_tbl[k] = v end
    new_tbl.type = 'uint'
    return self:bit_field( new_tbl )
  end
  local leaf = self:_find_leaf()
  if self.metadata_[tbl.type] then
    local meta = self.metadata_[tbl.type]
    self:dbg( '  type: %s', tbl.type )
    assert( tbl.type:match( '[0-9]bit_' ) )
    assert( type( leaf ) == 'string' )
    assert( meta[leaf] )
    local bits_str = meta[leaf]
    self:dbg( '  bits_str: %s', bits_str )
    local byte = parse_binary_digits( bits_str )
    assert( type( byte ) == 'number',
            format( 'type of byte: %s', type( byte ) ) )
    local res = byte
    for _ = 1, tbl.size do
      self:_bit( byte & 1 )
      byte = byte >> 1
    end
    return res
  elseif tbl.type == 'bit_bool' then
    assert( type( leaf ) == 'boolean' )
    if leaf then
      self:_bit( 1 )
      return 1
    else
      self:_bit( 0 )
      return 0
    end
  elseif tbl.type == 'uint' then
    local number = leaf
    if type( leaf ) == 'string' then
      local byte = parse_binary_digits( leaf )
      number = byte
    else
      assert( type( leaf ) == 'number' )
    end
    local res = number
    for _ = 1, tbl.size do
      self:_bit( number & 1 )
      number = number >> 1
    end
    return res
  else
    error( 'unhandled bit field type: ' .. tbl.type )
  end
end

function BinarySaver:_uint( leaf, size )
  self:dbg( 'writing uint...' )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  local bytes = {}
  if type( leaf ) == 'string' then
    assert( #leaf == 2 * size + size - 1 )
    bytes = parse_hex_bytes( leaf )
  else
    assert( type( leaf ) == 'number' )
    bytes = {}
    for _ = 1, size do
      insert( bytes, leaf % 256 )
      leaf = leaf // 256
    end
  end
  local res = 0
  for _, b in ipairs( bytes ) do self:dbg( '  b: 0x%x', b ) end
  for i = 1, #bytes do
    local byte = bytes[i]
    res = res + (byte << ((i - 1) * 8))
    self:_byte( byte )
  end
  return res
end

function BinarySaver:_bits( leaf, byte_count )
  self:dbg( 'writing bits' )
  assert( byte_count )
  assert( type( byte_count ) == 'number' )
  assert( byte_count > 0 )
  assert( type( leaf ) == 'string' )
  assert( #leaf == byte_count * 8 )
  local number = parse_binary_digits( leaf )
  self:trace( '  bits number: %x', number )
  local res = number
  for _ = 1, byte_count do
    local byte = number % 256
    number = number // 256
    self:_byte( byte )
  end
  return res
end

function BinarySaver:_int( leaf, size )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  assert( type( leaf ) == 'number' )
  if leaf >= 0 then return self:_uint( leaf, size ) end
  local ffs = 2 ^ (size * 8) - 1
  ffs = ffs + leaf + 1
  return self:_uint( ffs, size )
end

function BinarySaver:_string( leaf, size )
  self:dbg( 'emitting string' )
  assert( size )
  assert( type( size ) == 'number' )
  assert( leaf )
  assert( type( leaf ) == 'string' )
  assert( #leaf <= size )
  self:dbg( 'string: #leaf=%d, size=%d', #leaf, size )
  for i = 1, size do
    local byte = 0
    if i <= #leaf then
      local c = assert( leaf:sub( i, i ) )
      byte = string.byte( c )
    end
    self:dbg( '  writing byte: %s', char( byte ) )
    self:_byte( byte )
  end
  return leaf
end

function BinarySaver:_find_leaf()
  local bt = self.backtrace_
  self:dbg( 'finding leaf for "%s"...', bt )
  local res = assert( self.json_ )
  for _, key in ipairs( bt ) do
    self:dbg( '  key: %s', key )
    res = res[key]
    assert( res ~= nil, format( 'key %s not found', key ) )
  end
  return res
end

function BinarySaver:_primitive_single( leaf, tbl )
  if tbl.type == 'str' then
    return self:_string( leaf, tbl.size )
  elseif tbl.type == 'bits' then
    local byte_count = assert( tbl.size )
    return self:_bits( leaf, byte_count )
  elseif tbl.type == 'uint' then
    return self:_uint( leaf, tbl.size )
  elseif tbl.type == 'int' then
    return self:_int( leaf, tbl.size )
  else
    -- Unknown.
    if type( tbl.type ) == 'string' then
      assert( tbl.type )
      assert( type( leaf ) == 'string' )
      local meta = assert( self.metadata_[tbl.type] )
      local digits = assert( meta[leaf] )
      local bytes
      if leaf:match( '[0-9]bit_' ) then
        bytes = parse_binary_digits( digits )
      else
        bytes = parse_hex_bytes( digits )
      end
      assert( type( bytes ) == 'table' )
      local res = 0
      for _, byte in ipairs( bytes ) do
        res = res << 8
        res = res + byte
        self:_byte( byte )
      end
      return res
    else
      assert( not tbl.type )
      return self:_primitive_single( leaf, {
        size=tbl.size,
        type='uint',
      } )
    end
  end
end

function BinarySaver:primitive( tbl )
  assert( type( tbl.size ) == 'number' )
  local leaf = self:_find_leaf()
  return self:_primitive_single( leaf, tbl )
end

function BinarySaver:primitive_array( cells, tbl )
  local leaf = self:_find_leaf()
  if type( leaf ) == 'string' then
    leaf = parse_hex_bytes( leaf )
  end
  assert( type( leaf ) == 'table' )
  assert( #leaf == cells )
  self:dbg( 'cells: %d', cells )
  for _, elem in ipairs( leaf ) do
    self:_primitive_single( elem,
                            { size=tbl.size, type=tbl.type } )
  end
  return leaf
end

function BinarySaver.new( metadata, json, sav )
  -- This simulates inheritance.
  local base = StructureParser( metadata )
  local obj = {}
  local BinarySaverMeta = setmetatable( {}, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=function( _, key )
      if BinarySaver[key] then return BinarySaver[key] end
      return base[key]
    end,
    __metatable=false,
  } )
  obj.base_ = base
  obj.json_ = json
  obj.sav_ = sav
  obj.byte_ = false
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=BinarySaverMeta,
    __metatable=false,
  } )
  return obj
end

setmetatable( BinarySaver, {
  __call=function( _, ... ) return BinarySaver.new( ... ) end,
} )

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
