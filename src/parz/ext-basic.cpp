/****************************************************************
**ext-basic.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
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

using namespace std;

namespace parz {

using ::base::maybe;

namespace detail {

parser<int> parse_int_default() {
  int multiplier = bool( co_await try_{ chr( '-' ) } ) ? -1 : 1;
  co_return multiplier* co_await unwrap(
      base::stoi( co_await many1( digit ) ) );
}

parser<double> parse_double_default() {
  double multiplier =
      bool( co_await try_{ chr( '-' ) } ) ? -1 : 1;
  string ipart = co_await many( digit );
  co_await chr( '.' );
  string fpart = co_await many( digit );
  if( ipart.empty() && fpart.empty() )
    co_await fail( "expected double" );
  co_return multiplier* co_await unwrap(
      base::from_chars<double>( ipart + '.' + fpart ) );
}

} // namespace detail

} // namespace parz
