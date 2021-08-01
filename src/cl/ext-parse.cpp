/****************************************************************
**ext-parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: parz parser extension for cl model.
*
*****************************************************************/
#include "ext-parse.hpp"

// parz
#include "parz/combinator.hpp"
#include "parz/ext-base.hpp"
#include "parz/ext-basic.hpp"
#include "parz/ext-std.hpp"
#include "parz/promise.hpp"

// C++ standard library
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace parz;

namespace cl {

parser<string_val> parser_for( tag<string_val> ) {
  return construct<string_val>( quoted_string() | identifier() );
}

parser<key_val> parser_for( tag<key_val> ) {
  return construct<key_val>(
      spaces() >> identifier(),
      chr( ':' ) >> spaces() >> parse<value>() );
}

parser<table> parser_for( tag<table> ) {
  co_await( spaces() >> chr( '{' ) );
  auto tbl = table{ co_await repeated_parse<key_val>() };
  co_await( spaces() >> chr( '}' ) );
  co_return tbl;
}

parser<doc> parser_for( tag<doc> ) {
  return construct<doc>( repeated_parse<key_val>() );
}

} // namespace cl
