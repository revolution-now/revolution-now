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

namespace {

table table_from_kv_pairs(
    vector<pair<string, value>>&& kv_pairs ) {
  table res;
  for( auto& [k, v] : kv_pairs )
    res.members.push_back(
        key_val{ k, make_unique<value>( std::move( v ) ) } );
  return res;
}

parz::parser<pair<string, value>> parse_kv() {
  string k = co_await parz::parse<string>();
  co_await parz::exact_char( ':' );
  value v = co_await parz::parse<value>();
  co_return pair{ k, std::move( v ) };
}

} // namespace

parz::parser<table> parser_for( parz::tag<table> ) {
  co_await parz::eat_spaces();
  co_await parz::exact_char( '{' );
  auto kv_pairs = co_await parz::repeated( parse_kv );
  co_await parz::eat_spaces();
  co_await parz::exact_char( '}' );
  co_return table_from_kv_pairs( std::move( kv_pairs ) );
}

parz::parser<value> parser_for( parz::tag<value> ) {
  co_return co_await parz::parse<value::base_type>();
}

parz::parser<doc> parser_for( parz::tag<doc> ) {
  vector<pair<string, value>> kvs =
      co_await parz::repeated( parse_kv );
  doc res;
  res.tbl = table_from_kv_pairs( std::move( kvs ) );
  co_return res;
}

} // namespace cl
