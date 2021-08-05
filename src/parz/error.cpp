/****************************************************************
**error.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Error type for parser.
*
*****************************************************************/
#include "error.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace parz {

ErrorPos ErrorPos::from_index( string_view in, int idx ) {
  CHECK_LE( idx, int( in.size() ) );
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

} // namespace parz
