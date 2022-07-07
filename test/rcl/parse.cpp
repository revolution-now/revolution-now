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

// Abseil
#include "absl/strings/str_split.h"

// Must be last.
#include "test/catch-common.hpp"

namespace rcl {
namespace {

using namespace std;

using ::Catch::Contains;
using ::testing::data_dir;

TEST_CASE( "[parse] complex doc" ) {
  static string const input = R"(
    a.c {
      f = 5
      g = true
      h = truedat
    }

    z = {}
    zz.yy = {}

    b {
      s = 5
      t = 3.5
    }

    c.d.e {
      f.g {
        h {
          i = 9
          j = 10
        }
        yes=no
      }
      unit: 1
    }

    file: /this/is/a/file/path
    url: "http://domain.com?x=y"

    tbl1: { x=1, y: 2, z=3, "hello yo"="world", yes=no }
    tbl2: { x=1, y: "2 3", z=3, hello="world wide", yes=     x  }

    one {
      " two\a\b\" xxx" = [
         1,
         2,
         {
           one {
             two {
               three = 3
               four = 4
               hello=1
               world=2
             }
           }
         },
      ]
    }

    subtype {
      "this is.a test[]{}".a = [
        abc,
        5,
        -.03,
        table,
        {
          a {
            b.c=1
            d=2
          }
        },
      ]
      x = 9
    }

    aaa.b.c {
      a.b.c [
        {
          a {
            b.c {
              f {
                g {
                  h {
                    i: 5
                    j: 6
                    k: 5
                    l: 6
                  }
                }
              }
            }
          }
        },
      ]
    }

    "list" [
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

TEST_CASE( "[parse] space-separated nested table syntax" ) {
  static string const input = R"(
    one two {
      a: b
      c: d
      three.four.five {
        six: 6
        seven: 7
      }
    }
    list: [
      {
        # Test braces next to key.
        x y z{ a=5 }
      }
      {
        a b c  = 9
      }
    ]
  )";

  auto doc = parse( "fake-file", input );
  REQUIRE( doc );

  string expected =
      "list: [\n"
      "  {\n"
      "    x {\n"
      "      y {\n"
      "        z {\n"
      "          a: 5\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "  },\n"
      "  {\n"
      "    a {\n"
      "      b {\n"
      "        c: 9\n"
      "      }\n"
      "    }\n"
      "  },\n"
      "]\n"
      "\n"
      "one {\n"
      "  two {\n"
      "    a: b\n"
      "    c: d\n"
      "    three {\n"
      "      four {\n"
      "        five {\n"
      "          seven: 7\n"
      "          six: 6\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "}\n";

  REQUIRE( fmt::to_string( doc ) == expected );
}

TEST_CASE( "[parse] table keys with quotes" ) {
  static string const input = R"(
    one ".two'hello world'" {
      a: b
      c: d
      three "fo\\ur  five" {
        "six": 6
        seven: 7
      }
    }
    "this"is a.weird."string" {}
    " \"list\"": [
      {
        # Test braces next to key.
        x y "z"{ a=5 }
      }
      {
        # Test key next to quote.
        "x"a b c  = 9
      }
    ]
  )";

  auto doc = parse( "fake-file", input );
  REQUIRE( doc );

  string expected =
      "\" \\\"list\\\"\": [\n"
      "  {\n"
      "    x {\n"
      "      y {\n"
      "        z {\n"
      "          a: 5\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "  },\n"
      "  {\n"
      "    xa {\n"
      "      b {\n"
      "        c: 9\n"
      "      }\n"
      "    }\n"
      "  },\n"
      "]\n"
      "\n"
      "one {\n"
      "  \".two'hello world'\" {\n"
      "    a: b\n"
      "    c: d\n"
      "    three {\n"
      "      \"fo\\\\ur  five\" {\n"
      "        seven: 7\n"
      "        six: 6\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "}\n"
      "\n"
      "thisis {\n"
      "  a {\n"
      "    weird {\n"
      "      string {}\n"
      "    }\n"
      "  }\n"
      "}\n";

  string doc_str = fmt::to_string( doc );

  vector<string> doc_split = absl::StrSplit( doc_str, "\n" );
  vector<string> expected_split =
      absl::StrSplit( expected, "\n" );

  for( int i = 0; i < int( doc_split.size() ); ++i ) {
    if( i >= int( expected_split.size() ) ) break;
    INFO( fmt::format( "line: {}", i ) );
    REQUIRE( doc_split[i] == expected_split[i] );
  }

  REQUIRE( fmt::to_string( doc ) == expected );
}

TEST_CASE( "[parse] table with duplicate keys" ) {
  SECTION( "top-level" ) {
    static string const input =
        "one: 1\n"
        "one: 2";

    auto doc = parse( "fake-file", input );
    REQUIRE( !doc.has_value() );
    REQUIRE_THAT(
        doc.error(),
        Contains(
            "fake-file:error:2:4: unexpected character" ) );
  }
  SECTION( "nested" ) {
    static string const input =
        "one {\n"
        "  a: b\n"
        "  a: c\n"
        "}";

    auto doc = parse( "fake-file", input );
    REQUIRE( !doc.has_value() );
    REQUIRE_THAT(
        doc.error(),
        Contains(
            "fake-file:error:3:4: unexpected character" ) );
  }
}

} // namespace
} // namespace rcl
