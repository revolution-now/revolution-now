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
#include "parz/ext-basic.hpp"
#include "parz/ext-variant.hpp"
#include "parz/promise.hpp"

// C++ standard library
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace parz;

namespace cl {

parser<key_val> parser_for( tag<key_val> ) {
  string k = co_await parse<string>();
  co_await exact_char( ':' );
  value v = co_await parse<value::base_type>();
  co_return key_val{ k, make_unique<value>( std::move( v ) ) };
}

parser<table> parser_for( tag<table> ) {
  co_await eat_spaces();
  co_await exact_char( '{' );
  auto kv_pairs = co_await repeated_parse<key_val>();
  co_await eat_spaces();
  co_await exact_char( '}' );
  co_return table{ std::move( kv_pairs ) };
}

parser<doc> parser_for( tag<doc> ) {
  co_return doc{ co_await repeated_parse<key_val>() };
}

} // namespace cl
