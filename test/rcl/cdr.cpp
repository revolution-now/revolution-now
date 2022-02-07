/****************************************************************
**cdr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-07.
*
* Description: Unit tests for the src/rcl/cdr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/cdr.hpp"

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

using KV = table::value_type;

using ::base::expect;
using ::testing::data_dir;

cdr::value cdr_doc = cdr::table{
  "a"_key = cdr::table{
    "c"_key = cdr::table{
      "f"_key = 5,
      "g"_key = true,
      "h"_key = "truedat",
    },
  },
  "b"_key = cdr::table{
    "s"_key = 5,
    "t"_key = 3.5,
  },
  "c"_key = cdr::table{
    "d"_key = cdr::table{
      "e"_key = cdr::table{
        "f"_key = cdr::table{
          "g"_key = cdr::table{
            "h"_key = cdr::table{
              "i"_key = 9,
              "j"_key = 10,
            },
            "yes"_key = "no",
          },
        },
        "unit"_key = 1,
      },
    },
  },
  "file"_key = "/this/is/a/file/path",
  "url"_key  = "http://domain.com?x=y",
  "tbl1"_key = cdr::table{
    "x"_key = 1,
    "y"_key = 2,
    "z"_key = 3,
    "hello"_key = "world",
    "yes"_key = "no",
  },
  "tbl2"_key = cdr::table{
    "x"_key = 1,
    "y"_key = 2,
    "z"_key = 3,
    "hello"_key = "world",
    "yes"_key = "x",
  },
  "one"_key = cdr::table{
    "two"_key = cdr::list{
      1,
      2,
      cdr::table{
        "one"_key = cdr::table{
          "two"_key = cdr::table{
            "three"_key = 3,
            "four"_key = 4,
            "hello"_key = 1,
            "world"_key = 2,
          },
        },
      },
    },
  },
  "c"_key = cdr::table{
    "d"_key = cdr::table{
      "e"_key = cdr::table{
      },
    },
  },
  "subtype"_key = cdr::table{
    "some_section"_key = cdr::table{
      "a"_key = cdr::list{
        "abc",
        5,
        -.03,
        "table",
        cdr::table{
          "a"_key = cdr::table{
            "b"_key = cdr::table{
              "c"_key = 1,
            },
            "d"_key = 2,
          },
        },
      },
    },
    "x"_key = 9,
  },
  "aaa"_key = cdr::table{
    "b"_key = cdr::table{
      "c"_key = cdr::table{
        "a"_key = cdr::table{
          "b"_key = cdr::table{
            "c"_key = cdr::list{
              cdr::table{
                "a"_key = cdr::table{
                  "b"_key = cdr::table{
                    "c"_key = cdr::table{
                      "f"_key = cdr::table{
                        "g"_key = cdr::table{
                          "h"_key = cdr::table{
                            "i"_key = 5,
                            "j"_key = 6,
                            "k"_key = 5,
                            "l"_key = 6,
                          },
                        },
                      },
                    },
                  },
                },
              },
            },
          },
        },
      },
    },
  },
  "list"_key = cdr::list{
    "one",
    "two",
    3,
    "false",
    "four",
    cdr::null,
  },
  "null_val"_key = cdr::null,
  "nonnull_val"_key = "null",
};

// This should construct the above document.
auto rcl_doc = make_doc(
    KV{ "a.c.f", 5 }, KV{ "a.c.g", true },
    KV{ "a.c.h", "truedat" },

    KV{ "b.s", 5 }, KV{ "b.t", 3.5 },

    KV{ "c.d.e.f.g.h.i", 9 }, KV{ "c.d.e.f.g.h.j", 10 },

    KV{ "file", "/this/is/a/file/path" },
    KV{ "url", "http://domain.com?x=y" },

    KV{ "tbl1",
        make_table_val( KV{ "x", 1 }, KV{ "y", 2 }, KV{ "z", 3 },
                        KV{ "hello", "world" },
                        KV{ "yes", "no" } ) },
    KV{ "tbl2", make_table_val(
                    KV{ "x", 1 }, KV{ "y", 2 }, KV{ "z", 3 },
                    KV{ "hello", "world" }, KV{ "yes", "x" } ) },

    KV{ "one",
        make_table_val(
            KV{ "two",
                make_list_val(
                    1, 2,
                    make_table_val(
                        KV{ "one.two.three", 3 },
                        KV{ "one two   four", 4 },
                        KV{ "one.two",
                            make_table_val(
                                KV{ "hello", 1 },
                                KV{ "world", 2 } ) } ) ) } ) },

    KV{ "c.d.e",
        make_table_val(
            KV{ "unit", 1 },
            KV{ "f.g", make_table_val( KV{ "yes", "no" } ) } ) },

    KV{ "subtype some_section a",
        make_list_val( "abc", 5, -.03, "table",
                       make_table_val( KV{ "a.b.c", 1 },
                                       KV{ "a.d", 2 } ) ) },

    KV{ "subtype.x", 9 },

    KV{ "aaa.b.c",
        make_table_val( KV{
            "a.b.c", make_list_val( make_table_val(
                         KV{ "a.b.c", make_table_val(
                                          KV{ "f.g.h.i", 5 },
                                          KV{ "f.g.h.j", 6 } ) },
                         KV{ "a.b.c", make_table_val(
                                          KV{ "f.g.h.k", 5 },
                                          KV{ "f.g.h.l",
                                              6 } ) } ) ) } ) },

    KV{ "list", make_list_val( "one", "two", 3, "false", "four",
                               null ) },
    KV{ "null_val", null }, KV{ "nonnull_val", "null" } );

TEST_CASE( "[rcl/cdr] to_canonical" ) {
  cdr::converter conv;
  REQUIRE( rcl_doc.has_value() );
  REQUIRE( conv.to( rcl_doc->top_val() ) == cdr_doc );
}

TEST_CASE( "[rcl/cdr] from_canonical" ) {
  cdr::converter conv;
  REQUIRE( rcl_doc.has_value() );
  cdr::result<rcl::value> res = conv.from<rcl::value>( cdr_doc );
  REQUIRE( res.has_value() );
  REQUIRE( res->holds<unique_ptr<table>>() );
  auto doc =
      doc::create( std::move( *res->as<unique_ptr<table>>() ) );
  REQUIRE( doc.has_value() );

  auto golden_file =
      data_dir() / "rcl" / "complex-golden-sorted.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( *doc ) == golden );
}

} // namespace
} // namespace rcl
