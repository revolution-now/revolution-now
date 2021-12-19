/****************************************************************
**thread-status.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-16.
*
* Description: Representation for Lua thread/coroutine status.
*
*****************************************************************/
#include "thread-status.hpp"

// base
#include "base/fmt.hpp"

using namespace std;

namespace lua {

void to_str( resume_status status, std::string& out,
             base::ADL_t ) {
  switch( status ) {
    case resume_status::ok: out += "ok"; break;
    case resume_status::yield: out += "yield"; break;
  }
}

void to_str( coroutine_status status, std::string& out,
             base::ADL_t ) {
  switch( status ) {
    case coroutine_status::suspended: out += "suspended"; break;
    case coroutine_status::normal: out += "normal"; break;
    case coroutine_status::dead: out += "dead"; break;
  }
}

void to_str( resume_result result, std::string& out,
             base::ADL_t ) {
  out += fmt::format( "resume_result{{status={}, nresults={}}}",
                      result.status, result.nresults );
}

} // namespace lua
