/****************************************************************
**io.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-12.
*
* Description: Input/Output.
*
*****************************************************************/
#include "io.hpp"

// C++ standard library.
#include <fstream>

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
// Read a text file into a string in its entirety.
expect<string> read_file_as_string( fs::path const& p ) {
  ifstream in( p, ifstream::in );
  if( !in.good() )
    return UNEXPECTED( "failed to open file {}.", p );

  expect<string> res;

#ifdef _WIN32
  // This is slower, but works correctly on all platforms for
  // text files I hear.
  ostringstream oss;
  oss << in.rdbuf();
  res = std::move( oss.str() );
#else
  // This is apparently the fastest method for large files.
  in.seekg( 0, ios::end );
  size_t size = in.tellg();
  res->resize( size );
  in.seekg( 0, ios::beg );
  in.read( &( ( *res )[0] ), size );
#endif

  return res;
}

/****************************************************************
** Testing
*****************************************************************/
void test_io() {}

} // namespace rn
