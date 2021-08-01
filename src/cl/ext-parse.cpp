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

parser<string_val> parser_for( lang<cl_lang>, tag<string_val> ) {
  return construct<string_val>( quoted_str() | identifier() );
}

parser<key_val> parser_for( lang<cl_lang>, tag<key_val> ) {
  return construct<key_val>(
      blanks() >> identifier(),
      chr( ':' ) >> blanks() >> parse<cl_lang, value>() );
}

parser<table> parser_for( lang<cl_lang>, tag<table> ) {
  co_await( blanks() >> chr( '{' ) );
  auto tbl = table{ co_await many_type<cl_lang, key_val>() };
  co_await( blanks() >> chr( '}' ) );
  co_return tbl;
}

parser<doc> parser_for( lang<cl_lang>, tag<doc> ) {
  return construct<doc>(
      seq_first( many_type<cl_lang, key_val>(), blanks() ) );
}

} // namespace cl
