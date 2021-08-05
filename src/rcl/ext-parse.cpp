/****************************************************************
**ext-parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: parz parser extension for rcl model.
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
#include "base/lambda.hpp"

// Abseil
#include "absl/strings/str_join.h"

// C++ standard library
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace parz;

using ::base::maybe;

namespace rcl {

namespace {

auto joiner( string_view sv ) {
  return [s = string( sv )]( vector<string> const& v ) {
    return absl::StrJoin( v, s );
  };
}

parser<string> unquoted_newline_delimited_str() {
  string res = co_await cat( not_of( "=:.[],{}\"'\n\r" ),
                             many( not_of, "=:[],{}\"'\n\r" ) );
  if( res == "true" || res == "false" ) co_await fail();
  co_return res;
}

parser<> assignment() {
  co_await( ( blanks() >> chr( '=' ) ) |
            ( blanks() >> chr( ':' ) ) | one_of( " \t" ) );
}

parser<string> dotted_identifier() {
  return fmap( joiner( "." ),
               interleave( identifier, L0( chr( '.' ) ) ) );
}

template<typename T>
parser<vector<T>> parse_vec() {
  return interleave_last( L0( blanks() >> parse<Rcl, T>() ),
                          L0( blanks() >> chr( ',' ) ),
                          /*sep_required=*/false );
}

template<typename T>
parser<vector<T>> bracketed_vec( char l, char r ) {
  return bracketed( blanks() >> chr( l ), parse_vec<T>(),
                    blanks() >> chr( r ) );
}

} // namespace

/****************************************************************
** string_val
*****************************************************************/
parser<string_val> parser_for( lang<Rcl>, tag<string_val> ) {
  return emplace<string_val>( unquoted_newline_delimited_str() |
                              quoted_str() );
}

/****************************************************************
** boolean
*****************************************************************/
parser<boolean> parser_for( lang<Rcl>, tag<boolean> ) {
  return ( str( "true" ) >> ret( boolean{ true } ) ) |
         ( str( "false" ) >> ret( boolean{ false } ) );
}

/****************************************************************
** number
*****************************************************************/
parser<number> parser_for( lang<Rcl>, tag<number> ) {
  return fmap( L( number{ _ } ), parse<Rcl, double>() ) |
         fmap( L( number{ _ } ), parse<Rcl, int>() );
}

/****************************************************************
** key_val
*****************************************************************/
parser<key_val> parser_for( lang<Rcl>, tag<key_val> ) {
  return emplace<key_val>(
      blanks() >> dotted_identifier(),
      assignment() >> blanks() >> parse<Rcl, value>() );
}

/****************************************************************
** table
*****************************************************************/
parser<table> parser_for( lang<Rcl>, tag<table> ) {
  co_return co_await bracketed_vec<key_val>( '{', '}' );
}

/****************************************************************
** list
*****************************************************************/
parser<list> parser_for( lang<Rcl>, tag<list> ) {
  co_return co_await bracketed_vec<value>( '[', ']' );
}

/****************************************************************
** doc
*****************************************************************/
parser<rawdoc> parser_for( lang<Rcl>, tag<rawdoc> ) {
  // The blanks must be inside the diagnose.
  return diagnose( emplace<rawdoc>( parse_vec<key_val>() )
                       << blanks(),
                   parse<Rcl, key_val>() );
}

} // namespace rcl
