/****************************************************************
**thread-status.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-04.
*
* Description: Representation for Lua thread/coroutine status.
*
*****************************************************************/
#pragma once

// base
#include "base/to-str.hpp"

namespace lua {

/****************************************************************
** Thread Status
*****************************************************************/
// This is not the same as what is returned by Lua's coroutine.s-
// tatus function, which is derived from this plus some other in-
// formation. It is probably more useful to use coroutine_status
// further below which mirrors coroutine.status.
enum class [[nodiscard]] thread_status {
  ok,
  yield,
  err
};

/****************************************************************
** Resume Results
*****************************************************************/
// Same as above, but used when the err state is represented by a
// lua_expect being in an error state. In other words, this type
// is supposed to be used in a lua_expect<resume_status> or some-
// thing similar.
enum class [[nodiscard]] resume_status {
  ok,
  yield
};

void to_str( resume_status status, std::string& out,
             base::ADL_t );

// This is used to communicate the results from lua_resume.
struct [[nodiscard]] resume_result {
  resume_status status;
  int           nresults;

  bool operator==( resume_result const& ) const = default;
};

void to_str( resume_result result, std::string& out,
             base::ADL_t );

template<typename R>
struct [[nodiscard]] resume_result_with_value {
  resume_status status;
  R             value;

  friend void to_str( resume_result_with_value result,
                      std::string& out, base::ADL_t ) {
    using base::to_str; // two-step
    to_str( "{status=", out, base::ADL );
    to_str( result.status, out, base::ADL );
    to_str( ", value=", out, base::ADL );
    to_str( result.value, out, base::ADL );
    out += "}";
  }

  bool operator==( resume_result_with_value const& ) const =
      default;
};

template<>
struct [[nodiscard]] resume_result_with_value<void> {
  resume_status status;

  bool operator==( resume_result_with_value const& ) const =
      default;
};

/****************************************************************
** Coroutine Status
*****************************************************************/
// These will mirror the values that the coroutine.status func-
// tion can return. The only difference is that it does not in-
// clude the "running" option, since that wouldn't make sense
// given that only C++ functions will return this type, and those
// are never called directly from Lua.
enum class coroutine_status {
  // If the coroutine is suspended in a call to yield, or if it
  // has not started running yet.
  suspended,
  // If the coroutine is active but not running (that is, it has
  // resumed another coroutine or called into a C function that
  // produced this status).
  normal,
  // If the coroutine has finished its body function, or if it
  // has stopped with an error.
  dead
};

void to_str( coroutine_status status, std::string& out,
             base::ADL_t );

} // namespace lua
