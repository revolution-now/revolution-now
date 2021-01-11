/****************************************************************
**flat-queue.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-04.
*
* Description: Unit tests for the flat-queue module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "flat-deque.hpp"
#include "flat-queue.hpp"
#include "src/rand.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <queue>

namespace {

namespace rl = ::base::rl;

using namespace std;
using namespace rn;

TEST_CASE( "[flat-queue] initialization" ) {
  flat_queue<int> q;

  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );

  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-deque] initialization" ) {
  flat_deque<int> q;

  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );

  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] initialization init-list" ) {
  flat_queue<int> q = { 1, 2, 3, 4 };

  REQUIRE( q.size() == 4 );
  REQUIRE( q.front() == 1 );
  q.pop();
  REQUIRE( q.front() == 2 );
  q.pop();
  REQUIRE( q.front() == 3 );
  q.pop();
  REQUIRE( q.front() == 4 );
  q.pop();

  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] initialization vector" ) {
  vector<int>     v{ 1, 2, 3, 4 };
  flat_queue<int> q( std::move( v ) );
  // Make sure that it moved the vector into its own storage in-
  // stead of copying.
  REQUIRE( v.empty() );

  REQUIRE( q.size() == 4 );
  REQUIRE( q.front() == 1 );
  q.pop();
  REQUIRE( q.front() == 2 );
  q.pop();
  REQUIRE( q.front() == 3 );
  q.pop();
  REQUIRE( q.front() == 4 );
  q.pop();

  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] push pop small" ) {
  flat_queue<int> q;

  q.push( 5 );
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 5 );

  q.pop();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );

  q.push( 5 );
  q.push( 6 );
  REQUIRE( q.size() == 2 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 5 );

  q.pop();
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front() == 6 );

  q.pop();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] to_string" ) {
  flat_queue<int> q;

  REQUIRE( q.to_string() == "[front:]" );
  REQUIRE( q.to_string( 0 ) == "[front:]" );

  q.push( 5 );
  REQUIRE( q.to_string() == "[front:5]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:5]" );
  REQUIRE( q.to_string( 2 ) == "[front:5]" );

  q.pop();
  REQUIRE( q.to_string() == "[front:]" );
  REQUIRE( q.to_string( 0 ) == "[front:]" );

  q.push( 5 );
  q.push( 6 );
  REQUIRE( q.to_string() == "[front:5,6]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:5...]" );
  REQUIRE( q.to_string( 2 ) == "[front:5,6]" );
  REQUIRE( q.to_string( 3 ) == "[front:5,6]" );

  q.push( 7 );
  q.push( 8 );
  q.push( 9 );
  REQUIRE( q.to_string() == "[front:5,6,7,8,9]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:5...]" );
  REQUIRE( q.to_string( 2 ) == "[front:5,6...]" );
  REQUIRE( q.to_string( 3 ) == "[front:5,6,7...]" );
  REQUIRE( q.to_string( 4 ) == "[front:5,6,7,8...]" );
  REQUIRE( q.to_string( 5 ) == "[front:5,6,7,8,9]" );
  REQUIRE( q.to_string( 6 ) == "[front:5,6,7,8,9]" );
}

TEST_CASE( "[flat-deque] to_string" ) {
  flat_deque<int> q;

  REQUIRE( q.to_string() == "[front:]" );
  REQUIRE( q.to_string( 0 ) == "[front:]" );

  q.push_back( 5 );
  REQUIRE( q.to_string() == "[front:5]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:5]" );
  REQUIRE( q.to_string( 2 ) == "[front:5]" );

  q.pop_front();
  REQUIRE( q.to_string() == "[front:]" );
  REQUIRE( q.to_string( 0 ) == "[front:]" );

  q.push_back( 5 );
  q.push_back( 6 );
  REQUIRE( q.to_string() == "[front:5,6]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:5...]" );
  REQUIRE( q.to_string( 2 ) == "[front:5,6]" );
  REQUIRE( q.to_string( 3 ) == "[front:5,6]" );

  q.push_front( 7 );
  q.push_front( 8 );
  q.push_front( 9 );
  REQUIRE( q.to_string() == "[front:9,8,7,5,6]" );
  REQUIRE( q.to_string( 0 ) == "[front:...]" );
  REQUIRE( q.to_string( 1 ) == "[front:9...]" );
  REQUIRE( q.to_string( 2 ) == "[front:9,8...]" );
  REQUIRE( q.to_string( 3 ) == "[front:9,8,7...]" );
  REQUIRE( q.to_string( 4 ) == "[front:9,8,7,5...]" );
  REQUIRE( q.to_string( 5 ) == "[front:9,8,7,5,6]" );
  REQUIRE( q.to_string( 6 ) == "[front:9,8,7,5,6]" );
}

TEST_CASE( "[flat-deque] push pop small" ) {
  flat_deque<int> q;

  q.push_back( 5 );
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 5 );

  q.pop_front();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );

  q.push_back( 5 );
  q.push_back( 6 );
  REQUIRE( q.size() == 2 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 5 );

  q.pop_front();
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front() == 6 );

  q.pop_front();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );

  q.push_front( 5 );
  q.push_front( 6 );
  REQUIRE( q.size() == 2 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 6 );

  q.pop_front();
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front() == 5 );

  q.pop_front();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );

  q.push_front( 5 );
  q.push_front( 6 );
  REQUIRE( q.size() == 2 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front() == 6 );

  q.pop_back();
  REQUIRE( q.size() == 1 );
  REQUIRE( !q.empty() );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front() == 6 );

  q.pop_front();
  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] min size" ) {
  flat_queue<int> q;

  int const push_size = 10 * 5;
  REQUIRE( push_size > 10 );

  vector<int> pushed;
  for( int i = 0; i < push_size; ++i ) {
    auto value = i * 2;
    pushed.push_back( value );
    q.push( value );
    REQUIRE( q.size() == i + 1 );
    REQUIRE( !q.empty() );
    REQUIRE( q.front() == 0 );
  }

  for( int i = push_size - 1; i >= 0; --i ) {
    REQUIRE( q.size() == i + 1 );
    REQUIRE( !q.empty() );
    REQUIRE( q.front() == pushed.front() );
    pushed.erase( pushed.begin(), pushed.begin() + 1 );
    q.pop();
  }

  REQUIRE( q.size() == 0 );
  REQUIRE( q.empty() );
  REQUIRE( !q.front().has_value() );
}

TEST_CASE( "[flat-queue] equality" ) {
  flat_queue<int> q1;
  flat_queue<int> q2;

  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );

  q1.push( 2 );
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );
  q2.push( 2 );
  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );

  q1.push( 3 );
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );
  q2.push( 3 );
  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );

  q1.pop();
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );
  q2.pop();
  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );
  q1.pop();
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );
  q2.pop();
  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );

  REQUIRE( q1.size() == 0 );
  REQUIRE( q2.size() == 0 );

  for( int i = 0;
       i < k_flat_queue_reallocation_size_default * 10; ++i )
    q1.push( 5 );
  q1.push( 4 );
  q1.push( 3 );
  q1.push( 2 );
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );

  q2.push( 4 );
  q2.push( 3 );
  q2.push( 2 );
  REQUIRE( q1 != q2 );
  REQUIRE( q2 != q1 );
  for( int i = 0;
       i < k_flat_queue_reallocation_size_default * 10; ++i )
    q1.pop();

  REQUIRE( q1 == q2 );
  REQUIRE( q2 == q1 );
}

TEST_CASE( "[flat-deque] equality" ) {
  flat_deque<int> q1;
  flat_deque<int> q2;

  REQUIRE( q1 == q2 );

  q1.push_front( 4 );
  REQUIRE( q1 != q2 );
  q1.push_front( 5 );
  REQUIRE( q1 != q2 );
  q1.push_front( 6 );
  REQUIRE( q1 != q2 );
  q2.push_back( 6 );
  REQUIRE( q1 != q2 );
  q2.push_back( 5 );
  REQUIRE( q1 != q2 );
  q2.push_back( 4 );
  REQUIRE( q1 == q2 );

  q1.push_back( 1 );
  REQUIRE( q1 != q2 );
  q1.push_back( 2 );
  REQUIRE( q1 != q2 );
  q1.push_back( 3 );
  REQUIRE( q1 != q2 );
  q2.push_back( 1 );
  REQUIRE( q1 != q2 );
  q2.push_back( 2 );
  REQUIRE( q1 != q2 );
  q2.push_back( 3 );
  REQUIRE( q1 == q2 );
}

TEST_CASE( "[flat-queue] reallocation size" ) {
  flat_queue<int> q;

  int const push_size =
      k_flat_queue_reallocation_size_default * 2;
  REQUIRE( push_size > k_flat_queue_reallocation_size_default );

  for( int i = 0; i < push_size; ++i ) q.push( i );

  REQUIRE( q.size() == push_size );

  for( int i = 0; i < k_flat_queue_reallocation_size_default - 1;
       ++i )
    q.pop();

  REQUIRE( q.size() ==
           k_flat_queue_reallocation_size_default + 1 );

  for( int i = 0; i < k_flat_queue_reallocation_size_default + 1;
       ++i )
    q.pop();

  REQUIRE( q.size() == 0 );
}

TEST_CASE( "[flat-queue] non-copyable, non-def-constructible" ) {
  struct A {
    A() = delete;

    A( A const& ) = delete;
    A( A&& )      = default;

    A& operator=( A const& ) = delete;
    A& operator=( A&& ) = default;

    A( int x ) : x_( x ) {}
    int x_;
  };
  flat_queue<A> q;
  q.push_emplace( A{ 5 } );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front().value().x_ == 5 );
  q.pop();
  REQUIRE( q.size() == 0 );
}

TEST_CASE( "[flat-deque] non-copyable" ) {
  struct A {
    A() = default;

    A( A const& ) = delete;
    A( A&& )      = default;

    A& operator=( A const& ) = delete;
    A& operator=( A&& ) = default;

    A( int x ) : x_( x ) {}
    int x_;
  };
  flat_deque<A> q;
  q.push_back_emplace( A{ 5 } );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front().has_value() );
  REQUIRE( q.front().value().x_ == 5 );
  q.pop_front();
  REQUIRE( q.size() == 0 );
}

// This test really puts the flat_queue to the test. It will for
// three unique configurations of reallocation_size randomly push
// and pop thousands of random elements into the flat_queue and
// an std::queue and compare them at every turn.
TEST_CASE( "[flat-queue] std::queue comparison" ) {
  auto realloc_size = GENERATE( 250, 1000, 10000 );

  flat_queue<int> q( realloc_size );
  std::queue<int> sq;

  // Catch2 takes its seed on the command line; from this seed
  // we generate some random numbers to reseed our own random
  // gener- ator to get predictable results based on Catch2's
  // seed.
  uint32_t sub_seed =
      GENERATE( take( 1, random( 0, 1000000 ) ) );
  INFO( fmt::format( "sub_seed: {}", sub_seed ) );
  rng::reseed( sub_seed );

  SECTION( "roughly equal push and pop" ) {
    auto bs = rl::generate_n( L0( rng::flip_coin() ),
                              realloc_size * 3 )
                  .to_vector();
    auto is = rl::ints( 0, realloc_size * 3 ).to_vector();
    REQUIRE( is.size() == bs.size() );

    for( auto [action, n] : rl::zip( bs, is ) ) {
      if( action ) {
        q.push( n );
        sq.push( n );
      } else if( !q.empty() && !sq.empty() ) {
        q.pop();
        sq.pop();
      }
      REQUIRE( q.size() == int( sq.size() ) );
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == sq.front() );
      REQUIRE( q.size() == int( sq.size() ) );
      q.pop();
      sq.pop();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == (int)sq.size() );
    REQUIRE( q.size() == 0 );
  }

  SECTION( "biased to push" ) {
    auto bs =
        rl::generate_n( L0( rng::between(
                            0, 3, rng::e_interval::half_open ) ),
                        10000 )
            .to_vector();
    auto is = rl::ints( 0, 10000 ).to_vector();
    REQUIRE( is.size() == bs.size() );

    for( auto [action, n] : rl::zip( bs, is ) ) {
      REQUIRE( action >= 0 );
      REQUIRE( action <= 2 );
      if( action == 0 || action == 1 ) {
        q.push( n );
        sq.push( n );
      } else if( !q.empty() && !sq.empty() ) {
        q.pop();
        sq.pop();
      }
      REQUIRE( q.size() == (int)sq.size() );
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == sq.front() );
      REQUIRE( q.size() == (int)sq.size() );
      q.pop();
      sq.pop();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == (int)sq.size() );
    REQUIRE( q.size() == 0 );
  }

  SECTION( "biased to pop" ) {
    auto bs =
        rl::generate_n( L0( rng::between(
                            0, 3, rng::e_interval::half_open ) ),
                        10000 )
            .to_vector();
    auto is = rl::ints( 0, 10000 ).to_vector();
    REQUIRE( is.size() == bs.size() );

    for( auto [action, n] : rl::zip( bs, is ) ) {
      REQUIRE( action >= 0 );
      REQUIRE( action <= 2 );
      if( action == 0 ) {
        q.push( n );
        sq.push( n );
      } else if( !q.empty() && !sq.empty() ) {
        q.pop();
        sq.pop();
      }
      REQUIRE( q.size() == (int)sq.size() );
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == (int)sq.front() );
      REQUIRE( q.size() == (int)sq.size() );
      q.pop();
      sq.pop();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == (int)sq.size() );
    REQUIRE( q.size() == 0 );
  }
}

// This test really puts the flat_deque to the test. It will ran-
// domly push and pop thousands of random elements into the
// flat_deque and an std::deque and compare them at every turn.
TEST_CASE( "[flat-deque] std::deque comparison" ) {
  int size = 10000;

  flat_deque<int> q;
  std::deque<int> sq;

  // Catch2 takes its seed on the command line; from this seed
  // we generate some random numbers to reseed our own random
  // gener- ator to get predictable results based on Catch2's
  // seed.
  uint32_t sub_seed =
      GENERATE( take( 1, random( 0, 1000000 ) ) );
  INFO( fmt::format( "sub_seed: {}", sub_seed ) );
  rng::reseed( sub_seed );

  SECTION( "roughly equal push and pop" ) {
    auto bs =
        rl::generate_n( L0( rng::between(
                            0, 3, rng::e_interval::half_open ) ),
                        size )
            .to_vector();
    auto is = rl::ints( 0, size ).to_vector();
    REQUIRE( is.size() == bs.size() );

    for( auto [action, n] : rl::zip( bs, is ) ) {
      REQUIRE( action >= 0 );
      REQUIRE( action <= 2 );
      if( action == 0 ) {
        q.push_back( n );
        sq.push_back( n );
      } else if( action == 1 ) {
        q.push_front( n );
        sq.push_front( n );
      } else if( !q.empty() && !sq.empty() ) {
        q.pop_front();
        sq.pop_front();
      }
      REQUIRE( q.size() == int( sq.size() ) );
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
      if( !q.empty() ) { REQUIRE( q.back() == sq.back() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == sq.front() );
      REQUIRE( q.back() == sq.back() );
      REQUIRE( q.size() == int( sq.size() ) );
      q.pop_front();
      sq.pop_front();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == int( sq.size() ) );
    REQUIRE( q.size() == 0 );
  }

  SECTION( "biased to push" ) {
    auto bs =
        rl::generate_n( L0( rng::between(
                            0, 3, rng::e_interval::half_open ) ),
                        10000 )
            .to_vector();
    auto is = rl::ints( 0, 10000 ).to_vector();
    REQUIRE( is.size() == bs.size() );

    for( auto [action, n] : rl::zip( bs, is ) ) {
      REQUIRE( action >= 0 );
      REQUIRE( action <= 2 );
      if( action == 0 ) {
        q.push_back( n );
        sq.push_back( n );
      } else if( action == 1 ) {
        q.push_front( n );
        sq.push_front( n );
      } else if( !q.empty() && !sq.empty() ) {
        q.pop_back();
        sq.pop_back();
      }
      REQUIRE( q.size() == int( sq.size() ) );
      if( !q.empty() ) { REQUIRE( q.back() == sq.back() ); }
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == sq.front() );
      REQUIRE( q.back() == sq.back() );
      REQUIRE( q.size() == int( sq.size() ) );
      q.pop_front();
      sq.pop_front();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == int( sq.size() ) );
    REQUIRE( q.size() == 0 );
  }

  SECTION( "biased to pop" ) {
    auto bs =
        rl::generate_n( L0( rng::between(
                            0, 3, rng::e_interval::half_open ) ),
                        10000 )
            .to_vector();
    auto is = rl::ints( 0, 10000 ).to_vector();
    REQUIRE( is.size() == bs.size() );

    bool front_back = true;
    for( auto [action, n] : rl::zip( bs, is ) ) {
      REQUIRE( action >= 0 );
      REQUIRE( action <= 2 );
      if( action == 0 ) {
        if( front_back ) {
          q.push_front( n );
          sq.push_front( n );
        } else {
          q.push_back( n );
          sq.push_back( n );
        }
        front_back = !front_back;
      } else if( !q.empty() && !sq.empty() ) {
        q.pop_back();
        sq.pop_back();
      }
      REQUIRE( q.size() == (int)sq.size() );
      if( !q.empty() ) { REQUIRE( q.front() == sq.front() ); }
      if( !q.empty() ) { REQUIRE( q.back() == sq.back() ); }
    }

    while( q.size() > 0 ) {
      REQUIRE( q.front() == sq.front() );
      REQUIRE( q.back() == sq.back() );
      REQUIRE( q.size() == int( sq.size() ) );
      q.pop_back();
      sq.pop_back();
    }

    REQUIRE( !q.front().has_value() );
    REQUIRE( q.size() == int( sq.size() ) );
    REQUIRE( q.size() == 0 );
  }
}

} // namespace
