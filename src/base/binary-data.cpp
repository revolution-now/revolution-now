/****************************************************************
**binary-data.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: For ease of working with binary buffers.
*
*****************************************************************/
#include "binary-data.hpp"

// C++ standard library.
#include <filesystem>
#include <fstream>

using namespace std;

namespace fs = std::filesystem;

namespace base {

using ::base::valid;
using ::base::valid_or;

/****************************************************************
** BinaryBuffer
*****************************************************************/
base::expect<BinaryBuffer, string> BinaryBuffer::from_file(
    string const& path ) {
  if( !fs::exists( path ) )
    return fmt::format( "file {} does not exist.", path );
  error_code      ec        = {};
  uintmax_t const file_size = fs::file_size( path, ec );
  if( ec.value() != 0 )
    return fmt::format( "could not determine size of file {}.",
                        path );

  ifstream in( path, ios::binary );
  if( !in.good() )
    return fmt::format( "failed to open file {}.", path );

  vector<unsigned char> buffer;
  buffer.resize( file_size );
  in.read( reinterpret_cast<char*>( buffer.data() ), file_size );
  if( in.tellg() < int( file_size ) )
    return fmt::format(
        "failed to read all {} bytes of file {}; only read {}.",
        file_size, path, in.gcount() );
  if( in.tellg() > int( file_size ) )
    return fmt::format(
        "internal error: more bytes read from file {} than "
        "requested; requested {}, read {}.",
        path, file_size, in.tellg() );

  return BinaryBuffer( std::move( buffer ) );
}

valid_or<string> BinaryBuffer::write_file( string const& path,
                                           int           n ) {
  ofstream out( path, ios::binary );
  if( !out.good() )
    return fmt::format( "failed to open file {}.", path );

  n = std::min( n, int( buffer_.size() ) );
  out.write( reinterpret_cast<char*>( buffer_.data() ), n );
  if( !out.good() )
    return fmt::format( "failed to write {} bytes to file {}.",
                        buffer_.size(), path );

  return valid;
}

/****************************************************************
** BinaryData
*****************************************************************/
bool BinaryData::eof() const { return !good( 1 ); }

bool BinaryData::good( int nbytes ) const {
  int const remaining = buffer_.size() - ( idx_ + nbytes );
  return ( remaining >= 0 );
}

bool BinaryData::read_bytes( int n, unsigned char* dst ) {
  if( !good( n ) ) return false;
  std::memcpy( dst, &buffer_[idx_], n );
  idx_ += n;
  CHECK_LE( pos(), size() );
  return true;
}

bool BinaryData::write_bytes( int n, unsigned char const* src ) {
  if( !good( n ) ) return false;
  std::memcpy( &buffer_[idx_], src, n );
  idx_ += n;
  return true;
}

} // namespace base
