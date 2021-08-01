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
#include "combinator.hpp"
#include "ext.hpp"
#include "promise.hpp"

// base
#include "base/conv.hpp"

// C++ standard library
#include <string>
#include <vector>

namespace parz {

template<typename Lang>
struct IntParser {
  parser<int> operator()() const {
    co_return co_await unwrap(
        base::stoi( co_await many1( digit ) ) );
  }
};

template<typename Lang>
inline constexpr IntParser<Lang> int_parser{};

// Assume that this will work the same way for any language.
template<typename Lang>
parser<int> parser_for( lang<Lang>, tag<int> ) {
  return int_parser<Lang>();
}

} // namespace parz
