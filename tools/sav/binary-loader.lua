--[[ ------------------------------------------------------------
|
| binary-loader.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-28.
|
| Description: TODO [FILL ME IN]
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'util' )
local structure_parser = require( 'structure-parser' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local dbg = util.dbg
local char = string.char
local NewStructureParser = structure_parser.NewStructureParser

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function format_hex_byte( b ) return format( '%02x', b ) end

local function format_as_binary( n, num_bits )
  local res = ''
  for _ = 1, num_bits do
    local digit = n & 1
    res = digit .. res
    n = n >> 1
  end
  assert( n == 0 )
  assert( #res == num_bits )
  return res
end

-----------------------------------------------------------------
-- BinaryLoader.
-----------------------------------------------------------------
-- Structure Handler. Implements an interface to handler re-
-- sponding to the various components of the structure document.
local BinaryLoader = {}

function BinaryLoader:dbg( ... )
  if not self.debug_logging_ then return end
  local msg = format( ... )
  msg = format( '[%08x] %s', self.sav_file_:seek(), msg )
  dbg( msg )
end

function BinaryLoader:stats()
  local res = {}
  local curr = self.sav_file_:seek()
  res.bytes_read = curr
  local size = self.sav_file_:seek( 'end' )
  self.sav_file_:seek( 'set', curr )
  local remaining = size - curr
  assert( remaining >= 0 )
  res.bytes_remaining = remaining
  return res
end

function BinaryLoader:_byte()
  local c = assert( self.sav_file_:read( 1 ), 'eof' )
  return string.byte( c )
end

function BinaryLoader:_bit_field( n, desc )
  assert( desc.size )
  assert( desc.size > 0 )
  local mask = (1 << desc.size) - 1
  local res = n & mask
  if desc.type == 'uint' then
    return res
  else
    local formatted = format_as_binary( res, desc.size )
    return self:as_meta_bitfield_type( formatted, desc.type )
  end
end

function BinaryLoader:unknown( size )
  assert( size )
  assert( size > 0 )
  local res = ''
  for _ = 1, size do
    local b = self:_byte()
    res = format( '%s%s ', res, format_hex_byte( b ) )
  end
  return res:match( '(.+) +' )
end

function BinaryLoader:bit_struct( bit_struct )
  self:dbg( 'parsing bit_struct [%s]', self:backtrace() )
  assert( bit_struct )
  assert( bit_struct.__key_order )
  local total_bits = 0
  for _, e in ipairs( bit_struct.__key_order ) do
    assert( bit_struct[e] )
    if not e:match( '__' ) then
      total_bits = total_bits + bit_struct[e].size
    end
  end
  assert( total_bits % 8 == 0,
          format( 'found %d bits in bit_struct.', total_bits ) )
  local total_bytes = total_bits // 8
  self:dbg( 'bit_struct has %d bytes.', total_bytes )
  -- This is the largest value seen in the structure file.
  assert( total_bytes <= 6 )
  local as_number = 0
  for i = 1, total_bytes do
    local b = self:_byte()
    assert( b >= 0 and b < 256 )
    self:dbg( 'byte: %s', format_hex_byte( b ) )
    as_number = as_number + (b << ((i - 1) * 8))
  end
  self:dbg( 'as number: %04x', as_number )
  local res = {}
  for _, e in ipairs( bit_struct.__key_order ) do
    if not e:match( '__' ) then
      local desc = bit_struct[e]
      res[e] = self:_bit_field( as_number, desc )
      as_number = as_number >> desc.size
    end
  end
  assert( as_number == 0 )
  res.__key_order = bit_struct.__key_order
  return res
end

function BinaryLoader:string( size )
  self:dbg( 'parsing string [%s]', self:backtrace() )
  assert( size )
  assert( type( size ) == 'number' )
  local res = ''
  local hit_zero = false
  for _ = 1, size do
    local b = self:_byte()
    if b == 0 then hit_zero = true end
    if not hit_zero then res = res .. char( b ) end
    -- Need to keep reading until the end of the loop even if we
    -- hit zero in order to seek past the bytes.
  end
  assert( #res <= size )
  return res
end

function BinaryLoader:uint( size )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  local ns = {}
  for _ = 1, size do
    -- Insert at beginning to make reverse order.
    table.insert( ns, 1, self:_byte() )
  end
  local n = 0
  for _, b in ipairs( ns ) do
    n = n << 8
    n = n + b
  end
  return n
end

function BinaryLoader:int( size )
  local uint = self:uint( size )
  assert( type( uint ) == 'number' )
  local nbits = size * 8
  if uint >> (nbits - 1) == 0 then
    -- positive.
    return uint
  end
  assert( uint >> (nbits - 1) == 1 )
  local fs = 0
  for _ = 1, size do
    fs = fs << 8
    fs = fs + 255
  end
  local positive = (fs & ~uint) + 1
  assert( positive > 0 )
  return -positive
end

function BinaryLoader:bits( nbytes )
  local n = 0
  for _ = 1, nbytes do
    n = n << 8
    n = n + self:_byte()
  end
  return format_as_binary( n, nbytes * 8 )
end

function BinaryLoader:_primitive_grab_one( tbl )
  if tbl.type == 'str' then
    return self:string( tbl.size )
  elseif tbl.type == 'bits' then
    local byte_count = assert( tbl.size )
    return self:bits( byte_count )
  elseif tbl.type == 'uint' then
    return self:uint( tbl.size )
  elseif tbl.type == 'int' then
    return self:int( tbl.size )
  else
    return
        self:as_meta_type( self:unknown( tbl.size ), tbl.type )
  end
end

function BinaryLoader:primitive( tbl )
  assert( type( tbl.size ) == 'number' )
  return self:_primitive_grab_one( tbl )
end

function BinaryLoader:primitive_array( tbl )
  assert( type( tbl.size ) == 'number' )
  assert( tbl.count or tbl.cols )
  local cells = self:lookup_cells( tbl )
  assert( type( cells ) == 'number' )
  local res = {}
  for _ = 1, cells do
    table.insert( res, self:_primitive_grab_one( tbl ) )
  end
  -- Signals this is an array.
  res[0] = cells
  return res
end

function M.NewBinaryLoader( metadata, binary_file )
  local base = NewStructureParser( metadata )
  local obj = {}
  local BinaryLoaderMeta = setmetatable( {}, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=function( _, key )
      if BinaryLoader[key] then return BinaryLoader[key] end
      return base[key]
    end,
    __metatable=false,
  } )
  obj.sav_file_ = assert( io.open( binary_file, 'rb' ) )
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=BinaryLoaderMeta,
    __gc=function( self ) self:close() end,
    __metatable=false,
  } )
  return obj
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
