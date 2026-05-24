/****************************************************************
**recursive-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-05-23.
*
* Description: Unit tests for the traverse/recursive module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/traverse/recursive.hpp"

// Testing.
#include "test/rds/testing.rds.hpp"

// refl
#include "src/refl/traverse.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace trv {
namespace {

using namespace std;

using namespace std::literals;

/****************************************************************
** Helpers.
*****************************************************************/
struct C {
  int n = 3;

  [[maybe_unused]] friend void to_str( C const& o, string& out,
                                       base::tag<C> ) {
    out += format( "C{{n={}}}", o.n );
  }

  [[maybe_unused]] friend void traverse( C const& o, auto& fn,
                                         tag_t<C const> ) {
    fn( o.n, "n"sv );
  }

  [[maybe_unused]] friend void traverse( C& o, auto& fn,
                                         tag_t<C> ) {
    fn( o.n, "n"sv );
  }
};

struct B {
  C c1;
  C c2;

  [[maybe_unused]] friend void to_str( B const& o, string& out,
                                       base::tag<B> ) {
    out += format( "B{{c1={},c2={}}}", o.c1, o.c2 );
  }

  [[maybe_unused]] friend void traverse( B const& o, auto& fn,
                                         tag_t<B const> ) {
    fn( o.c1, "c1"sv );
    fn( o.c2, "c2"sv );
  }

  [[maybe_unused]] friend void traverse( B& o, auto& fn,
                                         tag_t<B> ) {
    fn( o.c1, "c1"sv );
    fn( o.c2, "c2"sv );
  }
};

struct A {
  B b1;
  B b2;

  [[maybe_unused]] friend void to_str( A const& o, string& out,
                                       base::tag<A> ) {
    out += format( "A{{b1={},b2={}}}", o.b1, o.b2 );
  }

  [[maybe_unused]] friend void traverse( A const& o, auto& fn,
                                         tag_t<A const> ) {
    fn( o.b1, "b1"sv );
    fn( o.b2, "b2"sv );
  }

  [[maybe_unused]] friend void traverse( A& o, auto& fn,
                                         tag_t<A> ) {
    fn( o.b1, "b1"sv );
    fn( o.b2, "b2"sv );
  }
};

static_assert( Traversable<A> );
static_assert( Traversable<B> );
static_assert( Traversable<C> );
static_assert( base::Show<A> );
static_assert( base::Show<B> );
static_assert( base::Show<C> );

struct TestTraverser
  : public trv::RecursiveTraverserWithTracking {
  vector<string>& log;

  TestTraverser( vector<string>& log ) : log( log ) {}

  template<typename T>
  [[maybe_unused]] void visit( T const& o ) const {
    log.push_back(
        format( "{}: {}", path(), base::to_str( o ) ) );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[traverse/recursive] traverse_recursive" ) {
  vector<string> log;

  traverse_recursive( A{}, [&]( auto const& field ) {
    log.push_back( base::to_str( field ) );
  } );

  vector<string> const expected{
    "3",
    "C{n=3}",
    "3",
    "C{n=3}",
    "B{c1=C{n=3},c2=C{n=3}}",
    "3",
    "C{n=3}",
    "3",
    "C{n=3}",
    "B{c1=C{n=3},c2=C{n=3}}",
    "A{b1=B{c1=C{n=3},c2=C{n=3}},b2=B{c1=C{n=3},c2=C{n=3}}}",
  };
  REQUIRE( log == expected );
}

TEST_CASE(
    "[traverse/recursive] RecursiveTraverserWithTracking" ) {
  vector<string> log;

  TestTraverser t( log );
  t( A{}, "X" );

  vector<string> const expected{
    "X.b1.c1.n: 3",
    "X.b1.c1: C{n=3}",
    "X.b1.c2.n: 3",
    "X.b1.c2: C{n=3}",
    "X.b1: B{c1=C{n=3},c2=C{n=3}}",
    "X.b2.c1.n: 3",
    "X.b2.c1: C{n=3}",
    "X.b2.c2.n: 3",
    "X.b2.c2: C{n=3}",
    "X.b2: B{c1=C{n=3},c2=C{n=3}}",
    "X: A{b1=B{c1=C{n=3},c2=C{n=3}},b2=B{c1=C{n=3},c2=C{n=3}}}",
  };
  REQUIRE( log == expected );
}

TEST_CASE(
    "[traverse/recursive] RecursiveTraverserWithTracking (no "
    "label)" ) {
  vector<string> log;

  TestTraverser t( log );
  t( A{} );

  vector<string> const expected{
    "b1.c1.n: 3",
    "b1.c1: C{n=3}",
    "b1.c2.n: 3",
    "b1.c2: C{n=3}",
    "b1: B{c1=C{n=3},c2=C{n=3}}",
    "b2.c1.n: 3",
    "b2.c1: C{n=3}",
    "b2.c2.n: 3",
    "b2.c2: C{n=3}",
    "b2: B{c1=C{n=3},c2=C{n=3}}",
    ": A{b1=B{c1=C{n=3},c2=C{n=3}},b2=B{c1=C{n=3},c2=C{n=3}}}",
  };
  REQUIRE( log == expected );
}

} // namespace
} // namespace trv
