/****************************************************************
**co-time.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-21.
*
* Description: Time-related wait and coroutine helpers.
*
*****************************************************************/
#include "co-time.hpp"

// Revolution Now
#include "frame.hpp"

using namespace std;

namespace rn {

wait<std::chrono::microseconds> co_await_transform(
    chrono::microseconds us ) {
  return wait_for_duration( us );
}

} // namespace rn
