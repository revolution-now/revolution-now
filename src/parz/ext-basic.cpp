/****************************************************************
**ext-builtin.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser extension point for basic types.
*
*****************************************************************/
#include "ext-basic.hpp"

// parz
#include "combinator.hpp"
#include "promise.hpp"

// base
#include "base/conv.hpp"

// C++ standard library
#include <string>
#include <vector>

using namespace std;

namespace parz {

parser<int> parser_for( tag<int> ) {
  co_await eat_spaces();
  vector<char> chars   = co_await some( digit );
  string       num_str = string( chars.begin(), chars.end() );
  base::maybe<int> i   = base::stoi( num_str );
  if( !i ) co_await fail( "" );
  co_return *i;
}

parser<string> parser_for( tag<string> ) {
  co_await eat_spaces();
  vector<char> chars = co_await some( identifier_char );
  co_return string( chars.begin(), chars.end() );
}

} // namespace parz
