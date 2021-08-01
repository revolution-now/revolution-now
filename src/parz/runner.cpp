/****************************************************************
**runner.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Runners of parsers.
*
*****************************************************************/
#include "runner.hpp"

// base
#include "base/error.hpp"
#include "base/io.hpp"

using namespace std;

namespace parz {

ErrorPos ErrorPos::from_index( string_view in, int idx ) {
  CHECK_LT( idx, int( in.size() ) );
  ErrorPos res{ 1, 1 };
  for( int i = 0; i < idx; ++i ) {
    ++res.col;
    if( in[i] == '\n' ) {
      ++res.line;
      res.col = 1;
    }
  }
  return res;
}

namespace internal {

base::expect<string, error> read_file_into_buffer(
    string_view filename ) {
  auto buffer = base::read_text_file_as_string( filename );
  using namespace base;
  if( !buffer )
    return base::error_read_text_file_msg( filename,
                                           buffer.error() );
  return *buffer;
}

} // namespace internal

} // namespace parz
