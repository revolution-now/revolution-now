/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Unit tests for the src/rcl/model.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/model.hpp"

// cdr
#include "src/cdr/converter.hpp"

// base
#include "base/io.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rcl {
namespace {

using namespace std;

using namespace ::cdr::literals;

using ::base::expect;
using ::cdr::list;
using ::cdr::table;
using ::testing::data_dir;

TEST_CASE( "[rcl/model] post-processing" ) {
  static auto const tbl = table{
      "a"_key =
          table{
              "c"_key =
                  table{
                      "f"_key = 5,
                      "g"_key = true,
                      "h"_key = "truedat",
                  },
          },
      "b"_key =
          table{
              "s"_key = 5,
              "t"_key = 3.5,
          },
      "c.d.e"_key =
          table{
              "f"_key =
                  table{
                      "g"_key =
                          table{
                              "h"_key =
                                  table{
                                      "i"_key = 9,
                                      "j"_key = 10,
                                  },
                              "yes"_key = "no",
                          },
                  },
              "unit"_key = 1,
          },
      "file"_key = "/this/is/a/file/path",
      "url"_key  = "http://domain.com?x=y",
      "tbl1"_key =
          table{
              "x"_key            = 1,
              "y"_key            = 2,
              "z"_key            = 3,
              "\"hello yo\""_key = "world",
              "yes"_key          = "no",
          },
      "tbl2"_key =
          table{
              "x"_key     = 1,
              "y"_key     = "2 3",
              "z"_key     = 3,
              "hello"_key = "world wide",
              "yes"_key   = "x",
          },
      "one"_key =
          table{
              "\" two\\a\\b\\\" xxx\""_key =
                  list{
                      1,
                      2,
                      table{
                          "one two"_key =
                              table{
                                  "three"_key = 3,
                                  "four"_key  = 4,
                                  "hello"_key = 1,
                                  "world"_key = 2,
                              },
                      },
                  },
          },
      "subtype"_key =
          table{
              "\"this is.a test[]{}\""_key =
                  table{
                      "a"_key =
                          list{
                              "abc",
                              5,
                              -.03,
                              "table",
                              table{
                                  "a"_key =
                                      table{
                                          "b"_key =
                                              table{
                                                  "c"_key = 1,
                                              },
                                          "d"_key = 2,
                                      },
                              },
                          },
                  },
              "x"_key = 9,
          },
      "aaa.b.c.a.b.c"_key =
          list{
              table{
                  "a"_key =
                      table{
                          "b"_key =
                              table{
                                  "c"_key =
                                      table{
                                          "f"_key =
                                              table{
                                                  "g"_key =
                                                      table{
                                                          "h"_key =
                                                              table{
                                                                  "i"_key =
                                                                      5,
                                                                  "j"_key =
                                                                      6,
                                                                  "k"_key =
                                                                      5,
                                                                  "l"_key =
                                                                      6,
                                                              },
                                                      },
                                              },
                                      },
                              },
                      },
              },
          },
      "list"_key =
          list{
              "one",
              "two",
              3,
              "false",
              "four",
              cdr::null,
          },
      "null_val"_key    = cdr::null,
      "nonnull_val"_key = "null",
      "z"_key           = table{},
      "zz.yy"_key       = table{},
  };

  expect<doc> res = doc::create( table{ tbl }, /*opts=*/{} );
  REQUIRE( res.has_value() );

  // Now compare with golden.
  auto golden_file = data_dir() / "rcl" / "complex-golden.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( *res ) == golden );
}

} // namespace
} // namespace rcl
