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

namespace lua {

// This is not the same as what is returned by Lua's corou-
// tine.status function, which is derived from this plus some
// other information.
enum class thread_status { ok, yield, err };

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
};

template<>
struct resume_result_with_value<void> {
  resume_status status;
};

} // namespace lua

TOSTR_TO_FMT( ::lua::resume_status );
TOSTR_TO_FMT( ::lua::resume_result );
