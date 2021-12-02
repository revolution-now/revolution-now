/****************************************************************
**error.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-26.
*
* Description: Error checking/handling for OpenGL.
*
*****************************************************************/
#pragma once

// base
#include "base/error.hpp"

// C++ standard library
#include <type_traits>

// The use of a lambda and an RAII object to do the checking is a
// trick that is used to allow supporting calling both functions
// that return void and those that return values. Unfortunately,
// if-constexpr does not seem to be useful since it requires both
// branches to compile.
#define GL_CHECK( ... )                        \
  [&] {                                        \
    ::gl::detail::CheckErrorOnDestroy checker; \
    return __VA_ARGS__;                        \
  }()

namespace gl {

// Returns empty if no errors, otherwise a vector where each ele-
// ment is one error (not multiple lines of the same error).
std::vector<std::string> has_errors();

// Returns true if there were errors, after printing them to the
// console.
bool print_errors();

// Prints errors to console and then dies if any errors are
// found.
void check_errors();

namespace detail {

struct CheckErrorOnDestroy {
  ~CheckErrorOnDestroy() { check_errors(); }
};

} // namespace detail

} // namespace gl
