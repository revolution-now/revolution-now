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

// Revolution Now
#include "errors.hpp"
#include "logging.hpp"

namespace rn {

namespace {} // namespace

namespace detail {

void run_func_noexcept(
    char const* file, int line,
    tl::function_ref<void()> func ) noexcept {
  try {
    func();
  } catch( ::rn::exception_with_bt const& e ) {
    lg.critical(
        "exception_with_bt thrown during scope-exit function in "
        "{}:{}: {}",
        file, line, e.what() );
  } catch( std::exception const& e ) {
    lg.critical(
        "std::exception thrown during scope-exit function in "
        "{}:{}: {}",
        file, line, e.what() );
  } catch( ... ) {
    lg.critical(
        "unknown exception thrown during scope-exit function in "
        "{}:{}",
        file, line );
  }
}

} // namespace detail

} // namespace rn
