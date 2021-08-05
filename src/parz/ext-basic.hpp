/****************************************************************
**ext-basic.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser extension point for basic types.
*
*****************************************************************/
#pragma once

// parz
#include "ext.hpp"

namespace parz {

namespace detail {
parser<int>    parse_int_default();
parser<double> parse_double_default();
} // namespace detail

// Assume that this will work the same way for any language.
template<typename Lang>
parser<int> parser_for( lang<Lang>, tag<int> ) {
  return detail::parse_int_default();
}

template<typename Lang>
parser<double> parser_for( lang<Lang>, tag<double> ) {
  return detail::parse_double_default();
}

} // namespace parz
