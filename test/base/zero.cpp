/****************************************************************
**zero.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Unit tests for the src/base/zero.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/zero.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <unordered_map>

namespace base {
namespace {

using namespace std;

unordered_map<int, int> resources;
int                     next_key = 0;
int                     next_val = 0;

void reset_resources() {
  resources = {};
  next_key  = 0;
  next_val  = 0;
}

int new_resource() {
  resources.insert( { next_key, next_val } );
  ++next_val;
  return next_key++;
}

struct MockFile : public zero<MockFile, int> {
  using Base = zero<MockFile, int>;

  MockFile() : Base( new_resource() ) {}

  void free_resource() {
    int i = resource();
    BASE_CHECK( resources.contains( i ), "does not contain {}.",
                i );
    resources.erase( i );
  }

  int copy_resource() const {
    int r = resource();
    BASE_CHECK( resources.contains( r ), "does not contain {}.",
                r );
    resources.insert( { next_key, resources[r] } );
    return next_key++;
  }
};

// These test that a) the zero class has noexcept on its move op-
// erations and b) that this is preserved by subclasses. Having
// noexcept move operations is important for e.g. allowing a
// std::vector to optimize the process of growing its size.
static_assert( is_nothrow_move_constructible_v<MockFile> );
static_assert( is_nothrow_move_assignable_v<MockFile> );

TEST_CASE( "[zero] frees resource" ) {
  reset_resources();
  REQUIRE( resources.size() == 0 );

  {
    MockFile mf;
    REQUIRE( mf.own() );
    REQUIRE( mf.resource() == 0 );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
  }

  REQUIRE( resources == unordered_map<int, int>{} );
}

TEST_CASE( "[zero] no free after relinquish" ) {
  reset_resources();
  REQUIRE( resources.size() == 0 );

  {
    MockFile mf;
    REQUIRE( mf.resource() == 0 );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
    REQUIRE( mf.has_value() );
    REQUIRE( mf.own() );
    mf.relinquish();
    REQUIRE( mf.has_value() );
    REQUIRE( !mf.own() );
  }

  REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
}

TEST_CASE( "[zero] manual free" ) {
  reset_resources();
  REQUIRE( resources.size() == 0 );

  {
    MockFile mf;
    REQUIRE( mf.resource() == 0 );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
    REQUIRE( mf.has_value() );
    REQUIRE( mf.own() );

    mf.free();
    REQUIRE( !mf.has_value() );
    REQUIRE( !mf.own() );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{} );

    mf.free();
    REQUIRE( !mf.has_value() );
    REQUIRE( !mf.own() );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{} );

    mf.relinquish();
    REQUIRE( !mf.has_value() );
    REQUIRE( !mf.own() );
    REQUIRE( resources == unordered_map<int, int>{} );
  }

  REQUIRE( resources == unordered_map<int, int>{} );
}

TEST_CASE( "[zero] copy after relinquish" ) {
  reset_resources();
  REQUIRE( resources.size() == 0 );

  {
    // Default construction.
    MockFile mf;
    REQUIRE( mf.has_value() );
    REQUIRE( mf.own() );
    REQUIRE( mf.resource() == 0 );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );

    mf.relinquish();
    REQUIRE( !mf.own() );

    // Copy assignment after relinquish.
    MockFile mf2 = mf;
    REQUIRE( mf.resource() == 0 );
    REQUIRE( mf2.resource() == 0 );
    REQUIRE( mf.has_value() );
    REQUIRE( mf2.has_value() );
    REQUIRE( !mf.own() );
    REQUIRE( !mf2.own() );
    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
  }

  REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );
}

TEST_CASE( "[zero] copy and move" ) {
  reset_resources();
  REQUIRE( resources.size() == 0 );

  {
    // Default construction.
    MockFile mf;
    REQUIRE( mf.has_value() );
    REQUIRE( mf.own() );
    REQUIRE( mf.resource() == 0 );

    REQUIRE( next_key == 1 );
    REQUIRE( next_val == 1 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );

    {
      // Copy constructor.
      MockFile mf2( mf );
      REQUIRE( mf2.has_value() );
      REQUIRE( mf2.own() );
      REQUIRE( mf2.resource() == 1 );
      REQUIRE( next_key == 2 );
      REQUIRE( next_val == 1 );
      REQUIRE( resources ==
               unordered_map<int, int>{ { 0, 0 }, { 1, 0 } } );

      {
        // Move constructor.
        MockFile mf3( std::move( mf2 ) );
        REQUIRE( mf3.has_value() );
        REQUIRE( mf3.own() );
        REQUIRE( !mf2.has_value() );
        REQUIRE( !mf2.own() );
        REQUIRE( mf3.resource() == 1 );
        REQUIRE( next_key == 2 );
        REQUIRE( next_val == 1 );
        REQUIRE( resources ==
                 unordered_map<int, int>{ { 0, 0 }, { 1, 0 } } );

        MockFile mf4;
        REQUIRE( mf4.has_value() );
        REQUIRE( mf4.own() );
        REQUIRE( mf4.resource() == 2 );
        REQUIRE( next_key == 3 );
        REQUIRE( next_val == 2 );
        REQUIRE( resources ==
                 unordered_map<int, int>{
                     { 0, 0 }, { 1, 0 }, { 2, 1 } } );

        {
          MockFile mf4b;
          REQUIRE( mf4b.has_value() );
          REQUIRE( mf4b.own() );
          REQUIRE( mf4b.resource() == 3 );
          REQUIRE( next_key == 4 );
          REQUIRE( next_val == 3 );
          REQUIRE(
              resources ==
              unordered_map<int, int>{
                  { 0, 0 }, { 1, 0 }, { 2, 1 }, { 3, 2 } } );

          MockFile mf5;
          REQUIRE( mf5.has_value() );
          REQUIRE( mf5.own() );
          REQUIRE( mf5.resource() == 4 );
          REQUIRE( next_key == 5 );
          REQUIRE( next_val == 4 );
          REQUIRE( resources ==
                   unordered_map<int, int>{ { 0, 0 },
                                            { 1, 0 },
                                            { 2, 1 },
                                            { 3, 2 },
                                            { 4, 3 } } );

          // Move assignment.
          mf5 = std::move( mf4b );
          REQUIRE( mf5.has_value() );
          REQUIRE( mf5.own() );
          REQUIRE( !mf4b.has_value() );
          REQUIRE( !mf4b.own() );
          REQUIRE( mf5.resource() == 3 );
          REQUIRE( next_key == 5 );
          REQUIRE( next_val == 4 );
          REQUIRE(
              resources ==
              unordered_map<int, int>{
                  { 0, 0 }, { 1, 0 }, { 2, 1 }, { 3, 2 } } );
        }

        REQUIRE( next_key == 5 );
        REQUIRE( next_val == 4 );
        REQUIRE( resources ==
                 unordered_map<int, int>{
                     { 0, 0 }, { 1, 0 }, { 2, 1 } } );
      }

      REQUIRE( next_key == 5 );
      REQUIRE( next_val == 4 );
      REQUIRE( resources ==
               unordered_map<int, int>{ { 0, 0 } } );
    }

    // No change because mf2 was moved from.
    REQUIRE( next_key == 5 );
    REQUIRE( next_val == 4 );
    REQUIRE( resources == unordered_map<int, int>{ { 0, 0 } } );

    MockFile mf6;
    REQUIRE( mf6.has_value() );
    REQUIRE( mf6.own() );
    REQUIRE( next_key == 6 );
    REQUIRE( next_val == 5 );
    REQUIRE( resources ==
             unordered_map<int, int>{ { 0, 0 }, { 5, 4 } } );

    // Copy assignment.
    mf6 = mf;
    REQUIRE( mf6.resource() == 6 );
    REQUIRE( mf.has_value() );
    REQUIRE( mf.own() );
    REQUIRE( mf6.has_value() );
    REQUIRE( mf6.own() );
    REQUIRE( next_key == 7 );
    REQUIRE( next_val == 5 );
    REQUIRE( resources ==
             unordered_map<int, int>{ { 0, 0 }, { 6, 0 } } );
  }

  REQUIRE( next_key == 7 );
  REQUIRE( next_val == 5 );
  REQUIRE( resources == unordered_map<int, int>{} );
}

} // namespace
} // namespace base
