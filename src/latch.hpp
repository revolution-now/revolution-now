/****************************************************************
**latch.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-30.
*
* Description: A "latch" synchronization primitive for coros.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

/****************************************************************
** latch
*****************************************************************/
// In analogy to std::latch for threads, the co::latch is a down-
// ward counter which can be used to synchronize coroutines. The
// value of the counter is initialized on creation. Coros may
// block on the latch until the counter is decremented to zero.
// There is no possibility to increase or reset the counter,
// which makes the latch a single-use barrier. The number of
// coros that can be waiting on the counter to hit zero is unre-
// lated to the value of the counter itself, i.e. it can be an
// arbitrary number. This interface attempts to mirror the API of
// std::latch.
struct latch {
  latch( int counter = 1 );

  // Decrements the counter in a non-blocking manner
  void count_down( int n = 1 );

  // Tests if the internal counter equals zero
  bool try_wait() const;

  // Blocks until the counter reaches zero.
  wait<> Wait();

  // Decrements the counter and blocks until it reaches zero.
  wait<> arrive_and_wait( int n = 1 );

  int counter() const { return counter_; }

 private:
  int counter_ = 0;

  // These refer to the promises held as local variables in each
  // coroutine that is currently waiting. When the counter hits
  // zero, all of these promises will be fulfilled.
  std::vector<wait_promise<>*> promises_;
};

} // namespace rn::co
