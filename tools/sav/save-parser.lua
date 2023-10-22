--[[ ------------------------------------------------------------
|
| sav.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-16.
|
| Description: Prototype *.SAV parser (OG's save-game files).
|
--]] ------------------------------------------------------------
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local lunajson = require( 'lunajson' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local char = string.char
local exit = os.exit

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local ANSI_NORMAL = char( 27 ) .. '[00m'
local ANSI_GREEN = char( 27 ) .. '[32m'
local ANSI_RED = char( 27 ) .. '[31m'
local ANSI_YELLOW = char( 27 ) .. '[93m'
local ANSI_BLUE = char( 27 ) .. '[34m'
local ANSI_BOLD = char( 27 ) .. '[1m'
-- local ANSI_UNDER = char( 27 ) .. '[4m'

-----------------------------------------------------------------
-- General utils.
-----------------------------------------------------------------
local function printf( ... ) print( format( ... ) ) end

local function log( ... )
  local msg = format( ... )
  printf( '%sinfo%s %s', ANSI_GREEN, ANSI_NORMAL, msg )
end

local function warn( ... )
  local msg = format( ... )
  printf( '%swarning%s %s', ANSI_YELLOW, ANSI_NORMAL, msg )
end

local function dbg( ... )
  local msg = format( ... )
  printf( '%sdebug%s %s', ANSI_BLUE, ANSI_NORMAL, msg )
end

local function err( ... )
  local msg = format( ... )
  printf( '%s%serror%s %s', ANSI_RED, ANSI_BOLD, ANSI_NORMAL, msg )
end

local function fatal( ... )
  err( ... )
  exit( 1 )
end

local function check( condition, ... )
  if not condition then fatal( ... ) end
end

local function usage()
  err(
      'usage: sav.lua <structure-json-file> <SAV-filename> <output>' )
end

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
-- JSON helpers.
-----------------------------------------------------------------
local JNULL = {}

-- Decode  a string of json.
local function json_decode( json_string )
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

-----------------------------------------------------------------
-- Structure document.
-----------------------------------------------------------------
local function add_reverse_metadata( metadata )
  for _, v in pairs( metadata ) do
    if type( v ) == 'table' then
      local reverse = {}
      for k2, v2 in pairs( v ) do
        assert( not reverse[v2] )
        reverse[v2] = k2
      end
      v.__reverse = reverse
      setmetatable( v.__reverse, {
        __index=function( tbl, k )
          if rawget( tbl, k ) then
            return rawget( tbl, k )
          end
          if rawget( tbl, k:lower() ) then
            return rawget( tbl, k:lower() )
          end
          if rawget( tbl, k:upper() ) then
            return rawget( tbl, k:upper() )
          end
        end,
      } )
    end
  end
end

-----------------------------------------------------------------
-- Parsers.
-----------------------------------------------------------------
local SAVParser = {}

function SAVParser:dbg( ... )
  if not self.debug_logging_ then return end
  local msg = format( ... )
  msg = format( '[%08x] %s', self.sav_file_:seek(), msg )
  dbg( msg )
end

function SAVParser:backtrace()
  local res = 'top'
  for _, e in ipairs( self.backtrace_ ) do
    res = format( '%s.%s', res, e )
  end
  return res
end

function SAVParser:stats()
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

function SAVParser:as_meta_type( val, metatype )
  if not metatype then return val end
  assert( type( metatype ) == 'string' )
  assert( self.metadata_[metatype] )
  local rev = self.metadata_[metatype].__reverse
  assert( type( val ) == 'string', format( 'val is %s', val ) )
  return assert( rev[val] )
end

function SAVParser:as_meta_bitfield_type( val, metatype )
  if not metatype then return val end
  assert( type( metatype ) == 'string' )
  if metatype == 'bit_bool' then
    assert( val == '0' or val == '1' )
    if val == '0' then return false end
    if val == '1' then return true end
    error( 'should not be here.' )
  end
  assert( self.metadata_[metatype] )
  local rev = self.metadata_[metatype].__reverse
  return assert( rev[val] )
end

function SAVParser:byte()
  local c = assert( self.sav_file_:read( 1 ), 'eof' )
  return string.byte( c )
end

function SAVParser:bytes( n )
  local res = {}
  for _ = 1, n do table.insert( res, self:byte() ) end
  return res
end

function SAVParser:bit_field( n, desc )
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

function SAVParser:unknown( size )
  assert( size )
  assert( size > 0 )
  local res = ''
  for _ = 1, size do
    local b = self:byte()
    res = format( '%s%s ', res, format_hex_byte( b ) )
  end
  return res:match( '(.+) +' )
end

function SAVParser:struct( struct )
  self:dbg( 'parsing struct [%s]', self:backtrace() )
  assert( struct )
  assert( struct.__key_order )
  assert( not struct.struct )
  assert( not struct.bit_struct )
  assert( not struct.size )
  local res = {}
  for _, e in ipairs( struct.__key_order ) do
    assert( struct[e] )
    if not e:match( '__' ) then
      res[e] = self:entity( struct, e )
    end
  end
  res.__key_order = struct.__key_order
  return res
end

function SAVParser:struct_array( count, struct )
  self:dbg( 'parsing struct array [%s]', self:backtrace() )
  assert( count )
  assert( count >= 0 )
  local res = {}
  for _ = 1, count do table.insert( res, self:struct( struct ) ) end
  -- Signals this is an array.
  res[0] = count
  return res
end

function SAVParser:bit_struct( bit_struct )
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
  assert( total_bytes <= 4 )
  local as_number = 0
  for i = 1, total_bytes do
    local b = self:byte()
    assert( b >= 0 and b < 256 )
    self:dbg( 'byte: %s', format_hex_byte( b ) )
    as_number = as_number + (b << ((i - 1) * 8))
  end
  self:dbg( 'as number: %04x', as_number )
  local res = {}
  for _, e in ipairs( bit_struct.__key_order ) do
    if not e:match( '__' ) then
      local desc = bit_struct[e]
      res[e] = self:bit_field( as_number, desc )
      as_number = as_number >> desc.size
    end
  end
  assert( as_number == 0 )
  res.__key_order = bit_struct.__key_order
  return res
end

function SAVParser:bit_struct_array( count, bit_struct )
  self:dbg( 'parsing bit_struct array [%s]', self:backtrace() )
  assert( count )
  assert( count >= 0 )
  local res = {}
  for _ = 1, count do
    table.insert( res, self:bit_struct( bit_struct ) )
  end
  -- Signals this is an array.
  res[0] = count
  return res
end

function SAVParser:string( size )
  self:dbg( 'parsing string [%s]', self:backtrace() )
  assert( size )
  assert( type( size ) == 'number' )
  local res = ''
  local hit_zero = false
  for _ = 1, size do
    local b = self:byte()
    if b == 0 then hit_zero = true end
    if not hit_zero then res = res .. char( b ) end
    -- Need to keep reading until the end of the loop even if we
    -- hit zero in order to seek past the bytes.
  end
  assert( #res <= size )
  return res
end

function SAVParser:uint( size )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  local ns = {}
  for _ = 1, size do
    -- Insert at beginning to make reverse order.
    table.insert( ns, 1, self:byte() )
  end
  local n = 0
  for _, b in ipairs( ns ) do
    n = n << 8
    n = n + b
  end
  return n
end

function SAVParser:int( size )
  local uint = self:uint( size )
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

function SAVParser:bits( nbytes )
  local n = 0
  for _ = 1, nbytes do
    n = n << 8
    n = n + self:byte()
  end
  return format_as_binary( n, nbytes * 8 )
end

function SAVParser:lookup_cells( tbl )
  local count = tbl.count or 1
  local cols = tbl.cols or 1
  if type( count ) == 'string' then
    count = assert( self.saved_[count] )
  end
  if type( cols ) == 'string' then
    cols = assert( self.saved_[cols] )
  end
  assert( type( count ) == 'number' )
  assert( type( cols ) == 'number' )
  local res = count * cols
  assert( res >= 0 )
  return res
end

function SAVParser:primitive( tbl )
  assert( type( tbl.size ) == 'number' )
  local grab_one = function()
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
      return self:as_meta_type( self:unknown( tbl.size ),
                                tbl.type )
    end
  end
  if tbl.count or tbl.cols then
    local cells = self:lookup_cells( tbl )
    assert( type( cells ) == 'number' )
    local res = {}
    for _ = 1, cells do table.insert( res, grab_one() ) end
    return res
  else
    return grab_one()
  end
end

function SAVParser:entity( parent, field )
  assert( parent )
  assert( field )
  local tbl = parent[field]
  assert( type( tbl ) == 'table',
          format( 'field %s is not a table', field ) )
  table.insert( self.backtrace_, field )
  local res
  if tbl.struct then
    if tbl.count or tbl.cols then
      local cells = self:lookup_cells( tbl )
      assert( type( cells ) == 'number' )
      res = self:struct_array( cells, tbl.struct )
    else
      res = self:struct( tbl.struct )
    end
  elseif tbl.bit_struct then
    if tbl.count or tbl.cols then
      local cells = self:lookup_cells( tbl )
      assert( type( cells ) == 'number' )
      res = self:bit_struct_array( cells, tbl.bit_struct )
    else
      res = self:bit_struct( tbl.bit_struct )
    end
  else
    res = self:primitive( tbl )
  end
  assert( res )
  if tbl.save_meta then self.saved_[field] = res end
  table.remove( self.backtrace_ )
  return res
end

local function NewSAVParser( sav_file, metadata )
  local obj = {}
  obj.sav_file_ = sav_file
  obj.metadata_ = metadata
  obj.backtrace_ = {}
  obj.saved_ = {}
  obj.debug_logging_ = false
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify parsers.', 2 ) end,
    __index=SAVParser,
    __metatable=false,
  } )
  return obj
end

local function parse_sav( structure, sav )
  assert( sav )
  assert( structure.__metadata )
  local parser = NewSAVParser( sav, structure.__metadata )
  local res
  local success, msg = pcall( function()
    res = parser:struct( structure, 1 )
    res._type = 'SAV'
  end )
  assert( success, format( 'error at location [%s]: %s',
                           parser:backtrace(), msg ) )
  return res, parser:stats()
end

-----------------------------------------------------------------
-- Reporters.
-----------------------------------------------------------------
-- Recursive function that pretty-prints a layout in conforming
-- JSON while preserving key ordering.
local function pprint_json( o, prefix, spaces )
  spaces = spaces or ''
  assert( o ~= JNULL )
  if type( o ) == 'table' and not o[0] then
    -- Object.
    local name = o._type or ''
    if #name > 0 then name = name .. ' ' end
    local s = ''
    if #spaces == 0 then s = prefix end
    s = s .. name .. '{\n'
    spaces = spaces .. '  '
    local keys = o.__key_order
    if not keys then
      keys = {}
      for k, _ in pairs( o ) do table.insert( keys, k ) end
      table.sort( keys )
    end
    local total_emitted_keys = 0
    for _, k in ipairs( keys ) do
      if not tostring( k ):match( '^_' ) then
        total_emitted_keys = total_emitted_keys + 1
      end
    end
    for i, k in ipairs( keys ) do
      if not tostring( k ):match( '^_' ) then
        assert( o[k] ~= nil )
        local v = o[k]
        local k_str = tostring( k )
        k_str = '"' .. k_str .. '"'
        s = s .. prefix .. spaces .. k_str .. ': ' ..
                pprint_json( v, prefix, spaces )
        if i ~= total_emitted_keys then s = s .. ',' end
        s = s .. '\n'
      end
    end
    return s .. prefix .. string.sub( spaces, 3 ) .. '}'
  elseif type( o ) == 'table' and o[0] then
    -- Array.
    local name = o._type or ''
    if #name > 0 then name = name .. ' ' end
    local s = ''
    if #spaces == 0 then s = prefix end
    s = s .. name .. '[\n'
    spaces = spaces .. '  '
    local total_emitted_keys = #o
    for i, e in ipairs( o ) do
      s = s .. prefix .. spaces ..
              pprint_json( e, prefix, spaces )
      if i ~= total_emitted_keys then s = s .. ',' end
      s = s .. '\n'
    end
    return s .. prefix .. string.sub( spaces, 3 ) .. ']'
  elseif type( o ) == 'string' then
    local s = ''
    if #spaces == 0 then s = prefix end
    return s .. '"' .. tostring( o ) .. '"'
  else
    local s = ''
    if #spaces == 0 then s = prefix end
    return s .. tostring( o )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  if #args < 3 then
    usage()
    return 1
  end
  if #args > 3 then warn( 'extra unused arguments detected' ) end
  local structure_json = assert( args[1] )
  log( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  log( 'producing reverse metadata mapping...' )
  assert( structure.__metadata )
  add_reverse_metadata( structure.__metadata )
  local colony_sav = assert( args[2] )
  check( colony_sav:match( '^COLONY%d%d%.SAV$' ),
         'colony_sav %s has invalid format.', colony_sav )
  log( 'reading save file %s', colony_sav )
  local output_json = assert( args[3] )
  local parsed, stats = parse_sav( structure, assert(
                                       io.open( colony_sav, 'rb' ) ) )
  log( 'finished parsing. stats:' )
  log( 'bytes read: %d', stats.bytes_read )
  if stats.bytes_remaining > 0 then
    err( 'bytes remaining: %d', stats.bytes_remaining )
  end
  log( 'encoding json...' )
  local json_sav = pprint_json( parsed, '', '' )
  log( 'writing output file %s...', output_json )
  io.open( output_json, 'w' ):write( json_sav )
  return 0
end

os.exit( assert( main( table.pack( ... ) ) ) )