/****************************************************************
**emitter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Unit tests for the src/render/emitter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/emitter.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::gfx::pixel;
using ::gfx::point;

TEST_CASE( "[render/emitter] emit" ) {
  SpriteVertex vert1( point{ .x = 1, .y = 2 },
                      point{ .x = 3, .y = 4 },
                      point{ .x = 5, .y = 6 } );
  SolidVertex  vert2(
       point{ .x = 1, .y = 2 },
       pixel{ .r = 10, .g = 20, .b = 30, .a = 40 } );
  SpriteVertex         vert3( point{ .x = 2, .y = 3 },
                              point{ .x = 4, .y = 5 },
                              point{ .x = 5, .y = 6 } );
  vector<SpriteVertex> sprites{ vert1, vert3, vert3, vert1 };

  vector<GenericVertex> v;
  bool                  log_capacity_changes = false;

  {
    Emitter emitter( v );
    emitter.log_capacity_changes( log_capacity_changes );

    REQUIRE( v.size() == 0 );
    REQUIRE( v.capacity() == 0 );

    emitter.emit( vert1 );
    REQUIRE( v == vector<GenericVertex>{ vert1.generic() } );

    emitter.emit( vert2 );
    REQUIRE( v == vector<GenericVertex>{ vert1.generic(),
                                         vert2.generic() } );

    emitter.emit( vert2 );
    REQUIRE( v == vector<GenericVertex>{ vert1.generic(),
                                         vert2.generic(),
                                         vert2.generic() } );

    vector<SpriteVertex> sprites_empty;
    emitter.emit( span<SpriteVertex const>( sprites_empty ) );
    REQUIRE( v == vector<GenericVertex>{ vert1.generic(),
                                         vert2.generic(),
                                         vert2.generic() } );

    emitter.emit( span<SpriteVertex const>( sprites ) );
    REQUIRE( v == vector<GenericVertex>{
                      vert1.generic(), vert2.generic(),
                      vert2.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic() } );

    emitter.emit( span<SpriteVertex const>( sprites ) );
    REQUIRE( v == vector<GenericVertex>{
                      vert1.generic(), vert2.generic(),
                      vert2.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic() } );
  }

  // Overwrite.
  {
    Emitter emitter( v, 9 );
    emitter.log_capacity_changes( log_capacity_changes );

    emitter.emit( span<SpriteVertex const>( sprites ) );
    REQUIRE( v == vector<GenericVertex>{
                      vert1.generic(), vert2.generic(),
                      vert2.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic(), vert1.generic(),
                      vert3.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic() } );
  }

  {
    Emitter emitter( v, 12 );
    emitter.log_capacity_changes( log_capacity_changes );

    emitter.emit( span<SpriteVertex const>( sprites ) );
    REQUIRE( v == vector<GenericVertex>{
                      vert1.generic(), vert2.generic(),
                      vert2.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic(), vert1.generic(),
                      vert3.generic(), vert1.generic(),
                      vert3.generic(), vert3.generic(),
                      vert1.generic(), vert3.generic(),
                      vert3.generic(), vert1.generic() } );
  }

  {
    Emitter emitter( v, 100 );
    emitter.log_capacity_changes( log_capacity_changes );

    emitter.emit( span<SpriteVertex const>( sprites ) );
    REQUIRE( v.size() == 104 );
    REQUIRE( v[99] == GenericVertex{} );
    REQUIRE( v[100] == vert1.generic() );
    REQUIRE( v[101] == vert3.generic() );
    REQUIRE( v[102] == vert3.generic() );
    REQUIRE( v[103] == vert1.generic() );
  }
}

} // namespace
} // namespace rr
