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
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "logging.hpp"

// Flatbuffers
#include "fb/testing_generated.h"
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

namespace {

template<int N>
struct disambiguate;

struct UclArchiver {
  // All types that have a `serialize` member function.
  template<typename ObjectType, disambiguate<0>* = nullptr>
  auto save( string_view field_name, ObjectType const& o )
      -> void_t<decltype( o.serialize( *this ) )> {
    print_line_prefix( field_name );
    result += fmt::format( "{{\n" );
    level++;
    o.serialize( *this );
    level--;
    result += fmt::format( "{}}}\n", spaces(), field_name );
  }

  void save( string_view field_name, char n ) {
    print_line_prefix( field_name );
    result += fmt::format( "'{}'\n", n );
  }

  void save( string_view field_name, int n ) {
    print_line_prefix( field_name );
    result += fmt::format( "{}\n", n );
  }

  void save( string_view field_name, double d ) {
    print_line_prefix( field_name );
    result += fmt::format( "{}\n", d );
  }

  void save( string_view field_name, string const& s ) {
    print_line_prefix( field_name );
    result += fmt::format( "\"{}\"\n", s );
  }

  // Types for which there is a `serialize` free function found
  // by ADL.
  template<typename ADLserializable, disambiguate<1>* = nullptr>
  auto save( string_view            field_name,
             ADLserializable const& serializable )
      -> void_t<decltype( serialize( *this, serializable ) )> {
    print_line_prefix( field_name );
    result += fmt::format( "{{\n" );
    level++;
    serialize( *this, serializable );
    level--;
    result += fmt::format( "{}}}\n", spaces() );
  }

  // Vectors of serializable types.
  template<typename T>
  auto save( string_view field_name, Vec<T> const& v )
      -> void_t<decltype( save( field_name, v[0] ) )> {
    print_line_prefix( field_name );
    result +=
        fmt::format( "[\n", spaces(), field_name, v.size() );
    level++;
    for( auto const& e : v ) save( "", e );
    level--;
    result += fmt::format( "{}]\n", spaces() );
  }

  // Should not serialize pointers.
  template<typename T>
  void save( string_view field_name, T* ) = delete;

  // Better-enums.
  template<typename ReflectedEnum>
  auto save( string_view field_name, ReflectedEnum const& e )
      -> void_t<typename ReflectedEnum::_enumerated> {
    print_line_prefix( field_name );
    result += fmt::format( "\"{}\"\n", e );
  }

  void print_line_prefix( string_view field_name ) {
    string if_not_empty =
        field_name.empty() ? ""
                           : fmt::format( "{}: ", field_name );
    result += fmt::format( "{}{}", spaces(), if_not_empty );
  }

  string spaces() const { return string( level * 2, ' ' ); }

  int    level{0};
  string result;
};

} // namespace

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
  return expect<BinaryBlob>( std::move( *byte_buffer ) );
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

/****************************************************************
** Testing
*****************************************************************/
void test_serial() {
  // == Direct ==================================================
  // SaveableOmni omni;
  // UclArchiver  ar;
  // ar.save( "Parent", omni );
  // lg.info( "result:\n{}", ar.result );
}

} // namespace rn::serial
