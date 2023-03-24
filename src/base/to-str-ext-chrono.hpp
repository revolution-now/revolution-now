/****************************************************************
**to-str-ext-chrono.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-23.
*
* Description: String formatting for std::chrono stuff.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "to-str.hpp"

// base-util
#include "base-util/string.hpp" // FIXME

// C++ standard library
#include <chrono>

// FIXME: when we create a to-str library we should split all of
// the std namespace to_str overloads into separate headers, and
// this one should become to_str/chrono.

namespace base {

/****************************************************************
** Helpers.
*****************************************************************/
std::string format_duration( std::chrono::nanoseconds ns );

/****************************************************************
** to_str overloads.
*****************************************************************/
template<typename... Ts>
void to_str( std::chrono::time_point<Ts...> const& o,
             std::string&                          out, ADL_t ) {
  out += "\"";
  out += util::to_string( o );
  out += "\"";
};

// {fmt} formatter for formatting duration.
template<class Rep, class Period>
void to_str( std::chrono::duration<Rep, Period> const& o,
             std::string& out, ADL_t ) {
  out += format_duration( o );
};

} // namespace base
