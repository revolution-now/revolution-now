/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Extension point for parsing user types.
*
*****************************************************************/
#pragma once

// parz
#include "parser.hpp"

/****************************************************************
** Extension point.
*****************************************************************/
namespace parz {

template<typename T>
struct tag {};

template<typename T>
parser<T> parse() {
  return parser_for( tag<T>{} );
}

} // namespace parz
