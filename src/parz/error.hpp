/****************************************************************
**error.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Error type for parser.
*
*****************************************************************/
#pragma once

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace parz {

/****************************************************************
** Parser Error/Result
*****************************************************************/
struct error {
  explicit error() : msg_( "" ) {}
  explicit error( std::string_view m ) : msg_( m ) {}

  template<typename... Args>
  explicit error( std::string_view m, Args&&... args )
    : msg_( fmt::format( m, std::forward<Args>( args )... ) ) {}

  std::string const& what() const { return msg_; }

  operator std::string() const { return msg_; }

private:
  std::string msg_;
};

template<typename T>
using result = base::expect<T, error>;

} // namespace parz

DEFINE_FORMAT( parz::error, "{}", o.what() );