/****************************************************************
**matrix.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-22.
*
* Description: Unit tests for the src/gfx/matrix.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/matrix.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/matrix] construction" ) {
  Matrix<int> m( rn::Delta{ .w = 2, .h = 3 } );
  // TODO
  (void)m;
}

TEST_CASE( "[gfx/matrix] traverse" ) {
  using T          = Matrix<string>;
  using K_expected = point;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<K, point> ) v.push_back( "point" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o = Matrix<string>( size{ .w = 2, .h = 2 } );
  o[{ .x = 0, .y = 0 }] = "hello";
  o[{ .x = 1, .y = 0 }] = "world";
  o[{ .x = 0, .y = 1 }] = "again";
  o[{ .x = 1, .y = 1 }] = "matrix";
  f();

  REQUIRE( v == vector<string>{
                  "gfx::point{x=0,y=0}",
                  "point",
                  "hello",
                  "gfx::point{x=1,y=0}",
                  "point",
                  "world",
                  "gfx::point{x=0,y=1}",
                  "point",
                  "again",
                  "gfx::point{x=1,y=1}",
                  "point",
                  "matrix",
                } );
}

} // namespace
} // namespace gfx
