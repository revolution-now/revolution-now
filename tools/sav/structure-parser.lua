--[[ ------------------------------------------------------------
|
| structure-parser.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: Traverses the SAV structure doc.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local util = require( 'util' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local not_implemented = util.not_implemented

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function add_reverse_metadata( metadata )
  for _, v in pairs( metadata ) do
    if type( v ) ~= 'table' then goto continue end
    if v.__reverse then goto continue end
    local reverse = {}
    for k2, v2 in pairs( v ) do
      assert( not reverse[v2] )
      reverse[v2] = k2
    end
    v.__reverse = reverse
    setmetatable( v.__reverse, {
      __index=function( tbl, k )
        if rawget( tbl, k ) then return rawget( tbl, k ) end
        if rawget( tbl, k:lower() ) then
          return rawget( tbl, k:lower() )
        end
        if rawget( tbl, k:upper() ) then
          return rawget( tbl, k:upper() )
        end
      end,
    } )
    ::continue::
  end
end

-----------------------------------------------------------------
-- StructureParser.
-----------------------------------------------------------------
-- This is the "base class".
local StructureParser = {}

function StructureParser:as_meta_bitfield_type( val, metatype )
  if not metatype then return val end
  assert( type( metatype ) == 'string' )
  if metatype == 'bit_bool' then
    assert( val == '0' or val == '1' )
    if val == '0' then return false end
    if val == '1' then return true end
    error( 'should not be here.' )
  end
  assert( self.metadata_[metatype], metatype )
  local rev = self.metadata_[metatype].__reverse
  return assert( rev[val] )
end

function StructureParser:as_meta_type( val, metatype )
  if not metatype then return val end
  assert( type( metatype ) == 'string' )
  assert( self.metadata_[metatype] )
  local rev = self.metadata_[metatype].__reverse
  assert( type( val ) == 'string', format( 'val is %s', val ) )
  assert( rev[val], format(
              'failed to find value %s in reverse metadata dictionary for metatype %s.',
              val, metatype ) )
  return rev[val]
end

function StructureParser:backtrace()
  local res = 'struct'
  for _, e in ipairs( self.backtrace_ ) do
    res = format( '%s.%s', res, e )
  end
  return res
end

function StructureParser:struct( struct )
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

function StructureParser:struct_array( count, struct )
  self:dbg( 'parsing struct array [%s]', self:backtrace() )
  assert( count )
  assert( count >= 0 )
  local res = {}
  for _ = 1, count do table.insert( res, self:struct( struct ) ) end
  -- Signals this is an array.
  res[0] = count
  return res
end

function StructureParser:bit_struct( bit_struct )
  self:dbg( 'parsing bit_struct [%s]', self:backtrace() )
  assert( bit_struct )
  assert( bit_struct.__key_order )
  local res = {}
  for _, e in ipairs( bit_struct.__key_order ) do
    local field = assert( bit_struct[e] )
    if not e:match( '__' ) then
      self:dbg( 'parsing bit_field [%s] %s', self:backtrace(), e )
      res[e] = self:bit_field( field )
    end
  end
  res.__key_order = bit_struct.__key_order
  return res
end

function StructureParser:bit_struct_array( count, bit_struct )
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

function StructureParser:lookup_cells( tbl )
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

function StructureParser:entity( parent, field )
  assert( parent )
  assert( field )
  local tbl = parent[field]
  assert( type( tbl ) == 'table',
          format( 'field %s is not a table', field ) )
  table.insert( self.backtrace_, field )
  local cells = self:lookup_cells( tbl )
  local res
  if tbl.struct then
    if tbl.count or tbl.cols then
      res = self:struct_array( cells, tbl.struct )
    else
      res = self:struct( tbl.struct )
    end
  elseif tbl.bit_struct then
    if tbl.count or tbl.cols then
      res = self:bit_struct_array( cells, tbl.bit_struct )
    else
      res = self:bit_struct( tbl.bit_struct )
    end
  else
    if tbl.count or tbl.cols then
      res = self:primitive_array( cells, tbl )
    else
      res = self:primitive( tbl )
    end
  end
  assert( res )
  if tbl.save_meta then self.saved_[field] = res end
  table.remove( self.backtrace_ )
  return res
end

function StructureParser:bit_field( _, _ ) not_implemented() end
function StructureParser:unknown( _ ) not_implemented() end
function StructureParser:stats() not_implemented() end
function StructureParser:string( _ ) not_implemented() end
function StructureParser:uint( _ ) not_implemented() end
function StructureParser:int( _ ) not_implemented() end
function StructureParser:bits( _ ) not_implemented() end
function StructureParser:primitive( _ ) not_implemented() end
function StructureParser:primitive_array( _, _ ) not_implemented() end

function StructureParser.new( metadata )
  local obj = {}
  add_reverse_metadata( metadata )
  obj.metadata_ = assert( metadata )
  obj.backtrace_ = {}
  obj.saved_ = {}
  obj.debug_logging_ = false
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify parsers.', 2 ) end,
    __index=StructureParser,
    __metatable=false,
  } )
  return obj
end

setmetatable( StructureParser, {
  __call=function( _, ... ) return StructureParser.new( ... ) end,
} )

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return { StructureParser=StructureParser }
