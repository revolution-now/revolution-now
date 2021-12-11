/****************************************************************
**parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Unit tests for the src/rcl/parse.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/parse.hpp"

// base
#include "base/io.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rcl {
namespace {

using namespace std;

using ::testing::data_dir;

TEST_CASE( "[parse] complex doc" ) {
  string_view input = R"(
    a.c.f = 5
    a.c.g = true
    a.c.h = truedat

    b.s = 5
    b.t = 3.5

    c.d.e.f.g.h.i = 9
    c.d.e.f.g.h.j = 10

    file: /this/is/a/file/path
    url: "http://domain.com?x=y"

    tbl1: { x=1, y: 2, z=3, hello="world", yes=no }
    tbl2: { x=1, y: 2, z=3, hello="world", yes=     x  }

    one {
      two = [
         1,
         2,
         {
           one.two.three = 3
           one.two.four = 4
           one.two {
             hello=1
             world=2
           }
         },
      ]
    }

    c.d.e {
      unit: 1
      f.g {
        yes=no
      }
    }

    subtype.some_section.a = [
      abc,
      5,
      -.03,
      table,
      {
        a.b.c=1
        a.d=2
      },
    ]
    subtype.x = 9

    aaa.b.c {
      a.b.c [
        {
          a.b.c {
            f.g.h.i: 5
            f.g.h.j: 6
          }
          a.b.c {
            f.g.h.k: 5
            f.g.h.l: 6
          }
        },
      ]
    }

    list [
      one
      two
      3
      "false"
      four,
      null
    ]

    null_val: null

    nonnull_val: "null"
  )";

  auto doc = parse( "fake-file", input );
  REQUIRE( doc );

  auto golden_file = data_dir() / "rcl" / "complex-golden.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( doc ) == golden );
}

} // namespace
} // namespace rcl
