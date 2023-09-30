/****************************************************************
**latch.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-30.
*
* Description: A "latch" synchronization primitive for coros.
*
*****************************************************************/
#include "latch.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn::co {

/****************************************************************
** latch
*****************************************************************/
latch::latch( int counter ) : counter_( counter ) {
  // It is not necessarily the case that the number of coroutines
  // waiting will be equal to the initial counter value (could be
  // more or less), but in the usual scenario they are equal.
  promises_.reserve( counter );
}

void latch::count_down( int n ) {
  CHECK_GE( n, 0 );
  if( n == 0 ) return;
  CHECK_GE( counter_, n );
  counter_ -= n;
  if( counter_ > 0 ) return;
  // We've hit bottom, so notify everyone who is listening.
  for( wait_promise<>* const p : promises_ )
    p->set_value_emplace();
}

bool latch::try_wait() const { return counter_ == 0; }

wait<> latch::Wait() {
  if( counter_ == 0 ) co_return;
  wait_promise<> p;
  promises_.push_back( &p );
  co_await p.wait();
}

wait<> latch::arrive_and_wait( int n ) {
  count_down( n );
  co_await Wait();
}

} // namespace rn::co
