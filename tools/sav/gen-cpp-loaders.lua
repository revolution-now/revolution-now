--[[ ------------------------------------------------------------
|
| gen-cpp-loaders.lua
|
| Project: Revolution Now
|
| Created by David P. Sicilia on 2023-10-29.
|
| Description: Generates C++ SAV loaders.
|
--]] ------------------------------------------------------------
local cpp_emitter = require( 'cpp-emitter' )
local util = require( 'util' )
local json_transcode = require( 'json-transcode' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local exit = os.exit

local format = string.format

local err = util.err
local info = util.info

local CppEmitter = cpp_emitter.CppEmitter
local json_decode = json_transcode.decode

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function usage()
  err( 'usage: gen-cpp-loaders.lua ' .. --
  '<structure-json-file> ' .. --
  '<output-dir>' )
end

local function emit_lines( out, lines )
  assert( lines )
  for i, line in ipairs( lines ) do
    local emit = line .. '\n'
    if i == #lines then emit = line end
    out:write( emit )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( args )
  -- Check args.
  if #args < 2 then
    usage()
    return 1
  end
  if #args > 2 then warn( 'extra unused arguments detected' ) end

  -- Structure document.
  local structure_json = assert( args[1] )
  info( 'decoding json structure file %s...', structure_json )
  local structure = json_decode(
                        io.open( structure_json, 'r' ):read( 'a' ) )
  info( 'producing reverse metadata mapping...' )
  assert( structure.__metadata )
  assert( structure.HEAD )

  -- Output header file.
  local output_dir = assert( args[2] )
  local hpp_file = format( '%s/sav-struct.hpp', output_dir )
  local cpp_file = format( '%s/sav-struct.cpp', output_dir )
  info( 'emitting hpp file %s', hpp_file )
  info( 'emitting cpp file %s', cpp_file )
  local hpp = assert( io.open( hpp_file, 'w' ) )
  local cpp = assert( io.open( cpp_file, 'w' ) )

  -- Emitting.
  info( 'running emitter...' )
  local emitter = assert( CppEmitter( structure.__metadata ) )
  local top = emitter:struct( structure )
  top.__name = 'ColonySAV'
  table.insert( emitter.finished_structs_, top )
  info( 'generating source code...' )
  local res = emitter:generate_code()
  local hpp_lines = assert( res.hpp )
  local cpp_lines = assert( res.cpp )

  -- Encoding and outputting JSON.
  info( 'emitting hpp to %s...', hpp_file )
  emit_lines( hpp, hpp_lines )
  emit_lines( cpp, cpp_lines )

  return 0
end

exit( assert( main( table.pack( ... ) ) ) )
