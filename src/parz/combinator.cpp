/****************************************************************
**combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser combinators.
*
*****************************************************************/
#include "combinator.hpp"

// parz
#include "parser.hpp"
#include "promise.hpp"

using namespace std;

namespace parz {

namespace {

bool is_digit( char c ) { return ( c >= '0' && c <= '9' ); }

bool is_lower( char c ) { return ( c >= 'a' && c <= 'z' ); }

bool is_upper( char c ) { return ( c >= 'A' && c <= 'Z' ); }

bool is_alpha( char c ) {
  return is_lower( c ) || is_upper( c );
}

bool is_blank( char c ) {
  return ( c == ' ' ) || ( c == '\n' ) || ( c == '\r' ) ||
         ( c == '\t' );
}

bool is_alphanum( char c ) {
  return is_digit( c ) || ( c >= 'a' && c <= 'z' ) ||
         ( c >= 'A' && c <= 'Z' );
}

bool is_identifier_char( char c ) {
  return is_alphanum( c ) || ( c == '_' );
}

bool is_leading_identifier_char( char c ) {
  return is_alpha( c ) || ( c == '_' );
}

} // namespace

parser<char> chr() { co_return co_await next_char{}; }

parser<> chr( char c ) {
  co_await pred( [c]( char c_ ) { return c == c_; } );
}

parser<char> lower() { return pred( is_lower ); }

parser<char> upper() { return pred( is_upper ); }

parser<char> alpha() { return pred( is_alpha ); }

parser<char> alphanum() { return pred( is_alphanum ); }

parser<> space() { co_await chr( ' ' ); }
parser<> crlf() { co_await one_of( "\r\n" ); }
parser<> tab() { co_await chr( '\t' ); }
parser<> blank() { co_await pred( is_blank ); }

parser<> blanks() { co_await many( blank ); }

parser<char> identifier_char() {
  co_return co_await pred( is_identifier_char );
}

parser<char> leading_identifier_char() {
  co_return co_await pred( is_leading_identifier_char );
}

parser<std::string> identifier() {
  co_return( co_await leading_identifier_char() +
             co_await many( identifier_char ) );
}

parser<char> digit() { return pred( is_digit ); }

parser<> str( string_view sv ) {
  for( char c : sv ) co_await chr( c );
}

parser<char> one_of( string_view sv ) {
  return pred( [sv]( char c ) {
    return sv.find_first_of( c ) != string_view::npos;
  } );
}

parser<char> not_of( string_view sv ) {
  return pred( [sv]( char c ) {
    return sv.find_first_of( c ) == string_view::npos;
  } );
}

parser<> eof() {
  result_t<char> c = co_await Try{ chr() };
  if( c.has_value() )
    // If we're here that means that there is more input in the
    // buffer, and thus that the `eof` parser (which requires
    // eof) has failed, meaning that the previous parsers did not
    // consume all of the input.
    co_await fail(
        "failed to parse all characters in input stream" );
}

parser<std::string> double_quoted_str() {
  return bracketed( '"', many( not_of, "\"\n\r" ), '"' );
}

parser<std::string> single_quoted_str() {
  return bracketed( '\'', many( not_of, "'\n\r" ), '\'' );
}

parser<std::string> quoted_str() {
  return first( double_quoted_str(), single_quoted_str() );
}

parser<char> ret( char c ) { co_return c; }

parser<string> ret_str( string_view s ) {
  co_return string( s );
}

} // namespace parz
