/****************************************************************
**serial.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-28.
*
* Description: Serialization.
*
*****************************************************************/
#include "serial.hpp"

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "io.hpp"
#include "logging.hpp"
#include "meta.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// Flatbuffers
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/minireflect.h"

using namespace std;

namespace rn::serial {

// Hack to ensure that flatbuffers is being compiled with const-
// expr on. FIXME: can remove this after setting a proper CMake
// toolchain file that the flatbuffer build is looking for.
inline FLATBUFFERS_CONSTEXPR int fb_ensure_constexpr_impl = 0;
inline constexpr int             fb_ensure_constexpr =
    fb_ensure_constexpr_impl;

namespace {} // namespace

/****************************************************************
** Binary Blobs
*****************************************************************/
expect<ByteBuffer> ByteBuffer::read( fs::path const& file ) {
  if( !fs::exists( file ) )
    return UNEXPECTED( "file `{}` does not exist.", file );
  auto size = fs::file_size( file );

  auto fp = ::fopen( string( file ).c_str(), "rb" );
  if( !fp )
    return UNEXPECTED( "failed to open file `{}`", file );

  ByteBuffer buffer( new uint8_t[size], size );
  if( ::fread( buffer.get(), 1, size, fp ) != size_t( size ) )
    return UNEXPECTED( "failed to read entire file: {}", file );
  return buffer;
}

expect<BinaryBlob> BinaryBlob::read( fs::path const& path ) {
  XP_OR_RETURN( byte_buffer, ByteBuffer::read( path ) );
  return std::move( byte_buffer );
}

expect<> BinaryBlob::write( fs::path const& file ) const {
  // !! Must not access members directly in this function,
  // should call the member functions.
  auto fp = ::fopen( file.c_str(), "wb" );
  if( !fp )
    return UNEXPECTED( "failed to open file `{}`.", file );
  // This must be called after `Finish()`.
  auto wrote = ::fwrite( get(), 1, size(), fp );
  ::fclose( fp );
  if( wrote != size_t( size() ) )
    return UNEXPECTED(
        "failed to write {} bytes to file {}; wrote only {}.",
        size(), file, wrote );
  return xp_success_t{};
}

BinaryBlob BinaryBlob::from_builder(
    flatbuffers::FlatBufferBuilder builder ) {
  size_t out_size, out_offset;
  auto*  buf = builder.ReleaseRaw( out_size, out_offset );
  return BinaryBlob( buf, int( out_size ), int( out_offset ) );
}

expect<BinaryBlob> BinaryBlob::from_json(
    fs::path const& schema_file_name,
    fs::path const& json_file_path, string_view root_type ) {
  flatbuffers::Parser parser;
  // Store this as a string so that we can then pass C strings
  // safely to the Parser API.
  string include = config_rn.flatbuffers.include_path.string();
  char const* c_includes[] = {include.c_str(), nullptr};

  auto schema_path =
      config_rn.flatbuffers.include_path / schema_file_name;
  XP_OR_RETURN( schema, rn::read_file_as_string( schema_path ) );
  if( !parser.Parse( schema.c_str(), c_includes ) )
    return UNEXPECTED( "failed to parse schema file `{}`: {}",
                       schema_path, parser.error_ );

  if( !parser.SetRootType( string( root_type ).c_str() ) )
    return UNEXPECTED( "failed to set root type: `{}`.",
                       root_type );

  XP_OR_RETURN( json,
                rn::read_file_as_string( json_file_path ) );
  if( !parser.Parse( json.c_str() ) )
    return UNEXPECTED(
        "failed to parse JSON flatbuffers file `{}`: {}",
        json_file_path, parser.error_ );

  return from_builder( std::move( parser.builder_ ) );
}

/****************************************************************
** Testing
*****************************************************************/
void test_serial() {}

} // namespace rn::serial
