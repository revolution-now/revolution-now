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

// base
#include "base/conv.hpp"

// C++ standard library
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace parz;

using ::base::maybe;

namespace cl {

namespace {

parser<string> unquoted_newline_delimited_str() {
  char   c    = co_await not_of( ".[],{}\"'\n\r" );
  string rest = co_await many( not_of, "[],{}\"'\n\r" );
  co_return c + rest;
}

parser<> assignment() {
  co_await( ( blanks() >> chr( '=' ) ) |
            ( blanks() >> chr( ':' ) ) | one_of( " \n\r\t" ) );
}

parser<vector<key_val>> parse_kvs() {
  auto kvs = co_await many( []() -> parser<key_val> {
    co_await blanks();
    key_val kv = co_await parse<cl_lang, key_val>();
    co_await blanks();
    co_await try_ignore( chr( ',' ) );
    co_return kv;
  } );
  co_return kvs;
}

parser<string> dotted_identifier() {
  string         fst  = co_await identifier();
  vector<string> rest = co_await many( []() -> parser<string> {
    co_await chr( '.' );
    co_return co_await identifier();
  } );
  for( auto const& elem : rest ) {
    fst += '.';
    fst += elem;
  }
  co_return fst;
}

} // namespace

/****************************************************************
** string_val
*****************************************************************/
parser<string_val> parser_for( lang<cl_lang>, tag<string_val> ) {
  return construct<string_val>(
      unquoted_newline_delimited_str() | quoted_str() );
}

/****************************************************************
** number
*****************************************************************/
parser<number> parser_for( lang<cl_lang>, tag<number> ) {
  bool   neg = bool( co_await Try{ chr( '-' ) } );
  string num = co_await many1_L( digit() | chr( '.' ) );
  if( num.find_first_not_of( '.' ) == string::npos )
    co_await fail( "expected number" );
  if( neg ) num = '-' + num;
  if( num.find_first_of( '.' ) != string::npos )
    co_return co_await unwrap( base::from_chars<double>( num ) );
  co_return co_await unwrap(
      base::from_chars<int>( num, /*base=*/10 ) );
}

/****************************************************************
** key_val
*****************************************************************/
parser<key_val> parser_for( lang<cl_lang>, tag<key_val> ) {
  return construct<key_val>(
      blanks() >> dotted_identifier(),
      assignment() >> blanks() >> parse<cl_lang, value>() );
}

/****************************************************************
** table
*****************************************************************/
parser<table> parser_for( lang<cl_lang>, tag<table> ) {
  co_await( blanks() >> chr( '{' ) );
  auto tbl = co_await parse_kvs();
  co_await( blanks() >> chr( '}' ) );
  co_return tbl;
}

/****************************************************************
** list
*****************************************************************/
parser<list> parser_for( lang<cl_lang>, tag<list> ) {
  co_await( blanks() >> chr( '[' ) );
  auto lst = co_await many( []() -> parser<value> {
    co_await blanks();
    value v = co_await parse<cl_lang, value>();
    co_await blanks();
    co_await chr( ',' );
    co_return v;
  } );
  co_await( blanks() >> chr( ']' ) );
  co_return lst;
}

/****************************************************************
** doc
*****************************************************************/
parser<doc> parser_for( lang<cl_lang>, tag<doc> ) {
  co_return co_await unwrap( co_await parz::apply(
      doc::create, seq_first( parse_kvs(), blanks() ) ) );
}

} // namespace cl
