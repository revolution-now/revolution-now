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

// base
#include "base/io.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rcl {
namespace {

using namespace std;

using ::base::expect;
using ::rn::testing::data_dir;

/****************************************************************
** Helpers
*****************************************************************/
template<typename... Vs>
list make_list( Vs&&... vs ) {
  vector<value> v;
  ( v.push_back( std::forward<Vs>( vs ) ), ... );
  return list( std::move( v ) );
}

template<typename... Vs>
value make_list_val( Vs&&... vs ) {
  return value{ make_unique<list>( make_list( FWD( vs )... ) ) };
}

template<typename... Kvs>
table make_table( Kvs&&... kvs ) {
  using KV = table::key_val;
  vector<KV> v;
  ( v.push_back( std::forward<Kvs>( kvs ) ), ... );
  return table( std::move( v ) );
}

template<typename... Kvs>
value make_table_val( Kvs&&... kvs ) {
  return value{
      make_unique<table>( make_table( FWD( kvs )... ) ) };
}

template<typename... Kvs>
expect<doc, string> make_doc( Kvs&&... kvs ) {
  using KV = table::key_val;
  vector<KV> v;
  ( v.push_back( std::forward<Kvs>( kvs ) ), ... );
  return doc::create( table( std::move( v ) ) );
}

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[model] complex doc" ) {
  using KV = table::key_val;

  /*
   * a.c.f = 5
   * a.c.g = true
   * a.c.h = truedat
   *
   * b.s = 5
   * b.t = 3.5
   *
   * c.d.e.f.g.h.i = 9
   * c.d.e.f.g.h.j = 10
   *
   * file: /this/is/a/file/path
   * url: "http://domain.com?x=y"
   *
   * tbl1: { x=1, y: 2, z=3, hello="world", yes=no }
   * tbl2: { x=1, y: 2, z=3, hello="world", yes=     x  }
   *
   * one {
   *   two = [
   *      1,
   *      2,
   *      {
   *        one.two.three = 3
   *        one.two.four = 4
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
   * subtype.some_section.a = [
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
   * list [
   *   one
   *   two
   *   3
   *   "false"
   *   four
   * ]
   */
  // This should construct the above document.
  auto doc = make_doc(
      KV{ "a.c.f", 5 }, KV{ "a.c.g", true },
      KV{ "a.c.h", "truedat" },

      KV{ "b.s", 5 }, KV{ "b.t", 3.5 },

      KV{ "c.d.e.f.g.h.i", 9 }, KV{ "c.d.e.f.g.h.j", 10 },

      KV{ "file", "/this/is/a/file/path" },
      KV{ "url", "http://domain.com?x=y" },

      KV{ "tbl1",
          make_table_val( KV{ "x", 1 }, KV{ "y", 2 },
                          KV{ "z", 3 }, KV{ "hello", "world" },
                          KV{ "yes", "no" } ) },
      KV{ "tbl2",
          make_table_val( KV{ "x", 1 }, KV{ "y", 2 },
                          KV{ "z", 3 }, KV{ "hello", "world" },
                          KV{ "yes", "x" } ) },

      KV{ "one",
          make_table_val(
              KV{ "two",
                  make_list_val(
                      1, 2,
                      make_table_val(
                          KV{ "one.two.three", 3 },
                          KV{ "one.two.four", 4 },
                          KV{ "one.two",
                              make_table_val(
                                  KV{ "hello", 1 },
                                  KV{ "world", 2 } ) } ) ) } ) },

      KV{ "c.d.e",
          make_table_val( KV{ "unit", 1 },
                          KV{ "f.g", make_table_val( KV{
                                         "yes", "no" } ) } ) },

      KV{ "subtype.some_section.a",
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

      KV{ "list",
          make_list_val( "one", "two", 3, "false", "four" ) } );

  REQUIRE( doc );

  auto golden_file = data_dir() / "rcl" / "complex-golden.rcl";
  UNWRAP_CHECK( golden,
                base::read_text_file_as_string( golden_file ) );

  REQUIRE( fmt::to_string( doc ) == golden );
}

} // namespace
} // namespace rcl
