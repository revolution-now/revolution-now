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
  co_return co_await unwrap(
      base::stoi( co_await some( digit ) ) );
}

} // namespace parz
