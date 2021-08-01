/****************************************************************
**ext-std.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-31.
*
* Description: Parser extensions for std types.
*
*****************************************************************/
#pragma once

// parz
#include "ext.hpp"

// C++ standard library
#include <memory>

namespace parz {

/****************************************************************
** unique_ptr
*****************************************************************/
// We have this struct helper to work around a weird clang issue
// where it doesn't like function templates that are coroutines.
template<typename Lang, typename T>
struct ParserForUniquePtr {
  parser<std::unique_ptr<T>> operator()() const {
    co_return std::make_unique<T>( co_await parse<Lang, T>() );
  }
};

template<typename Lang, typename T>
inline constexpr ParserForUniquePtr<Lang, T> unique_ptr_parser{};

template<typename Lang, typename T>
parser<std::unique_ptr<T>> parser_for(
    lang<Lang>, tag<std::unique_ptr<T>> ) {
  return unique_ptr_parser<Lang, T>();
}

} // namespace parz
