/****************************************************************
**scope-exit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-09.
*
* Description: Scope Guard.
*
*****************************************************************/
#include "scope-exit.hpp"

// C++ standard library.
#include <iostream>

namespace base {

namespace detail {

void run_func_noexcept(
    char const* file, int line,
    tl::function_ref<void()> func ) noexcept {
  try {
    func();
  } catch( std::exception const& e ) {
    std::cerr << "[ERROR] std::exception thrown during "
                 "scope-exit function in "
              << file << ":" << line << ": " << e.what();
  } catch( ... ) {
    std::cerr << "[ERROR] unknown exception thrown during "
                 "scope-exit function in "
              << file << ":" << line;
  }
}

} // namespace detail

} // namespace base
