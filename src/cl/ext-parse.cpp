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

namespace cl {

parz::parser<key_val> parser_for( parz::tag<key_val> ) {
  string k = co_await parz::parse<string>();
  co_await parz::exact_char( ':' );
  value v = co_await parz::parse<value>();
  co_return key_val{ k, make_unique<value>( std::move( v ) ) };
}

parz::parser<table> parser_for( parz::tag<table> ) {
  co_await parz::eat_spaces();
  co_await parz::exact_char( '{' );
  auto kv_pairs = co_await parz::repeated(
      [] { return parz::parse<key_val>(); } );
  co_await parz::eat_spaces();
  co_await parz::exact_char( '}' );
  co_return table{ std::move( kv_pairs ) };
}

parz::parser<value> parser_for( parz::tag<value> ) {
  co_return co_await parz::parse<value::base_type>();
}

parz::parser<doc> parser_for( parz::tag<doc> ) {
  co_return doc{ co_await parz::repeated(
      [] { return parz::parse<key_val>(); } ) };
}

} // namespace cl
