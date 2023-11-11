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

using namespace std;

namespace base {

/****************************************************************
** IBinaryIO
*****************************************************************/
int IBinaryIO::remaining() const {
  auto remaining = size() - pos();
  CHECK_GE( remaining, 0 );
  return remaining;
}

bool IBinaryIO::eof() const { return ( remaining() == 0 ); }

vector<IBinaryIO::Byte> IBinaryIO::read_remainder() {
  vector<Byte> res;
  int const    count = remaining();
  res.resize( count );
  read_bytes( count, res.data() );
  return res;
}

/****************************************************************
** FileBinaryIO.
*****************************************************************/
expect<FileBinaryIO, string>
FileBinaryIO::open_for_rw_fail_on_nonexist(
    std::string const& path ) {
  FILE* const fp = std::fopen( path.c_str(), "rb+" );
  if( fp == nullptr )
    return fmt::format(
        "failed to open file \"{}\" for reading.", path );
  return FileBinaryIO( fp );
}

expect<FileBinaryIO, string>
FileBinaryIO::open_for_rw_and_truncate(
    std::string const& path ) {
  FILE* const fp = std::fopen( path.c_str(), "wb+" );
  if( fp == nullptr )
    return fmt::format(
        "failed to open file \"{}\" for writing.", path );
  return FileBinaryIO( fp );
}

void FileBinaryIO::free_resource() {
  FILE* const fp = resource();
  CHECK( fp != nullptr );
  std::fclose( fp );
  // NOTE: zero will handle clearing the file handle.
}

int FileBinaryIO::pos() const {
  FILE* const fp  = resource();
  int const   res = std::ftell( fp );
  // On an error it could return -1, but there isn't much we can
  // really do to handle that here, so we'll just return 0 since
  // that is at least a valid result.
  return std::max( res, 0 );
}

int FileBinaryIO::size() const {
  FILE* const fp       = resource();
  int const   orig_pos = pos();
  std::fseek( fp, 0, SEEK_END );
  auto file_size = std::ftell( fp );
  std::fseek( fp, orig_pos, SEEK_SET );
  CHECK_EQ( pos(), orig_pos );
  return file_size;
}

bool FileBinaryIO::read_bytes( int n, unsigned char* dst ) {
  FILE* const  fp   = resource();
  size_t const read = std::fread( dst, 1, n, fp );
  return ( int( read ) == n );
}

bool FileBinaryIO::write_bytes( int                  n,
                                unsigned char const* src ) {
  FILE* const  fp      = resource();
  size_t const written = std::fwrite( src, 1, n, fp );
  return ( int( written ) == n );
}

/****************************************************************
** MemBufferBinaryIO
*****************************************************************/
bool MemBufferBinaryIO::read_bytes( int n, unsigned char* dst ) {
  if( remaining() < n ) return false;
  std::memcpy( dst, &buffer_[idx_], n );
  idx_ += n;
  CHECK_LE( pos(), size() );
  return true;
}

bool MemBufferBinaryIO::write_bytes( int                  n,
                                     unsigned char const* src ) {
  if( remaining() < n ) return false;
  std::memcpy( &buffer_[idx_], src, n );
  idx_ += n;
  return true;
}

} // namespace base
