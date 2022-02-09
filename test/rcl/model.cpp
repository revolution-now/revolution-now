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
using ::testing::data_dir;

using KV = table::value_type;

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[model] complex doc" ) {
  /*
   * a.c.f = 5
   * a.c.g = true
   * a.c.h = truedat
   *
   * b.s = 5
   * b.t = 3.5
   *
   * c.d e.f.g.h.i = 9
   * c.d.e.f.g.h.j = 10
   *
   * file: /this/is/a/file/path
   * url: "http://domain.com?x=y"
   *
   * tbl1: { x=1, y: 2, z=3, "hello yo"="world", yes=no }
   * tbl2: { x=1, y: 2, z=3, hello="world", yes=     x  }
   *
   * one {
   *   " two\a\b\" xxx" = [
   *      1,
   *      2,
   *      {
   *        one.two.three = 3
   *        one two   four = 4
   *        one.two {
   *          hello=1
   *          world=2
   *        }
   *      },
   *   ]
   * }
   *
   * c.d.e {
   *   unit: 1
   *   f.g {
   *     yes=no
   *   }
   * }
   *
   * subtype "this is.a test[]{}" a = [
   *   abc,
   *   5,
   *   -.03,
   *   table,
   *   {
   *     a.b.c=1
   *     a.d=2
   *   },
   * ]
   * subtype.x = 9
   *
   * aaa.b.c {
   *   a.b.c [
   *     {
   *       a.b.c {
   *         f.g.h.i: 5
   *         f.g.h.j: 6
   *       }
   *       a.b.c {
   *         f.g.h.k: 5
   *         f.g.h.l: 6
   *       }
   *     },
   *   ]
   * }
   *
   * "list" [
   *   one
   *   two
   *   3
   *   "false"
   *   four,
   *   null
   * ]
   *
   * null_val:    null
   * nonnull_val: "null"
   */
  // This should construct the above document.
  auto doc = make_doc(
      KV{ "a.c.f", 5 }, KV{ "a.c.g", true },
      KV{ "a.c.h", "truedat" },

      KV{ "b.s", 5 }, KV{ "b.t", 3.5 },

      KV{ "c.d.e.f.g.h.i", 9 }, KV{ "c.d.e.f.g.h.j", 10 },

      KV{ "file", "/this/is/a/file/path" },
      KV{ "url", "http://domain.com?x=y" },

      KV{ "tbl1", make_table_val( KV{ "x", 1 }, KV{ "y", 2 },
                                  KV{ "z", 3 },
                                  KV{ "\"hello yo\"", "world" },
                                  KV{ "yes", "no" } ) },
      KV{ "tbl2",
          make_table_val( KV{ "x", 1 }, KV{ "y", 2 },
                          KV{ "z", 3 }, KV{ "hello", "world" },
                          KV{ "yes", "x" } ) },

      KV{ "one",
          make_table_val(
              KV{ "\" two\\a\\b\\\" xxx\"",
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
          make_table_val( KV{ "unit", 1 },
                          KV{ "f.g", make_table_val( KV{
                                         "yes", "no" } ) } ) },

      KV{ "subtype \"this is.a test[]{}\" a",
          make_list_val( "abc", 5, -.03, "table",
                         make_table_val( KV{ "a.b.c", 1 },
                                         KV{ "a.d", 2 } ) ) },

      KV{ "subtype.x", 9 },

      KV{ "aaa.b.c",
          make_table_val(
              KV{ "a.b.c",
                  make_list_val( make_table_val(
                      KV{ "a.b.c",
                          make_table_val( KV{ "f.g.h.i", 5 },
                                          KV{ "f.g.h.j", 6 } ) },
                      KV{ "a.b.c",
                          make_table_val(
                              KV{ "f.g.h.k", 5 },
                              KV{ "f.g.h.l", 6 } ) } ) ) } ) },

      KV{ "\"list\"", make_list_val( "one", "two", 3, "false",
                                     "four", null ) },
      KV{ "null_val", null }, KV{ "nonnull_val", "null" } );

  REQUIRE( doc );

  auto golden_file = data_dir() / "rcl" / "complex-golden.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( doc ) == golden );

  auto& top = doc->top_tbl();

  REQUIRE( top.size() == 13 );

  REQUIRE( top.has_key( "file" ) );
  REQUIRE( top["file"].holds<string>() );
  REQUIRE( type_of( top["file"] ) == type::string );
  REQUIRE( top["file"].as<string>() == "/this/is/a/file/path" );

  REQUIRE( top.has_key( "one" ) );
  REQUIRE( top["one"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( top["one"] ) == type::table );
  unique_ptr<table> const& u_one =
      top["one"].as<unique_ptr<table>>();
  REQUIRE( u_one != nullptr );
  table const& one = *u_one;
  REQUIRE( one.size() == 1 );
  string two_stuff = " two\\a\\b\" xxx";
  REQUIRE( one.has_key( two_stuff ) );
  REQUIRE( one[two_stuff].holds<unique_ptr<list>>() );
  REQUIRE( type_of( one[two_stuff] ) == type::list );
  unique_ptr<list> const& u_two =
      one[two_stuff].as<unique_ptr<list>>();
  REQUIRE( u_two != nullptr );
  list const& two = *u_two;

  REQUIRE( two.size() == 3 );
  REQUIRE( two[0].holds<int>() );
  REQUIRE( type_of( two[0] ) == type::integral );
  REQUIRE( two[0].as<int>() == 1 );
  REQUIRE( two[1].holds<int>() );
  REQUIRE( type_of( two[1] ) == type::integral );
  REQUIRE( two[1].as<int>() == 2 );

  REQUIRE( two[2].holds<unique_ptr<table>>() );
  REQUIRE( type_of( two[2] ) == type::table );
  unique_ptr<table> const& u_third =
      two[2].as<unique_ptr<table>>();
  REQUIRE( u_third != nullptr );
  table const& third = *u_third;

  REQUIRE( third.size() == 1 );
  REQUIRE( third.has_key( "one" ) );
  REQUIRE( third["one"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( third["one"] ) == type::table );
  unique_ptr<table> const& u_third_one =
      third["one"].as<unique_ptr<table>>();
  REQUIRE( u_third_one != nullptr );
  table const& third_one = *u_third_one;
  REQUIRE( third_one.size() == 1 );
  REQUIRE( third_one.has_key( "two" ) );
  REQUIRE( third_one["two"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( third_one["two"] ) == type::table );
  unique_ptr<table> const& u_third_two =
      third_one["two"].as<unique_ptr<table>>();
  REQUIRE( u_third_two != nullptr );
  table const& third_two = *u_third_two;
  REQUIRE( third_two.size() == 4 );

  REQUIRE( third_two.has_key( "three" ) );
  REQUIRE( third_two["three"].holds<int>() );
  REQUIRE( type_of( third_two["three"] ) == type::integral );
  REQUIRE( third_two["three"].as<int>() == 3 );
  REQUIRE( third_two.has_key( "four" ) );
  REQUIRE( third_two["four"].holds<int>() );
  REQUIRE( type_of( third_two["four"] ) == type::integral );
  REQUIRE( third_two["four"].as<int>() == 4 );
  REQUIRE( third_two.has_key( "hello" ) );
  REQUIRE( third_two["hello"].holds<int>() );
  REQUIRE( type_of( third_two["hello"] ) == type::integral );
  REQUIRE( third_two["hello"].as<int>() == 1 );
  REQUIRE( third_two.has_key( "world" ) );
  REQUIRE( third_two["world"].holds<int>() );
  REQUIRE( type_of( third_two["world"] ) == type::integral );
  REQUIRE( third_two["world"].as<int>() == 2 );

  REQUIRE( top.has_key( "list" ) );
  REQUIRE( top["list"].holds<unique_ptr<list>>() );
  REQUIRE( type_of( top["list"] ) == type::list );
  unique_ptr<list> const& u_list =
      top["list"].as<unique_ptr<list>>();
  REQUIRE( u_list != nullptr );
  list const& l = *u_list;
  REQUIRE( l.size() == 6 );

  REQUIRE( l[0].holds<string>() );
  REQUIRE( type_of( l[0] ) == type::string );
  REQUIRE( l[1].holds<string>() );
  REQUIRE( type_of( l[1] ) == type::string );
  REQUIRE( l[2].holds<int>() );
  REQUIRE( type_of( l[2] ) == type::integral );
  REQUIRE( l[3].holds<string>() );
  REQUIRE( type_of( l[3] ) == type::string );
  REQUIRE( l[4].holds<string>() );
  REQUIRE( type_of( l[4] ) == type::string );
  REQUIRE( l[5].holds<null_t>() );
  REQUIRE( type_of( l[5] ) == type::null );

  REQUIRE( l[0].as<string>() == "one" );
  REQUIRE( l[1].as<string>() == "two" );
  REQUIRE( l[2].as<int>() == 3 );
  REQUIRE( l[3].as<string>() == "false" );
  REQUIRE( l[4].as<string>() == "four" );
  REQUIRE( l[5].as<null_t>() == null );

  auto i = l.begin();
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
    REQUIRE( v.as<string>() == "one" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
    REQUIRE( v.as<string>() == "two" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<int>() );
    REQUIRE( type_of( v ) == type::integral );
    REQUIRE( v.as<int>() == 3 );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
    REQUIRE( v.as<string>() == "false" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
    REQUIRE( v.as<string>() == "four" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<null_t>() );
    REQUIRE( type_of( v ) == type::null );
    REQUIRE( v.as<null_t>() == null );
  }
  ++i;
  REQUIRE( i == l.end() );

  REQUIRE( top.has_key( "null_val" ) );
  REQUIRE( top["null_val"].holds<null_t>() );
  REQUIRE( top["null_val"].as<null_t>() == null );
  REQUIRE( type_of( top["null_val"] ) == type::null );
  REQUIRE( top.has_key( "nonnull_val" ) );
  REQUIRE( top["nonnull_val"].holds<string>() );
  REQUIRE( top["nonnull_val"].as<string>() == "null" );
  REQUIRE( type_of( top["nonnull_val"] ) == type::string );

  REQUIRE( top.has_key( "a" ) );
  REQUIRE( top["a"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( top["a"] ) == type::table );
  unique_ptr<table> const& u_a =
      top["a"].as<unique_ptr<table>>();
  REQUIRE( u_a != nullptr );
  table const& a = *u_a;
  REQUIRE( a.size() == 1 );

  // Make sure that the table we just got is the same table that
  // we get from indexing top at its first (ordered) element.
  {
    REQUIRE( top[0].second.holds<unique_ptr<table>>() );
    REQUIRE( type_of( top[0].second ) == type::table );
    unique_ptr<table> const& u_a0 =
        top[0].second.as<unique_ptr<table>>();
    REQUIRE( u_a0 != nullptr );
    table const& a0 = *u_a0;
    REQUIRE( a0.size() == 1 );
    REQUIRE( &a0 == &a );
  }

  REQUIRE( a.has_key( "c" ) );
  REQUIRE( a["c"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( a["c"] ) == type::table );
  unique_ptr<table> const& u_ac = a["c"].as<unique_ptr<table>>();
  REQUIRE( u_ac != nullptr );
  table const& ac = *u_ac;
  REQUIRE( ac.size() == 3 );

  REQUIRE( ac.has_key( "f" ) );
  REQUIRE( ac.has_key( "g" ) );
  REQUIRE( ac.has_key( "h" ) );

  REQUIRE( ac["f"].holds<int>() );
  REQUIRE( type_of( ac["f"] ) == type::integral );
  REQUIRE( ac["g"].holds<bool>() );
  REQUIRE( type_of( ac["g"] ) == type::boolean );
  REQUIRE( ac["h"].holds<string>() );
  REQUIRE( type_of( ac["h"] ) == type::string );

  REQUIRE( ac["f"].as<int>() == 5 );
  REQUIRE( ac["g"].as<bool>() == true );
  REQUIRE( ac["h"].as<string>() == "truedat" );

  REQUIRE( top.has_key( "b" ) );
  REQUIRE( top["b"].holds<unique_ptr<table>>() );
  REQUIRE( type_of( top["b"] ) == type::table );
  unique_ptr<table> const& u_b =
      top["b"].as<unique_ptr<table>>();
  REQUIRE( u_b != nullptr );
  table const& b = *u_b;
  REQUIRE( b.size() == 2 );

  REQUIRE( b.has_key( "s" ) );
  REQUIRE( b.has_key( "t" ) );

  REQUIRE( b["s"].holds<int>() );
  REQUIRE( type_of( b["s"] ) == type::integral );
  REQUIRE( b["t"].holds<double>() );
  REQUIRE( type_of( b["t"] ) == type::floating );

  REQUIRE( b["s"].as<int>() == 5 );
  REQUIRE( b["t"].as<double>() == 3.5 );

  auto it = top.begin();
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "a" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "b" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "c" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "file" );
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "url" );
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "tbl1" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "tbl2" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "one" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "subtype" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "aaa" );
    REQUIRE( v.holds<unique_ptr<table>>() );
    REQUIRE( type_of( v ) == type::table );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "list" );
    REQUIRE( v.holds<unique_ptr<list>>() );
    REQUIRE( type_of( v ) == type::list );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "null_val" );
    REQUIRE( v.holds<null_t>() );
    REQUIRE( type_of( v ) == type::null );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "nonnull_val" );
    REQUIRE( v.holds<string>() );
    REQUIRE( type_of( v ) == type::string );
  }

  ++it;
  REQUIRE( it == top.end() );
}

TEST_CASE( "[model] type_visitor" ) {
  SECTION( "null" ) {
    value v = null;
    REQUIRE( std::visit( type_visitor{}, v ) == type::null );
  }
  SECTION( "bool" ) {
    value v = true;
    REQUIRE( std::visit( type_visitor{}, v ) == type::boolean );
  }
  SECTION( "int" ) {
    value v = 5;
    REQUIRE( std::visit( type_visitor{}, v ) == type::integral );
  }
  SECTION( "double" ) {
    value v = 5.5;
    REQUIRE( std::visit( type_visitor{}, v ) == type::floating );
  }
  SECTION( "string" ) {
    value v = "hello";
    REQUIRE( std::visit( type_visitor{}, v ) == type::string );
  }
  SECTION( "table" ) {
    value v = make_unique<table>();
    REQUIRE( std::visit( type_visitor{}, v ) == type::table );
  }
  SECTION( "list" ) {
    value v = make_unique<list>();
    REQUIRE( std::visit( type_visitor{}, v ) == type::list );
  }
}

/****************************************************************
** Cdr
*****************************************************************/
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

TEST_CASE( "[rcl/model] cdr/to_canonical" ) {
  cdr::converter conv;
  REQUIRE( rcl_doc.has_value() );
  REQUIRE( conv.to( rcl_doc->top_val() ) == cdr_doc );
}

TEST_CASE( "[rcl/model] cdr/from_canonical" ) {
  cdr::converter conv;
  REQUIRE( rcl_doc.has_value() );
  REQUIRE( cdr_doc.holds<cdr::table>() );
  doc res = doc_from_cdr( conv, cdr_doc.get<cdr::table>() );

  auto golden_file =
      data_dir() / "rcl" / "complex-golden-sorted.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( res ) == golden );
}

} // namespace
} // namespace rcl
