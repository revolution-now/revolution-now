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
#include "base/fmt.hpp"
#include "base/to-str.hpp"

namespace lua {

/****************************************************************
** Thread Status
*****************************************************************/
// This is not the same as what is returned by Lua's coroutine.s-
// tatus function, which is derived from this plus some other in-
// formation. It is probably more useful to use coroutine_status
// further below which mirrors coroutine.status.
enum class thread_status { ok, yield, err };

/****************************************************************
** Resume Results
*****************************************************************/
// Same as above, but used when the err state is represented by a
// lua_expect being in an error state. In other words, this type
// is supposed to be used in a lua_expect<resume_status> or some-
// thing similar.
enum class resume_status { ok, yield };

void to_str( resume_status status, std::string& out );

// This is used to communicate the results from lua_resume.
struct resume_result {
  resume_status status;
  int           nresults;

  bool operator==( resume_result const& ) const = default;
};

void to_str( resume_result result, std::string& out );

template<typename R>
struct resume_result_with_value {
  resume_status status;
  R             value;

  friend void to_str( resume_result_with_value result,
                      std::string&             out ) {
    using base::to_str; // two-step
    to_str( "{status=", out );
    to_str( result.status, out );
    to_str( ", value=", out );
    to_str( result.value, out );
    out += "}";
  }

  bool operator==( resume_result_with_value const& ) const =
      default;
};

template<>
struct resume_result_with_value<void> {
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

void to_str( coroutine_status status, std::string& out );

} // namespace lua

TOSTR_TO_FMT( ::lua::resume_status );
TOSTR_TO_FMT( ::lua::coroutine_status );
TOSTR_TO_FMT( ::lua::resume_result );
DEFINE_FORMAT_T( ( R ), (::lua::resume_result_with_value<R>),
                 "{}", base::to_str( o ) );
