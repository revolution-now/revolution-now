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
  using KV = table::value_type;
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
  using KV = table::value_type;
  vector<KV> v;
  ( v.push_back( std::forward<Kvs>( kvs ) ), ... );
  return doc::create( table( std::move( v ) ) );
}

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[model] complex doc" ) {
  using KV = table::value_type;

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

  auto& top = doc->top();

  REQUIRE( top.size() == 11 );

  REQUIRE( top.has_key( "file" ) );
  REQUIRE( top["file"].holds<string>() );
  REQUIRE( top["file"].as<string>() == "/this/is/a/file/path" );

  REQUIRE( top.has_key( "one" ) );
  REQUIRE( top["one"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_one =
      top["one"].as<unique_ptr<table>>();
  REQUIRE( u_one != nullptr );
  table const& one = *u_one;
  REQUIRE( one.size() == 1 );
  REQUIRE( one.has_key( "two" ) );
  REQUIRE( one["two"].holds<unique_ptr<list>>() );
  unique_ptr<list> const& u_two =
      one["two"].as<unique_ptr<list>>();
  REQUIRE( u_two != nullptr );
  list const& two = *u_two;

  REQUIRE( two.size() == 3 );
  REQUIRE( two[0].holds<int>() );
  REQUIRE( two[0].as<int>() == 1 );
  REQUIRE( two[1].holds<int>() );
  REQUIRE( two[1].as<int>() == 2 );

  REQUIRE( two[2].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_third =
      two[2].as<unique_ptr<table>>();
  REQUIRE( u_third != nullptr );
  table const& third = *u_third;

  REQUIRE( third.size() == 1 );
  REQUIRE( third.has_key( "one" ) );
  REQUIRE( third["one"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_third_one =
      third["one"].as<unique_ptr<table>>();
  REQUIRE( u_third_one != nullptr );
  table const& third_one = *u_third_one;
  REQUIRE( third_one.size() == 1 );
  REQUIRE( third_one.has_key( "two" ) );
  REQUIRE( third_one["two"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_third_two =
      third_one["two"].as<unique_ptr<table>>();
  REQUIRE( u_third_two != nullptr );
  table const& third_two = *u_third_two;
  REQUIRE( third_two.size() == 4 );

  REQUIRE( third_two.has_key( "three" ) );
  REQUIRE( third_two["three"].holds<int>() );
  REQUIRE( third_two["three"].as<int>() == 3 );
  REQUIRE( third_two.has_key( "four" ) );
  REQUIRE( third_two["four"].holds<int>() );
  REQUIRE( third_two["four"].as<int>() == 4 );
  REQUIRE( third_two.has_key( "hello" ) );
  REQUIRE( third_two["hello"].holds<int>() );
  REQUIRE( third_two["hello"].as<int>() == 1 );
  REQUIRE( third_two.has_key( "world" ) );
  REQUIRE( third_two["world"].holds<int>() );
  REQUIRE( third_two["world"].as<int>() == 2 );

  REQUIRE( top.has_key( "list" ) );
  REQUIRE( top["list"].holds<unique_ptr<list>>() );
  unique_ptr<list> const& u_list =
      top["list"].as<unique_ptr<list>>();
  REQUIRE( u_list != nullptr );
  list const& l = *u_list;
  REQUIRE( l.size() == 5 );

  REQUIRE( l[0].holds<string>() );
  REQUIRE( l[1].holds<string>() );
  REQUIRE( l[2].holds<int>() );
  REQUIRE( l[3].holds<string>() );
  REQUIRE( l[4].holds<string>() );

  REQUIRE( l[0].as<string>() == "one" );
  REQUIRE( l[1].as<string>() == "two" );
  REQUIRE( l[2].as<int>() == 3 );
  REQUIRE( l[3].as<string>() == "false" );
  REQUIRE( l[4].as<string>() == "four" );

  auto i = l.begin();
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( v.as<string>() == "one" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( v.as<string>() == "two" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<int>() );
    REQUIRE( v.as<int>() == 3 );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( v.as<string>() == "false" );
  }
  ++i;
  REQUIRE( i != l.end() );
  {
    value const& v = *i;
    REQUIRE( v.holds<string>() );
    REQUIRE( v.as<string>() == "four" );
  }
  ++i;
  REQUIRE( i == l.end() );

  REQUIRE( top.has_key( "a" ) );
  REQUIRE( top["a"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_a =
      top["a"].as<unique_ptr<table>>();
  REQUIRE( u_a != nullptr );
  table const& a = *u_a;
  REQUIRE( a.size() == 1 );

  // Make sure that the table we just got is the same table that
  // we get from indexing top at its first (ordered) element.
  {
    REQUIRE( top[0].holds<unique_ptr<table>>() );
    unique_ptr<table> const& u_a0 =
        top[0].as<unique_ptr<table>>();
    REQUIRE( u_a0 != nullptr );
    table const& a0 = *u_a0;
    REQUIRE( a0.size() == 1 );
    REQUIRE( &a0 == &a );
  }

  REQUIRE( a.has_key( "c" ) );
  REQUIRE( a["c"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_ac = a["c"].as<unique_ptr<table>>();
  REQUIRE( u_ac != nullptr );
  table const& ac = *u_ac;
  REQUIRE( ac.size() == 3 );

  REQUIRE( ac.has_key( "f" ) );
  REQUIRE( ac.has_key( "g" ) );
  REQUIRE( ac.has_key( "h" ) );

  REQUIRE( ac["f"].holds<int>() );
  REQUIRE( ac["g"].holds<bool>() );
  REQUIRE( ac["h"].holds<string>() );

  REQUIRE( ac["f"].as<int>() == 5 );
  REQUIRE( ac["g"].as<bool>() == true );
  REQUIRE( ac["h"].as<string>() == "truedat" );

  REQUIRE( top.has_key( "b" ) );
  REQUIRE( top["b"].holds<unique_ptr<table>>() );
  unique_ptr<table> const& u_b =
      top["b"].as<unique_ptr<table>>();
  REQUIRE( u_b != nullptr );
  table const& b = *u_b;
  REQUIRE( b.size() == 2 );

  REQUIRE( b.has_key( "s" ) );
  REQUIRE( b.has_key( "t" ) );

  REQUIRE( b["s"].holds<int>() );
  REQUIRE( b["t"].holds<double>() );

  REQUIRE( b["s"].as<int>() == 5 );
  REQUIRE( b["t"].as<double>() == 3.5 );

  auto it = top.begin();
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "a" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "b" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "c" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "file" );
    REQUIRE( v.holds<string>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "url" );
    REQUIRE( v.holds<string>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "tbl1" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "tbl2" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "one" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "subtype" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "aaa" );
    REQUIRE( v.holds<unique_ptr<table>>() );
  }

  ++it;
  REQUIRE( it != top.end() );
  {
    auto& [k, v] = *it;
    REQUIRE( k == "list" );
    REQUIRE( v.holds<unique_ptr<list>>() );
  }

  ++it;
  REQUIRE( it == top.end() );
}

} // namespace
} // namespace rcl
