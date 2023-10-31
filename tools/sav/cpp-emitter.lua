--[[ ------------------------------------------------------------
|
| cpp-emitter.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: Generates C++ SAV loaders.
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
local insert = table.insert
local concat = table.concat
local sort = table.sort

local format = string.format

local dbg = util.dbg

local StructureParser = structure_parser.StructureParser

-----------------------------------------------------------------
-- Code gen.
-----------------------------------------------------------------
local CodeGenerator = {}

function CodeGenerator:newline()
  insert( self.lines_, { indent=0 } )
end

function CodeGenerator:_fragment_unformatted( frag )
  assert( frag )
  local last = #self.lines_
  if last == 0 then
    self:newline()
    last = 1
  end
  local curr = self.lines_[last]
  assert( type( curr ) == 'table' )
  if #curr == 0 then
    -- Initialize line.
    curr.indent = self.indent_
  end
  insert( curr, frag )
end

function CodeGenerator:fragment( fmt, ... )
  if #{ ... } > 0 then
    self:_fragment_unformatted( format( fmt, ... ) )
  else
    self:_fragment_unformatted( fmt )
  end
end

function CodeGenerator:_line_unformatted( text )
  text = text or ''
  self:fragment( text )
  self:newline()
end

function CodeGenerator:line( fmt, ... )
  if #{ ... } > 0 then
    self:_line_unformatted( format( fmt, ... ) )
  else
    self:_line_unformatted( fmt )
  end
end

function CodeGenerator:indent() self.indent_ = self.indent_ + 1 end

function CodeGenerator:unindent()
  self.indent_ = self.indent_ - 1
  assert( self.indent_ >= 0 )
end

function CodeGenerator:with_indent( f )
  self:indent()
  f()
  self:unindent()
end

function CodeGenerator:section( name )
  local bar_top = '/' .. string.rep( '*', 64 )
  local middle = '** ' .. name
  local bar_bottom = string.rep( '*', 65 ) .. '/'
  self:line( bar_top )
  self:line( middle )
  self:line( bar_bottom )
end

function CodeGenerator:comment( text ) self:line( '// ' .. text ) end

function CodeGenerator:include( what )
  self:line( '#include ' .. what )
end

function CodeGenerator:open_ns( name )
  self:line( 'namespace %s {', name )
end

function CodeGenerator:close_ns( name )
  self:newline()
  self:line( '}  // namespace %s', name )
end

function CodeGenerator:result()
  local res = {}
  local single_indent = string.rep( ' ', self.indent_step_ )
  for _, line_tbl in ipairs( self.lines_ ) do
    local line = ''
    if #line_tbl > 0 then
      line = string.rep( single_indent, line_tbl.indent )
      line = line .. concat( line_tbl )
    end
    insert( res, line )
  end
  return res
end

function CodeGenerator.new( indent )
  local obj = {}
  -- Member variables.
  obj.lines_ = {}
  obj.indent_ = 0
  obj.indent_step_ = indent or 2
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=CodeGenerator,
    __metatable=false,
  } )
  return obj
end

setmetatable( CodeGenerator, {
  __call=function( _, ... ) return CodeGenerator.new( ... ) end,
} )

-----------------------------------------------------------------
-- CppEmitter.
-----------------------------------------------------------------
-- Structure Handler. Implements an interface to handler re-
-- sponding to the various components of the structure document.
local CppEmitter = {}

function CppEmitter:dbg( ... )
  if not self.debug_logging_ then return end
  local msg = format( ... )
  dbg( msg )
end

-- function CppEmitter:stats()
--   local res = {}
--   -- TODO
--   return res
-- end

function CppEmitter:unknown( size )
  assert( size )
  assert( size > 0 )
  return { type='std::array', value_type='uint8_t', size=size }
end

function CppEmitter:string( size )
  assert( size )
  assert( type( size ) == 'number' )
  return { type='str', size=size }
end

local ALLOWED_INTEGRAL_SIZES = {
  [1]=true,
  [2]=true,
  [4]=true,
  [8]=true,
}

function CppEmitter:uint( size )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  local res = { size=size }
  local types = {
    [1]='uint8_t',
    [2]='uint16_t',
    [3]=nil,
    [4]='uint32_t',
    [5]=nil,
    [6]=nil,
    [7]=nil,
    [8]='uint64_t',
  }
  res.type = assert( types[size] )
  return res
end

function CppEmitter:int( size )
  assert( size )
  assert( type( size ) == 'number' )
  assert( size > 0 )
  local res = { size=size }
  local types = {
    [1]='int8_t',
    [2]='int16_t',
    [3]=nil,
    [4]='int32_t',
    [5]=nil,
    [6]=nil,
    [7]=nil,
    [8]='int64_t',
  }
  res.type = assert( types[size] )
  return res
end

function CppEmitter:bits( nbytes )
  if ALLOWED_INTEGRAL_SIZES[nbytes] then
    return self:primitive{ size=nbytes, type='uint' }
  end
  return { type='std::array', value_type='uint8_t', size=nbytes }
end

function CppEmitter:_primitive_impl( tbl )
  if tbl.type == 'str' then
    return self:string( tbl.size )
  elseif tbl.type == 'bits' then
    local byte_count = assert( tbl.size )
    return self:bits( byte_count )
  elseif tbl.type == 'uint' then
    return self:uint( tbl.size )
  elseif tbl.type == 'int' then
    return self:int( tbl.size )
  elseif not tbl.type and tbl.size and
      ALLOWED_INTEGRAL_SIZES[tbl.size] then
    return self:uint( tbl.size )
  elseif tbl.type then
    assert( tbl.size )
    return tbl
  else
    return self:unknown( tbl.size )
  end
end

function CppEmitter:primitive( tbl )
  assert( type( tbl.size ) == 'number' )
  return self:_primitive_impl( tbl )
end

function CppEmitter:primitive_array( cells, tbl )
  assert( type( tbl.size ) == 'number' )
  assert( type( cells ) == 'number' )
  local res = self:primitive( tbl )
  res = { type='std::array', value_type=res, size=cells }
  return res
end

function CppEmitter:lookup_cells( tbl )
  local count = tbl.count or 1
  local cols = tbl.cols or 1
  if type( count ) == 'string' or type( cols ) == 'string' then
    return { count=count, cols=cols }
  end
  assert( type( count ) == 'number' )
  assert( type( cols ) == 'number' )
  local res = count * cols
  assert( res >= 0 )
  return res
end

function CppEmitter:bit_field( field )
  return { size=assert( field.size ), type=field.type }
end

function CppEmitter:struct_array( cells, struct )
  local res = self.base_.struct( self, struct )
  if type( cells ) == 'number' then
    return {
      type='std::array',
      value_type=res,
      size=cells,
      __key_order=assert( struct.__key_order ),
    }
  end
  return {
    type='std::vector',
    value_type=res,
    cells=cells,
    __key_order=assert( struct.__key_order ),
  }
end

function CppEmitter:bit_struct_array( cells, bit_struct )
  local res = self.base_.bit_struct( self, bit_struct )
  if type( cells ) == 'number' then
    return {
      type='std::array',
      value_type=res,
      size=cells,
      __key_order=assert( bit_struct.__key_order ),
    }
  end
  return {
    type='std::vector',
    value_type=res,
    cells=cells,
    __key_order=assert( bit_struct.__key_order ),
  }
end

function CppEmitter:entity( parent, field )
  local res = self.base_.entity( self, parent, field )
  local tbl = assert( parent[field],
                      format( 'field %s not found.', field ) )
  assert( type( tbl ) == 'table',
          format( 'field %s is not a table', field ) )
  -- res.cells = self:lookup_cells( tbl ) -- do we need this?
  if tbl.struct then
    insert( self.finished_structs_, res )
  elseif tbl.bit_struct then
    self:dbg( 'adding finished bit struct for field %s.', field )
    for k, _ in pairs( tbl.bit_struct ) do
      self:dbg( '  * %s', k )
    end
    assert( res.__key_order )
    insert( self.finished_bit_structs_, res )
  end
  assert( type( res ) == 'table' )
  res.__name = field
  if type( res.value_type ) == 'table' then
    res.value_type.__name = field
  end
  return res
end

local function replace_non_identifier_chars( raw )
  raw = raw:gsub( '[_()%-, ]', '_' )
  raw = raw:gsub( '%^', 'c' )
  raw = raw:gsub( '%*', 'a' )
  raw = raw:gsub( '~', 't' )
  raw = raw:gsub( ':', 'n' )
  raw = raw:gsub( '=', 'e' )
  raw = raw:gsub( '?', 'q' )
  raw = raw:gsub( '#', 'h' )
  return raw
end

local function as_identifier( raw )
  if raw:match( '^ +$' ) then return 'empty' end
  raw = raw:lower()
  raw = replace_non_identifier_chars( raw )
  if raw:match( '^[0-9]' ) then raw = '_' .. raw end
  if raw == 'goto' then raw = 'g0to' end
  raw = raw:gsub( '_+', '_' )
  -- Remove trailing underscore.
  if raw:match( '[^_]' ) then raw = raw:gsub( '_$', '' ) end
  return raw
end

local function readably_equivalent( l, r )
  local function canonical( raw )
    raw = raw:lower()
    raw = raw:gsub( '[_%- ]', '_' )
    if raw:match( '^[0-9]' ) then raw = '_' .. raw end
    if raw == 'goto' then raw = 'g0to' end
    return raw
  end
  return canonical( l ) == canonical( r )
end

function CppEmitter:emit_bit_struct( emitter, bit_struct, name )
  self:dbg( 'emitting bit struct %s.', name )
  emitter:newline()
  emitter:section( name )
  emitter:line( 'struct %s {', name )
  emitter:indent()
  if bit_struct.value_type then
    bit_struct = bit_struct.value_type
  end
  for _, key in ipairs( bit_struct.__key_order ) do
    assert( bit_struct[key],
            format(
                'key "%s" not found in bit struct of name "%s".',
                key, name ) )
    local member = bit_struct[key]
    assert( member.size )
    assert( type( member.size ) == 'number' )
    assert( member.size >= 0 )
    assert( member.size <= 8 )
    local type = member.type or 'uint8_t'
    local remaps = { bit_bool='bool', uint='uint8_t' }
    type = remaps[type] or type
    emitter:line( '%s %s : %d;', type, as_identifier( key ),
                  member.size );
  end
  emitter:unindent()
  emitter:line( '};' )
end

local function struct_name_for( name )
  assert( name )
  assert( type( name ) == 'string' )
  local title = name:gsub( '%w+', function( word )
    return word:gsub( '^.', string.upper )
  end )
  local camel = title:gsub( '_', '' )
  return replace_non_identifier_chars( camel )
end

function CppEmitter:emit_bit_structs( emitter )
  self:dbg( 'emitting bit structs...' )
  for _, bit_struct in ipairs( self.finished_bit_structs_ ) do
    local name = struct_name_for( assert( bit_struct.__name ) )
    assert( bit_struct.__key_order )
    self:emit_bit_struct( emitter, bit_struct, name )
  end
end

local function elem_type_name( info )
  if type( info ) == 'string' then return info end
  if info.type then
    if info.type == 'std::array' then
      return format( 'std::array<%s, %s>',
                     elem_type_name( info.value_type ), info.size )
    end
    return elem_type_name( info.type )
  end
  if info.__name then return struct_name_for( info.__name ) end
  error( 'failed to find elem type name.' )
end

function CppEmitter:emit_struct( emitter, struct, name )
  self:dbg( 'emitting struct %s.', name )
  emitter:newline()
  emitter:section( name )
  emitter:line( 'struct %s {', name )
  emitter:indent()
  if struct.value_type then struct = struct.value_type end
  for _, key in ipairs( struct.__key_order ) do
    if key:match( '__' ) then goto continue end
    assert( struct[key],
            format(
                'key "%s" not found in bit struct of name "%s".',
                key, name ) )
    local member = struct[key]
    -- assert( member.size )
    -- assert( type( member.size ) == 'number' )
    -- assert( member.size >= 0 )
    -- assert( member.size <= 8 )
    -- emitter:line( 'uint8_t %s : %d;', key, member.size );
    if member.type == 'std::vector' then
      emitter:line( 'std::vector<%s> %s = {};',
                    elem_type_name( member.value_type ),
                    as_identifier( key ) )
    elseif member.type == 'std::array' then
      emitter:line( '%s %s = {};', elem_type_name( member ),
                    as_identifier( key ) );
    elseif member.type == 'str' then
      emitter:line( 'char %s[%d] = {};', as_identifier( key ),
                    member.size )
    elseif member.type then
      emitter:line( '%s %s = {};', member.type,
                    as_identifier( key ) )
    else
      emitter:line( '%s %s = {};',
                    struct_name_for( assert( member.__name ) ),
                    as_identifier( key ) )
    end
    ::continue::
  end
  emitter:unindent()
  emitter:line( '};' )
end

function CppEmitter:emit_structs( emitter )
  self:dbg( 'emitting structs...' )
  for _, struct in ipairs( self.finished_structs_ ) do
    local name = struct_name_for( assert( struct.__name ) )
    assert( struct.__key_order )
    self:emit_struct( emitter, struct, name )
  end
end

local function metadata_field_value( field, str_value )
  local bits = field:match( '_([0-9]+)bit_' )
  if bits then
    return format( '0b%s', str_value )
  else
    local bytes = {}
    str_value:gsub( '[0-9a-zA-Z]+', function( byte )
      insert( bytes, 1, byte )
    end )
    return format( '0x%s', concat( bytes ) )
  end
end

function CppEmitter:emit_metadata_item( emitter, name, elems )
  self:dbg( 'emitting metadata for %s.', name )
  emitter:newline()
  emitter:section( name )
  emitter:line( 'enum class %s {', name )
  emitter:indent()
  local ordered_elems = {}
  for k, v in pairs( elems ) do
    if k:match( '__' ) then goto continue end
    assert( type( k ) == 'string', k )
    assert( type( v ) == 'string', k )
    insert( ordered_elems, { field=k, str_value=v } )
    ::continue::
  end
  sort( ordered_elems,
        function( l, r ) return l.str_value < r.str_value end )
  local result = {}
  for _, pair in ipairs( ordered_elems ) do
    local field, str_value = pair.field, pair.str_value
    if field:match( '__' ) then goto continue end
    local numeric_value = metadata_field_value( name, str_value )
    insert( result, {
      field=as_identifier( field ),
      original_field=field,
      numeric_value=numeric_value,
    } )
    ::continue::
  end
  local max_key_width = 0
  for _, pair in ipairs( result ) do
    local field = pair.field
    assert( type( field ) == 'string' )
    max_key_width = math.max( max_key_width, #field )
  end
  local fmt = format( '%%-%ds = %%s,%%s', max_key_width )
  for _, pair in ipairs( result ) do
    local field, original_field, numeric_value = pair.field,
                                                 pair.original_field,
                                                 pair.numeric_value
    local comment = ''
    if not readably_equivalent( field, original_field ) then
      comment = format( '  // original: "%s"', original_field )
    end
    emitter:line( fmt, field, numeric_value, comment )
  end
  emitter:unindent()
  emitter:line( '};' )
end

function CppEmitter:emit_metadata( emitter )
  self:dbg( 'emitting metadata...' )
  local ordered_keys = {}
  for k, _ in pairs( self.metadata_ ) do
    if k:match( '__' ) then goto continue end
    assert( type( k ) == 'string', k )
    insert( ordered_keys, k )
    ::continue::
  end
  sort( ordered_keys )
  for _, name in ipairs( ordered_keys ) do
    if name:match( '__' ) then goto continue end
    local elems = assert( self.metadata_[name] )
    self:emit_metadata_item( emitter, name, elems )
    ::continue::
  end
end

function CppEmitter:generate_code()
  local hpp = CodeGenerator()
  hpp:section( 'Classic Colonization Save File Structure.' )
  hpp:comment(
      'NOTE: this file was auto-generated. DO NOT MODIFY!' )
  hpp:newline()
  hpp:include( '<array>' )
  hpp:include( '<cstdint>' )
  hpp:include( '<vector>' )
  hpp:newline()
  hpp:open_ns( 'sav' )
  self:emit_metadata( hpp )
  self:emit_bit_structs( hpp )
  self:emit_structs( hpp )
  hpp:close_ns( 'sav' )
  local hpp_lines = hpp:result()

  local cpp = CodeGenerator()
  cpp:section( 'Test.' )
  cpp:comment( 'Hello World.' )
  cpp:newline()
  cpp:include( '<string>' )
  cpp:newline()
  cpp:section( 'Bar' )
  cpp:fragment( 'struct ' )
  cpp:fragment( 'Bar ' )
  cpp:line( '{' )
  cpp:with_indent( function()
    cpp:line( 'int x = {};' )
    cpp:line( 'double y = {};' )
    cpp:line( 'std::string %s;', 'var_name' )
  end )
  cpp:line( '};' )
  local cpp_lines = cpp:result()

  return { hpp=hpp_lines, cpp=cpp_lines }
end

function M.NewCppEmitter( metadata )
  local base = StructureParser( metadata )
  local obj = {}
  local CppEmitterMeta = setmetatable( {}, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=function( _, key )
      if CppEmitter[key] then return CppEmitter[key] end
      return base[key]
    end,
    __metatable=false,
  } )
  obj.base_ = base
  obj.finished_bit_structs_ = {}
  obj.finished_structs_ = {}
  setmetatable( obj, {
    __newindex=function() error( 'cannot modify.', 2 ) end,
    __index=CppEmitterMeta,
    __gc=function( self ) self:close() end,
    __metatable=false,
  } )
  return obj
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
