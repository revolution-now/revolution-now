/****************************************************************
**error.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Types for error handling in Cdr conversion.
*
*****************************************************************/
#pragma once

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/to-str.hpp"

// C++ standard library
#include <string>
#include <string_view>
#include <vector>

namespace cdr {

/****************************************************************
** error_frame
*****************************************************************/
// Used to hold one stack frame. These frames are accumulated
// when unwinding the stack during an error in from_canonical.
struct error_frame {
  // Name of the type we were trying to produce when the error
  // happened.
  std::string_view type_name;
};

/****************************************************************
** error
*****************************************************************/
// This is the object that holds an error message upon a failure
// of `from_canonical`. It also holds an accumulated stack trace.
//
// These should only be created directly in the unit tests; in
// normal code (i.e. implementations of from_canonical) you
// should be using the cdr::converter to create error objects.
struct error {
  template<typename... Args>
  explicit error( std::string_view fmt_str, Args&&... args )
    : what( fmt::format( fmt::runtime( fmt_str ),
                         std::forward<Args>( args )... ) ) {}

  // This ignores the frame upon comparison, which makes unit
  // testing easier.
  bool operator==( error const& rhs ) const;

  std::string              what;
  std::vector<error_frame> frames;
};

void to_str( error const& o, std::string& out, base::ADL_t );

template<typename T>
using result = base::expect<T, error>;

} // namespace cdr
