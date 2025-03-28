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

namespace cdr {

/****************************************************************
** error
*****************************************************************/
// This is the object that holds an error message upon a failure
// of `from_canonical`.
struct error {
  std::string const& what() const { return what_; }

  bool operator==( error const& ) const = default;

  operator std::string const&() const { return what_; }

 private:
  friend struct converter;

  explicit error( std::string_view what ) : what_( what ) {}

  template<typename Arg1, typename... Args>
  explicit error( std::string_view fmt_str, Arg1&& arg1,
                  Args&&... args )
    // TODO: replace with std::runtime_format when available.
    : what_( std::vformat(
          fmt_str, std::make_format_args( arg1, args... ) ) ) {}

  std::string what_;
};

inline void to_str( error const& o, std::string& out,
                    base::tag<error> ) {
  out += o.what();
}

template<typename T>
using result = base::expect<T, error>;

} // namespace cdr
