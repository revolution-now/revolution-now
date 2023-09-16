/****************************************************************
**io.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: File IO utilities.
*
*****************************************************************/
#include "io.hpp"

// base
#include "fmt.hpp"
#include "scope-exit.hpp"

// C++ standard library
#include <cstring>
#include <exception>

using namespace std;

namespace base {

string error_read_text_file_msg( string_view            filename,
                                 e_error_read_text_file e ) {
  switch( e ) {
    case e_error_read_text_file::file_does_not_exist:
      return fmt::format( "file {} does not exist", filename );
    case e_error_read_text_file::alloc_failure:
      return fmt::format(
          "failed to allocate memory for buffer for file {}",
          filename );
    case e_error_read_text_file::open_file_failure:
      return fmt::format( "failed to open file {}", filename );
    case e_error_read_text_file::incomplete_read:
      return fmt::format( "failed to read all bytes in file {}",
                          filename );
  }
}

void to_str( e_error_read_text_file val, std::string& out,
             ADL_t ) {
  switch( val ) {
    case e_error_read_text_file::file_does_not_exist:
      out += "file_does_not_exist";
      break;
    case e_error_read_text_file::alloc_failure:
      out += "alloc_failure";
      break;
    case e_error_read_text_file::open_file_failure:
      out += "open_file_failure";
      break;
    case e_error_read_text_file::incomplete_read:
      out += "incomplete_read";
      break;
  }
}

expect<std::unique_ptr<char[]>, e_error_read_text_file>
read_text_file( fs::path const& file, maybe<size_t&> o_size ) {
  if( !fs::exists( file ) )
    return e_error_read_text_file::file_does_not_exist;
  size_t file_size = fs::file_size( file );

  // Add one for null zero at the end, although the null zero
  // might not actually end up in that last byte if any windows
  // line endings are converted (and hence the length shortened).
  unique_ptr<char[]> buffer;
  try {
    buffer.reset( new char[file_size + 1] );
  } catch( std::bad_alloc const& ) {
    return e_error_read_text_file::alloc_failure;
  }

  FILE* fp = ::fopen( file.string().c_str(), "rb" );
  if( fp == nullptr )
    return e_error_read_text_file::open_file_failure;
  SCOPE_EXIT { ::fclose( fp ); };

  size_t binary_size_read = 0;

  char* p = buffer.get();

  constexpr size_t kStackBufferSize = 4096;
  char             stack_buffer[kStackBufferSize];
  // This algo probably won't work unless char has size 1.
  static_assert( sizeof( char ) == 1 );
  while( size_t read =
             std::fread( &stack_buffer, 1, 100, fp ) ) {
    binary_size_read += read;
    size_t start = 0;
    while( start < read ) {
      char b = stack_buffer[start++];
      if( b == 0x0d ) continue;
      *p++ = b;
    }
  }

  size_t c_string_size = p - buffer.get();

  if( binary_size_read != file_size )
    return e_error_read_text_file::incomplete_read;

  if( o_size ) *o_size = c_string_size;

  // Append null zero to the end.
  buffer.get()[c_string_size] = '\0';

  return buffer;
}

expect<std::string, e_error_read_text_file>
read_text_file_as_string( fs::path const& p ) {
  // We want NRVO here.
  expect<std::string, e_error_read_text_file> res = "";

  size_t size;
  auto   buf = read_text_file( p, size );
  if( !buf ) {
    res = buf.error();
  } else {
    char const* src = buf->get();
    string      s;
    s.resize( size, ' ' );
    memcpy( s.data(), src, size );
    res.emplace( std::move( s ) );
  }
  return res;
}

} // namespace base
