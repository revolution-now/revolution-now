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
#include "flat-queue.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[flat-queue] initialization" ) {
  flat_queue<int> q;
  //
}

TEST_CASE( "[flat-queue] push pop small" ) {
  flat_queue<int> q;
  //
}

TEST_CASE( "[flat-queue] push pop large" ) {
  flat_queue<int> q;
  //
}

TEST_CASE( "[flat-queue] push pop many" ) {
  flat_queue<int> q;
  //
}

TEST_CASE( "[flat-queue] reallocation size" ) {
  flat_queue<int> q;
  //
}

TEST_CASE( "[flat-queue] with object" ) {
  struct A {};
  flat_queue<int> q;
  //
}

} // namespace
