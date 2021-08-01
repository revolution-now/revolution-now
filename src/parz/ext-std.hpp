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
template<typename T>
struct ParserForUniquePtr {
  parser<std::unique_ptr<T>> operator()() const {
    co_return std::make_unique<T>( co_await parse<T>() );
  }
};

template<typename T>
inline constexpr ParserForUniquePtr<T> unique_ptr_parser{};

template<typename T>
parser<std::unique_ptr<T>> parser_for(
    tag<std::unique_ptr<T>> ) {
  return unique_ptr_parser<T>();
}

} // namespace parz
