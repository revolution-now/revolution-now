/****************************************************************
**expect.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-05.
*
* Description: Unit tests for the src/expect.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/expect.hpp"

// Testing
#include "test/monitoring-types.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <experimental/type_traits>
#include <functional>

#define ASSERT_VAR_TYPE( var, ... ) \
  static_assert( std::is_same_v<decltype( var ), __VA_ARGS__> )

namespace base {
namespace {

using namespace std;

using ::Catch::Contains;
using ::Catch::Equals;
using ::std::experimental::is_detected_v;
using ::testing::monitoring_types::BaseClass;
using ::testing::monitoring_types::Boolable;
using ::testing::monitoring_types::Constexpr;
using ::testing::monitoring_types::DerivedClass;
using ::testing::monitoring_types::Empty;
using ::testing::monitoring_types::Intable;
using ::testing::monitoring_types::NoCopy;
using ::testing::monitoring_types::NoCopyNoMove;
using ::testing::monitoring_types::Stringable;
using ::testing::monitoring_types::Throws;
using ::testing::monitoring_types::Tracker;
using ::testing::monitoring_types::Trivial;

template<typename T, typename V>
using E = ::base::expect<T, V>;

template<typename T>
using RR = ::std::reference_wrapper<T>;

template<typename T>
using EE = ::base::expect<T, Empty>;

} // namespace
} // namespace base

namespace base {
namespace {

/****************************************************************
** [static] Default Error Type.
*****************************************************************/
static_assert( std::is_same_v<::base::expect<int>::error_type,
                              std::string> );
static_assert( std::is_same_v<::base::expect<int&>::error_type,
                              std::string> );
// If we construct it with a std::string as the value_type but
// leave out the error type then the error type should default to
// a std::string, which should then not be allowed since we don't
// allow same value and error types.
static_assert( !is_detected_v<::base::expect, std::string> );
static_assert( !is_detected_v<::base::expect, std::string&> );

/****************************************************************
** [static] Invalid value types.
*****************************************************************/
/* clang-format off */
static_assert(  is_detected_v<E, int, Empty> );
static_assert(  is_detected_v<E, string, Empty> );
static_assert(  is_detected_v<E, NoCopy, Empty> );
static_assert(  is_detected_v<E, NoCopyNoMove, Empty> );
static_assert(  is_detected_v<E, double, Empty> );
static_assert( !is_detected_v<E, std::in_place_t, Empty> );
static_assert( !is_detected_v<E, std::in_place_t&, Empty> );
static_assert( !is_detected_v<E, std::in_place_t const&, Empty> );
static_assert( !is_detected_v<E, nothing_t, Empty> );
static_assert( !is_detected_v<E, nothing_t&, Empty> );
static_assert( !is_detected_v<E, nothing_t const&, Empty> );
/* clang-format on */

/****************************************************************
** [static] Invalid error types.
*****************************************************************/
/* clang-format off */
static_assert(  is_detected_v<E, Empty, int> );
static_assert(  is_detected_v<E, Empty, string> );
static_assert(  is_detected_v<E, Empty, NoCopy> );
static_assert(  is_detected_v<E, Empty, NoCopyNoMove> );
static_assert(  is_detected_v<E, Empty, double> );
static_assert( !is_detected_v<E, Empty, std::in_place_t> );
static_assert( !is_detected_v<E, Empty, std::in_place_t&> );
static_assert( !is_detected_v<E, Empty, std::in_place_t const&> );
static_assert( !is_detected_v<E, Empty, nothing_t> );
static_assert( !is_detected_v<E, Empty, nothing_t&> );
static_assert( !is_detected_v<E, Empty, nothing_t const&> );
/* clang-format on */

/****************************************************************
** [static] Invalid same types.
*****************************************************************/
static_assert( !is_detected_v<E, int, int> );
static_assert( !is_detected_v<E, string, string> );
static_assert( !is_detected_v<E, int&, int> );
static_assert( !is_detected_v<E, string, string&> );
static_assert( !is_detected_v<E, int const&, int> );
static_assert( !is_detected_v<E, string, string const&> );
static_assert( !is_detected_v<E, int const&, int const> );
static_assert( !is_detected_v<E, string const, string const&> );

/****************************************************************
** [static] Propagation of noexcept.
*****************************************************************/
/* clang-format off */
// `int` should always be nothrow.
static_assert(  is_nothrow_constructible_v<E<int,  Empty>,  int>   );
static_assert(  is_nothrow_constructible_v<E<int,  Empty>,  int>   );
static_assert(  is_nothrow_constructible_v<E<int,  Empty>,  Empty>  );
static_assert(  is_nothrow_constructible_v<E<int,  Empty>,  Empty>  );

static_assert(  is_nothrow_move_constructible_v<E<int,  char>>  );
static_assert(  is_nothrow_move_assignable_v<E<int,     char>>  );
static_assert(  is_nothrow_copy_constructible_v<E<int,  char>>  );
static_assert(  is_nothrow_copy_assignable_v<E<int,     char>>  );

// `string` should only throw on copies.
static_assert(  is_nothrow_constructible_v<E<string,  char>,    string>  );
static_assert(  is_nothrow_constructible_v<E<string,  char>,    string>  );
static_assert(  is_nothrow_constructible_v<E<char,    string>,  string>  );
static_assert(  is_nothrow_constructible_v<E<char,    string>,  string>  );

static_assert(  is_nothrow_move_constructible_v<E<string,   char>>    );
static_assert(  is_nothrow_move_assignable_v<E<string,      char>>    );
static_assert(  !is_nothrow_copy_constructible_v<E<string,  char>>    );
static_assert(  !is_nothrow_copy_assignable_v<E<string,     char>>    );
static_assert(  is_nothrow_move_constructible_v<E<char,     string>>  );
static_assert(  is_nothrow_move_assignable_v<E<char,        string>>  );
static_assert(  !is_nothrow_copy_constructible_v<E<char,    string>>  );
static_assert(  !is_nothrow_copy_assignable_v<E<char,       string>>  );

// Always throws except on default construction or equivalent.
static_assert(  !is_nothrow_constructible_v<E<Throws,     char>,    Throws>  );
static_assert(  !is_nothrow_constructible_v<E<char,       Throws>,  Throws>  );
static_assert(  !is_nothrow_move_constructible_v<E<char,  Throws>>  );
static_assert(  !is_nothrow_move_assignable_v<E<char,     Throws>>  );
static_assert(  !is_nothrow_copy_constructible_v<E<char,  Throws>>  );
static_assert(  !is_nothrow_copy_assignable_v<E<char,     Throws>>  );

static_assert(  !is_nothrow_move_constructible_v<E<Throws,  char>>  );
static_assert(  !is_nothrow_move_assignable_v<E<Throws,     char>>  );
static_assert(  !is_nothrow_copy_constructible_v<E<Throws,  char>>  );
static_assert(  !is_nothrow_copy_assignable_v<E<Throws,     char>>  );
/* clang-format on */

/****************************************************************
** [static] Non Default-Constructability.
*****************************************************************/
static_assert( !is_default_constructible_v<E<int, Empty>> );
static_assert( !is_default_constructible_v<E<string, Empty>> );
static_assert( !is_default_constructible_v<E<Throws, Empty>> );
static_assert( !is_default_constructible_v<E<Empty, int>> );
static_assert( !is_default_constructible_v<E<Empty, string>> );
static_assert( !is_default_constructible_v<E<Empty, Throws>> );

/****************************************************************
** [static] Constructability.
*****************************************************************/
static_assert( is_constructible_v<E<int, Empty>, int> );
static_assert( is_constructible_v<E<int, Empty>, char> );
static_assert( is_constructible_v<E<int, Empty>, long> );
static_assert( is_constructible_v<E<int, Empty>, int&> );
static_assert( is_constructible_v<E<int, Empty>, long&> );
static_assert( is_constructible_v<E<int, Empty>, float&> );
static_assert( !is_constructible_v<E<int, Empty>, string&> );
static_assert( is_constructible_v<E<int, string>, string&> );
static_assert( is_constructible_v<E<int, Empty>, char&> );
static_assert( is_constructible_v<E<int, Empty>, long&> );
static_assert( is_constructible_v<E<Empty, float>, int> );
static_assert( is_constructible_v<E<Empty, float>, double> );
static_assert( !is_constructible_v<E<Empty*, float>, void*> );
static_assert( !is_constructible_v<E<long, float>, char*> );

/* clang-format off */
static_assert( is_constructible_v<E<BaseClass, Empty>, BaseClass> );
static_assert( is_constructible_v<E<BaseClass, Empty>, DerivedClass> );
static_assert( is_constructible_v<E<Empty, BaseClass>, BaseClass> );
static_assert( is_constructible_v<E<Empty, BaseClass>, DerivedClass> );
static_assert( is_constructible_v<E<DerivedClass, Empty>, DerivedClass> );
static_assert( is_constructible_v<E<Empty, DerivedClass>, DerivedClass> );
/* clang-format on */

// Fails on gcc !?
// static_assert( !is_constructible_v<E<DerivedClass>, BaseClass>
// );

/****************************************************************
** [static] Ambiguous Constructability.
*****************************************************************/
/* clang-format off */
static_assert( is_constructible_v<E<int, string>, char> );
static_assert( !is_constructible_v<E<int, long>, char> );

static_assert( is_constructible_v<E<char const*, int>, char*> );
static_assert( !is_constructible_v<E<char const*, string>, char*> );

static_assert( is_constructible_v<E<void*, int>, char*> );
static_assert( !is_constructible_v<E<void*, char const*>, char*> );
/* clang-format on */

/****************************************************************
** [static] Avoiding bool ambiguity.
*****************************************************************/
static_assert( !is_constructible_v<bool, E<bool, Empty>> );
static_assert( is_constructible_v<bool, E<int, Empty>> );
static_assert( is_constructible_v<bool, E<string, Empty>> );
static_assert( !is_constructible_v<bool, E<Empty, bool>> );
static_assert( is_constructible_v<bool, E<Empty, int>> );
static_assert( is_constructible_v<bool, E<Empty, string>> );

/****************************************************************
** [static] is_value_truish.
*****************************************************************/
template<typename T, typename V>
using is_value_truish_t = decltype( &E<T, V>::is_value_truish );

/* clang-format off */
static_assert( is_detected_v<is_value_truish_t, int, Empty> );
static_assert( !is_detected_v<is_value_truish_t, string, Empty> );
static_assert( is_detected_v<is_value_truish_t, Boolable, Empty> );
static_assert( is_detected_v<is_value_truish_t, Intable, Empty> );
static_assert( !is_detected_v<is_value_truish_t, Stringable, Empty> );
/* clang-format on */

/****************************************************************
** [static] expect-of-reference.
*****************************************************************/
/* clang-format off */
static_assert( is_detected_v<E, int&, char> );
static_assert( is_detected_v<E, string const&, char> );
static_assert( !is_detected_v<E, char, int&> );
static_assert( !is_detected_v<E, char, string const&> );

static_assert( is_same_v<E<int&, Empty>::value_type, int&> );
static_assert( is_same_v<E<int const&, Empty>::value_type, int const&> );

static_assert( !is_constructible_v<E<int&, Empty>, int> );
static_assert( is_constructible_v<E<int&, Empty>, int&> );
static_assert( !is_constructible_v<E<int&, Empty>, int&&> );
static_assert( !is_constructible_v<E<int&, Empty>, int const&> );

static_assert( !is_constructible_v<E<int const&, Empty>, int> );
static_assert( is_constructible_v<E<int const&, Empty>, int&> );
static_assert( !is_constructible_v<E<int const&, Empty>, int&&> );
static_assert( is_constructible_v<E<int const&, Empty>, int const&> );

static_assert( !is_assignable_v<E<int&, Empty>, int> );
static_assert( !is_assignable_v<E<int&, Empty>, int&> );
static_assert( !is_assignable_v<E<int&, Empty>, int const&> );
static_assert( !is_assignable_v<E<int&, Empty>, int&&> );

static_assert( !is_assignable_v<E<int const&, Empty>, int> );
static_assert( !is_assignable_v<E<int const&, Empty>, int&> );
static_assert( !is_assignable_v<E<int const&, Empty>, int const&> );
static_assert( !is_assignable_v<E<int const&, Empty>, int&&> );

static_assert( !is_constructible_v<E<int&, Empty>, char&> );
static_assert( !is_constructible_v<E<int&, Empty>, long&> );

static_assert( !is_convertible_v<E<int&, Empty>, int&> );

static_assert( is_constructible_v<bool, E<int&, Empty>> );
static_assert( !is_constructible_v<bool, E<bool&, Empty>> );

static_assert( is_constructible_v<E<BaseClass&, Empty>, BaseClass&> );
static_assert( is_constructible_v<E<BaseClass&, Empty>, DerivedClass&> );
static_assert( !is_constructible_v<E<DerivedClass&, Empty>, BaseClass&> );
static_assert( is_constructible_v<E<DerivedClass&, Empty>, DerivedClass&> );
/* clang-format on */

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[expected] has_value/bool" ) {
  E<int, char> m1 = 5;
  REQUIRE( m1.has_value() );
  REQUIRE( bool( m1 ) );
  if( m1 ) {
  } else {
    REQUIRE( false );
  }

  m1 = 'c';
  REQUIRE( !m1.has_value() );
  REQUIRE( m1.error() == 'c' );
  REQUIRE( !bool( m1 ) );
  if( m1 ) {
    REQUIRE( false );
  } else {
  }
}

TEST_CASE( "[expected] T with no default constructor" ) {
  struct A {
    A() = delete;
    A( int m ) : n( m ) {}
    int n;
  };

  E<A, char> m = 'c';
  REQUIRE( !m.has_value() );
  REQUIRE( m.error() == 'c' );
  m = A{ 2 };
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 2 );
  A a( 6 );
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 6 );
}

TEST_CASE( "[expected] value construction" ) {
  E<int, string> m( 5 );
  REQUIRE( m.has_value() );
  REQUIRE( *m == 5 );

  E<string, int> m2( "hello" );
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == "hello" );

  E<string, int> m3( string( "hello" ) );
  REQUIRE( m3.has_value() );
  REQUIRE( *m3 == "hello" );

  E<NoCopy, int> m4( NoCopy( 'h' ) );
  REQUIRE( m4.has_value() );
  REQUIRE( *m4 == NoCopy{ 'h' } );
}

TEST_CASE( "[expected] error construction" ) {
  E<string, int> m( 5 );
  REQUIRE( !m.has_value() );
  REQUIRE( m.error() == 5 );

  E<int, string> m2( "hello" );
  REQUIRE( !m2.has_value() );
  REQUIRE( m2.error() == "hello" );

  E<int, string> m3( string( "hello" ) );
  REQUIRE( !m3.has_value() );
  REQUIRE( m3.error() == "hello" );

  E<int, NoCopy> m4( NoCopy( 'h' ) );
  REQUIRE( !m4.has_value() );
  REQUIRE( m4.error() == NoCopy{ 'h' } );
}

TEST_CASE( "[expected] converting value construction" ) {
  SECTION( "val" ) {
    struct A {
      A() = default;
      operator int() const { return 7; }
    };

    A a;

    E<int, string> m1{ a };
    REQUIRE( m1.has_value() );
    REQUIRE( *m1 == 7 );

    E<int, string> m2{ A{} };
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 7 );
  }
  SECTION( "err" ) {
    struct B {
      B() = default;
      operator string() const { return "hello"; }
    };

    B b;

    E<int, string> m1{ b };
    REQUIRE( !m1.has_value() );
    REQUIRE( m1.error() == "hello" );

    E<int, string> m2{ B{} };
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "hello" );
  }
}

TEST_CASE( "[expected] copy construction" ) {
  SECTION( "int" ) {
    SECTION( "val" ) {
      E<int, string> m{ 4 };
      REQUIRE( m.has_value() );
      REQUIRE( *m == 4 );

      E<int, string> m2( m );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 4 );
      REQUIRE( m.has_value() );
      REQUIRE( *m == 4 );

      E<int, string> m3( m );
      REQUIRE( m3.has_value() );
      REQUIRE( *m3 == 4 );
      REQUIRE( m.has_value() );
      REQUIRE( *m == 4 );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 4 );
    }
    SECTION( "err" ) {
      E<string, int> m{ 4 };
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == 4 );

      E<string, int> m2( m );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == 4 );
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == 4 );

      E<string, int> m3( m );
      REQUIRE( !m3.has_value() );
      REQUIRE( m3.error() == 4 );
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == 4 );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == 4 );
    }
  }
  SECTION( "string" ) {
    SECTION( "val" ) {
      E<string, int> m{ string( "hello" ) };
      REQUIRE( m.has_value() );
      REQUIRE( *m == "hello" );

      E<string, int> m2( m );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "hello" );
      REQUIRE( m.has_value() );
      REQUIRE( *m == "hello" );

      E<string, int> m3( m );
      REQUIRE( m3.has_value() );
      REQUIRE( *m3 == "hello" );
      REQUIRE( m.has_value() );
      REQUIRE( *m == "hello" );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "hello" );
    }
    SECTION( "err" ) {
      E<int, string> m{ string( "hello" ) };
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == "hello" );

      E<int, string> m2( m );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == "hello" );
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == "hello" );

      E<int, string> m3( m );
      REQUIRE( !m3.has_value() );
      REQUIRE( m3.error() == "hello" );
      REQUIRE( !m.has_value() );
      REQUIRE( m.error() == "hello" );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == "hello" );
    }
  }
}

TEST_CASE( "[expected] converting copy construction" ) {
  SECTION( "val" ) {
    SECTION( "int" ) {
      E<Intable, string> m1 = "hello";
      E<int, string>     m2( m1 );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == "hello" );

      E<Intable, string> m3 = Intable{ 5 };
      E<int, string>     m4( m3 );
      REQUIRE( m4.has_value() );
      REQUIRE( *m4 == 5 );
    }
    SECTION( "string" ) {
      E<Stringable, int> m1 = 3;
      E<string, int>     m2( m1 );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == 3 );

      E<Stringable, int> m3 = Stringable{ "hello" };
      E<string, int>     m4( m3 );
      REQUIRE( m4.has_value() );
      REQUIRE( *m4 == "hello" );
    }
  }
  SECTION( "err" ) {
    SECTION( "int" ) {
      E<string, Intable> m1 = "hello";
      E<string, int>     m2( m1 );
      REQUIRE( m2.has_value() );

      E<string, Intable> m3 = Intable{ 5 };
      E<string, int>     m4( m3 );
      REQUIRE( !m4.has_value() );
      REQUIRE( m4.error() == 5 );
    }
    SECTION( "string" ) {
      E<int, Stringable> m1 = 3;
      E<int, string>     m2( m1 );
      REQUIRE( m2.has_value() );

      E<int, Stringable> m3 = Stringable{ "hello" };
      E<int, string>     m4( m3 );
      REQUIRE( !m4.has_value() );
      REQUIRE( m4.error() == "hello" );
    }
  }
}

TEST_CASE( "[expected] state after move" ) {
  SECTION( "val" ) {
    E<int, string> m = "hello";
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "hello" );

    E<int, string> m2 = std::move( m );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "hello" );
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "" );

    m = 5;
    REQUIRE( m.has_value() );

    E<int, string> m3{ std::move( m ) };
    REQUIRE( m3.has_value() );
    // `expect`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m.has_value() );
  }
  SECTION( "err" ) {
    E<int, string> m = 4;
    REQUIRE( m.has_value() );

    E<int, string> m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( m.has_value() );

    m = "hello";
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "hello" );

    E<int, string> m3{ std::move( m ) };
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );
    // `expect`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "" );
  }
}

TEST_CASE( "[expected] move construction" ) {
  SECTION( "int" ) {
    E<int, string> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    E<int, string> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    // `expecct`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m.has_value() );

    E<int, string> m3( E<int, string>{ "hello" } );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );

    E<int, string> m4( E<int, string>{ 0 } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 0 );

    E<int, string> m5( std::move( m4 ) );
    // `expecct`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == 0 );

    E<int, string> m6{ std::move( m2 ) };
    REQUIRE( m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == 4 );
  }
  SECTION( "string" ) {
    E<string, int> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    E<string, int> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );

    E<string, int> m3( E<string, int>{ 5 } );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == 5 );

    E<string, int> m4( E<string, int>{ "hellm2" } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hellm2" );

    E<string, int> m5( std::move( m4 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == "hellm2" );

    E<string, int> m6{ std::move( m2 ) };
    REQUIRE( m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == "hello" );
  }
}

TEST_CASE( "[expected] converting move construction" ) {
  SECTION( "int" ) {
    E<Intable, string> m1 = "hello";
    E<int, string>     m2( std::move( m1 ) );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "hello" );

    E<Intable, string> m3 = Intable{ 5 };
    E<int, string>     m4( std::move( m3 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 5 );
  }
  SECTION( "string" ) {
    E<Stringable, int> m1 = 5;
    E<string, int>     m2( std::move( m1 ) );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 5 );

    E<Stringable, int> m3 = Stringable{ "hello" };
    E<string, int>     m4( std::move( m3 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hello" );
  }
}

TEST_CASE( "[expected] in place construction" ) {
  struct A {
    A( int n_, string s_, double d_ )
      : n( n_ ), s( s_ ), d( d_ ) {}
    int    n;
    string s;
    double d;
  };

  E<A, string> m( in_place, 5, "hello", 4.5 );
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 5 );
  REQUIRE( m->s == "hello" );
  REQUIRE( m->d == 4.5 );

  E<vector<int>, string> m2( in_place, { 4, 5 } );
  REQUIRE( m2.has_value() );
  REQUIRE_THAT( *m2, Equals( vector<int>{ 4, 5 } ) );
  REQUIRE( m2->size() == 2 );

  E<vector<int>, string> m3( in_place, 4, 5 );
  REQUIRE( m3.has_value() );
  REQUIRE_THAT( *m3, Equals( vector<int>{ 5, 5, 5, 5 } ) );
  REQUIRE( m3->size() == 4 );
}

TEST_CASE( "[expected] copy assignment" ) {
  SECTION( "val" ) {
    E<int, string> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    E<int, string> m2 = 2;
    m2                = m;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    E<int, string> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = m2;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
  }
  SECTION( "err" ) {
    E<int, string> m{ "hello" };
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "hello" );

    E<int, string> m2 = "world";
    m2                = m;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "hello" );
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "hello" );

    E<int, string> m3( "yes" );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "yes" );
    m3 = m2;
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );
  }
}

TEST_CASE( "[expected] move assignment (val)" ) {
  SECTION( "int" ) {
    E<int, string> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    E<int, string> m2 = "";
    m2                = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    // `expect`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m.has_value() );

    E<int, string> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( m2.has_value() );
  }
  SECTION( "string" ) {
    E<string, int> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    E<string, int> m2 = 0;
    m2                = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );

    E<string, int> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
    REQUIRE( m2.has_value() );
  }
  SECTION( "Tracker" ) {
    {
      Tracker::reset();
      E<Tracker, string> m1 = "";
      E<Tracker, string> m2 = "";
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 0 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 0 );
    {
      Tracker::reset();
      E<Tracker, string> m1 = "";
      m1.emplace();
      E<Tracker, string> m2 = "";
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 1 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 1 );
    {
      Tracker::reset();
      E<Tracker, string> m1 = "";
      E<Tracker, string> m2 = "";
      m2.emplace();
      m1 = std::move( m2 );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
    {
      Tracker::reset();
      E<Tracker, string> m1 = "";
      m1.emplace();
      E<Tracker, string> m2 = "";
      m2.emplace();
      m1 = std::move( m2 );
      REQUIRE( Tracker::constructed == 2 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 1 );
    }
    REQUIRE( Tracker::destructed == 2 );
  }
}

TEST_CASE( "[expected] move assignment (err)" ) {
  SECTION( "int" ) {
    E<int, string> m{ "4" };
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "4" );

    E<int, string> m2 = 5;
    m2                = std::move( m );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "4" );
    // `expect`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == "" );

    E<int, string> m3( "5" );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "5" );
    m3 = std::move( m2 );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "4" );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == "" );
  }
  SECTION( "string" ) {
    E<string, int> m{ 5 };
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 5 );

    E<string, int> m2 = "hello";
    m2                = std::move( m );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 5 );
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 5 );

    E<string, int> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = std::move( m2 );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == 5 );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 5 );
  }
  SECTION( "Tracker" ) {
    {
      Tracker::reset();
      E<string, Tracker> m1 = "";
      E<string, Tracker> m2 = "";
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 0 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 0 );
    {
      Tracker::reset();
      E<string, Tracker> m1 = "";
      m1                    = Tracker{};
      E<string, Tracker> m2 = "";
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
    {
      Tracker::reset();
      E<string, Tracker> m1 = "";
      E<string, Tracker> m2 = "";
      m2                    = Tracker{};
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 1 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 2 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 3 );
    {
      Tracker::reset();
      E<string, Tracker> m1 = "";
      m1                    = Tracker{};
      E<string, Tracker> m2 = "";
      m2                    = Tracker{};
      m1                    = std::move( m2 );
      REQUIRE( Tracker::constructed == 2 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 2 );
      REQUIRE( Tracker::move_assigned == 1 );
    }
    REQUIRE( Tracker::destructed == 4 );
  }
}

TEST_CASE( "[expected] converting assignments" ) {
  SECTION( "val" ) {
    E<int, Empty> m = 5;
    REQUIRE( m.has_value() );
    REQUIRE( *m == 5 );

    Intable a{ 7 };
    m = a;
    REQUIRE( m.has_value() );
    REQUIRE( *m == 7 );

    m = Intable{ 9 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 9 );

    E<Intable, Empty> m2 = Empty{};
    REQUIRE( !m2.has_value() );
    m2 = Intable{ 3 };

    m = m2;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 3 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 3 );

    m = std::move( m2 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 3 );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 3 );
  }
  SECTION( "err" ) {
    E<Empty, int> m = 5;
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 5 );

    Intable a{ 7 };
    m = a;
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 7 );

    m = Intable{ 9 };
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 9 );

    E<Empty, Intable> m2 = Empty{};
    REQUIRE( m2.has_value() );
    m2 = Intable{ 3 };

    m = m2;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 3 );

    m = std::move( m2 );
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == 3 );
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );
  }
}

TEST_CASE( "[expected] dereference" ) {
  E<int, string> m1 = 5;
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );

  ASSERT_VAR_TYPE( *m1, int& );
  ASSERT_VAR_TYPE( as_const( *m1 ), int const& );

  ASSERT_VAR_TYPE( ( *E<int, string>{ "" } ), int&& );

  E<NoCopy, string> m2{ 'c' };
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == NoCopy{ 'c' } );
  REQUIRE( m2->c == 'c' );

  ASSERT_VAR_TYPE( *m2, NoCopy& );
  ASSERT_VAR_TYPE( as_const( *m2 ), NoCopy const& );

  ASSERT_VAR_TYPE( ( *E<NoCopy, string>{ "" } ), NoCopy&& );

  ASSERT_VAR_TYPE( m2->c, char );
  ASSERT_VAR_TYPE( as_const( m2 )->c, char );

  ASSERT_VAR_TYPE( &m2->c, char* );
  ASSERT_VAR_TYPE( &as_const( m2->c ), char const* );
}

TEST_CASE( "[expected] emplace" ) {
  SECTION( "int" ) {
    E<int, string> m = "";
    REQUIRE( !m.has_value() );
    m.emplace( 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );
    m.emplace( 0 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 0 );
  }
  SECTION( "string" ) {
    E<string, Empty> m = Empty{};
    REQUIRE( !m.has_value() );
    m.emplace( string( "hello" ) );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );
    m.emplace( "hello2" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello2" );
    m.emplace( string_view( "hello3" ) );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello3" );
    m.emplace();
    REQUIRE( m.has_value() );
    REQUIRE( *m == "" );
  }
  SECTION( "weird constructor" ) {
    struct A {
      [[maybe_unused]] A( int n_, string s_, double d_ )
        : n( n_ ), s( s_ ), d( d_ ) {}
      int    n;
      string s;
      double d;
    };
    E<A, string> m = "";
    REQUIRE( !m.has_value() );
    m.emplace( 5, "hello", 4.0 );
    REQUIRE( m.has_value() );
    REQUIRE( m->n == 5 );
    REQUIRE( m->s == "hello" );
    REQUIRE( m->d == 4.0 );
  }
  SECTION( "non-copy non-movable" ) {
    NoCopyNoMove            ncnm( 'b' );
    E<NoCopyNoMove, string> m = "";
    REQUIRE( !m.has_value() );
    m.emplace( 'g' );
    REQUIRE( m.has_value() );
    REQUIRE( *m == NoCopyNoMove( 'g' ) );
  }
  SECTION( "initializer list" ) {
    E<vector<int>, string> m = "";
    REQUIRE( !m.has_value() );
    m.emplace( { 4, 5 } );
    REQUIRE( m.has_value() );
    REQUIRE_THAT( *m, Equals( vector<int>{ 4, 5 } ) );
    REQUIRE( m->size() == 2 );
    m.emplace( 4, 5 );
    REQUIRE( m.has_value() );
    REQUIRE_THAT( *m, Equals( vector<int>{ 5, 5, 5, 5 } ) );
    REQUIRE( m->size() == 4 );
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    {
      E<Tracker, string> m = "";
      m.emplace();
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 1 );
  }
}

TEST_CASE( "[expected] swap" ) {
  SECTION( "int" ) {
    SECTION( "both err" ) {
      E<int, string> m1 = "hello";
      E<int, string> m2 = "world";
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == "world" );
      REQUIRE( m2.error() == "hello" );
    }
    SECTION( "both val" ) {
      E<int, string> m1 = 7;
      E<int, string> m2 = 9;
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == 7 );
      REQUIRE( *m2 == 9 );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == 9 );
      REQUIRE( *m2 == 7 );
      m2.swap( m1 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == 7 );
      REQUIRE( *m2 == 9 );
    }
    SECTION( "one val one err" ) {
      E<int, string> m1 = "hello";
      E<int, string> m2 = 9;
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == "hello" );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 9 );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( *m1 == 9 );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == "hello" );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == "hello" );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 9 );
    }
  }
  SECTION( "string" ) {
    SECTION( "both err" ) {
      E<string, int> m1 = 5;
      E<string, int> m2 = 7;
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == 7 );
      REQUIRE( m2.error() == 5 );
    }
    SECTION( "both val" ) {
      E<string, Empty> m1 = "hello";
      E<string, Empty> m2 = "world";
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == "hello" );
      REQUIRE( *m2 == "world" );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == "world" );
      REQUIRE( *m2 == "hello" );
      m2.swap( m1 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == "hello" );
      REQUIRE( *m2 == "world" );
    }
    SECTION( "one val one err" ) {
      E<string, int> m1 = 5;
      E<string, int> m2 = "world";
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == 5 );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "world" );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( *m1 == "world" );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == 5 );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == 5 );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "world" );
    }
  }
  SECTION( "non-copyable val" ) {
    SECTION( "both err" ) {
      E<NoCopy, string> m1 = "hello";
      E<NoCopy, string> m2 = "world";
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == "world" );
      REQUIRE( m2.error() == "hello" );
    }
    SECTION( "both val" ) {
      E<NoCopy, string> m1 = NoCopy{ 'h' };
      E<NoCopy, string> m2 = NoCopy{ 'w' };
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == NoCopy{ 'h' } );
      REQUIRE( *m2 == NoCopy{ 'w' } );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == NoCopy{ 'w' } );
      REQUIRE( *m2 == NoCopy{ 'h' } );
      m2.swap( m1 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m1 == NoCopy{ 'h' } );
      REQUIRE( *m2 == NoCopy{ 'w' } );
    }
    SECTION( "one val one err" ) {
      E<NoCopy, string> m1 = "hello";
      E<NoCopy, string> m2 = NoCopy{ 'w' };
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == "hello" );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == NoCopy{ 'w' } );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( *m1 == NoCopy{ 'w' } );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == "hello" );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == "hello" );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == NoCopy{ 'w' } );
    }
  }
  SECTION( "non-copyable err" ) {
    SECTION( "both val" ) {
      E<string, NoCopy> m1 = "hello";
      E<string, NoCopy> m2 = "world";
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( m1.value() == "world" );
      REQUIRE( m2.value() == "hello" );
    }
    SECTION( "both err" ) {
      E<string, NoCopy> m1 = NoCopy{ 'h' };
      E<string, NoCopy> m2 = NoCopy{ 'w' };
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == NoCopy{ 'h' } );
      REQUIRE( m2.error() == NoCopy{ 'w' } );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == NoCopy{ 'w' } );
      REQUIRE( m2.error() == NoCopy{ 'h' } );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( m1.error() == NoCopy{ 'h' } );
      REQUIRE( m2.error() == NoCopy{ 'w' } );
    }
    SECTION( "one val one err" ) {
      E<string, NoCopy> m1 = "hello";
      E<string, NoCopy> m2 = NoCopy{ 'w' };
      REQUIRE( m1.has_value() );
      REQUIRE( m1.value() == "hello" );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == NoCopy{ 'w' } );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m1.error() == NoCopy{ 'w' } );
      REQUIRE( m2.has_value() );
      REQUIRE( m2.value() == "hello" );
      m2.swap( m1 );
      REQUIRE( m1.has_value() );
      REQUIRE( m1.value() == "hello" );
      REQUIRE( !m2.has_value() );
      REQUIRE( m2.error() == NoCopy{ 'w' } );
    }
  }
  SECTION( "Tracker val" ) {
    Tracker::reset();
    SECTION( "both err" ) {
      {
        E<Tracker, string> m1 = "hello";
        E<Tracker, string> m2 = "world";
        m1.swap( m2 );
        REQUIRE( Tracker::constructed == 0 );
        REQUIRE( Tracker::destructed == 0 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 0 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 0 );
    }
    SECTION( "both val" ) {
      {
        E<Tracker, string> m1 = "";
        E<Tracker, string> m2 = "";
        m1.emplace();
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 2 );
        REQUIRE( Tracker::destructed == 2 );
        REQUIRE( Tracker::copied == 0 );
        // Should call std::swap.
        REQUIRE( Tracker::move_constructed == 2 );
        REQUIRE( Tracker::move_assigned == 4 );
      }
      REQUIRE( Tracker::destructed == 4 );
    }
    SECTION( "one val one err" ) {
      {
        E<Tracker, string> m1 = "";
        E<Tracker, string> m2 = "";
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 1 );
        REQUIRE( Tracker::destructed == 4 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 4 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 5 );
    }
    SECTION( "other one has value" ) {
      {
        E<Tracker, string> m1 = "";
        E<Tracker, string> m2 = "";
        m1.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 1 );
        REQUIRE( Tracker::destructed == 4 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 4 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 5 );
    }
  }
  SECTION( "Tracker err" ) {
    Tracker::reset();
    SECTION( "both val" ) {
      {
        E<string, Tracker> m1 = "hello";
        E<string, Tracker> m2 = "world";
        m1.swap( m2 );
        REQUIRE( Tracker::constructed == 0 );
        REQUIRE( Tracker::destructed == 0 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 0 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 0 );
    }
    SECTION( "both err" ) {
      {
        E<string, Tracker> m1 = Tracker{};
        E<string, Tracker> m2 = Tracker{};
        m1.emplace();
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 2 );
        REQUIRE( Tracker::destructed == 4 );
        REQUIRE( Tracker::copied == 0 );
        // Should call std::swap.
        REQUIRE( Tracker::move_constructed == 2 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 4 );
    }
    SECTION( "one val one err" ) {
      {
        E<string, Tracker> m1 = Tracker{};
        E<string, Tracker> m2 = Tracker{};
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 2 );
        REQUIRE( Tracker::destructed == 7 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 6 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 8 );
    }
    SECTION( "other one has value" ) {
      {
        E<string, Tracker> m1 = Tracker{};
        E<string, Tracker> m2 = Tracker{};
        m1.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 2 );
        REQUIRE( Tracker::destructed == 7 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 6 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 8 );
    }
  }
}

TEST_CASE( "[expected] value_or" ) {
  SECTION( "int" ) {
    E<int, string> m = "hello";
    REQUIRE( m.value_or( 4 ) == 4 );
    m = 6;
    REQUIRE( m.value_or( 4 ) == 6 );
    m = "hello";
    REQUIRE( m.value_or( 4 ) == 4 );
    REQUIRE( E<int, string>{ 4 }.value_or( 5 ) == 4 );
    REQUIRE( E<int, string>{ "hello" }.value_or( 5 ) == 5 );
  }
  SECTION( "string" ) {
    E<string, int> m = 3;
    REQUIRE( m.value_or( "hello" ) == "hello" );
    m = "world";
    REQUIRE( m.value_or( "hello" ) == "world" );
    m = 3;
    REQUIRE( m.value_or( "hello" ) == "hello" );
    REQUIRE( E<string, int>{ "hello" }.value_or( "world" ) ==
             "hello" );
    REQUIRE( E<string, int>{ 3 }.value_or( "world" ) ==
             "world" );
  }
  SECTION( "NoCopy" ) {
    REQUIRE( E<NoCopy, string>{ 'c' }.value_or(
                 NoCopy{ 'v' } ) == NoCopy{ 'c' } );
    REQUIRE( E<NoCopy, string>{ "hello" }.value_or(
                 NoCopy{ 'v' } ) == NoCopy{ 'v' } );
  }
  SECTION( "Tracker" ) {
    {
      Tracker::reset();
      E<Tracker, string> m = "hello";
      (void)m.value_or( Tracker{} );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
    {
      Tracker::reset();
      E<Tracker, string> m = "hello";
      m.emplace();
      (void)m.value_or( Tracker{} );
      REQUIRE( Tracker::constructed == 2 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 1 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 3 );
    {
      Tracker::reset();
      (void)E<Tracker, string>{ "hello" }.value_or( Tracker{} );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
    {
      Tracker::reset();
      (void)E<Tracker, string>{ Tracker{} }.value_or(
          Tracker{} );
      REQUIRE( Tracker::constructed == 2 );
      REQUIRE( Tracker::destructed == 4 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 2 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 4 );
  }
}

TEST_CASE( "[expected] non-copyable non-movable T/E" ) {
  SECTION( "val" ) {
    E<NoCopyNoMove, Empty> m = Empty{};
    REQUIRE( !m.has_value() );
    m = 'c';
    REQUIRE( m.has_value() );
    REQUIRE( *m == NoCopyNoMove{ 'c' } );
    m = 3;
    REQUIRE( m.has_value() );
    REQUIRE( *m == NoCopyNoMove{ 3 } );
  }
  SECTION( "err" ) {
    E<Empty, NoCopyNoMove> m = 'c';
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == NoCopyNoMove{ 'c' } );
    m = 'c';
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == NoCopyNoMove{ 'c' } );
    m = 'd';
    REQUIRE( !m.has_value() );
    REQUIRE( m.error() == NoCopyNoMove{ 'd' } );
  }
}

TEST_CASE( "[expected] std::swap overload" ) {
  E<int, string> m1 = "hello";
  E<int, string> m2 = 5;
  REQUIRE( !m1.has_value() );
  REQUIRE( m2.has_value() );
  REQUIRE( m1.error() == "hello" );
  REQUIRE( *m2 == 5 );

  std::swap( m1, m2 );

  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );
  REQUIRE( !m2.has_value() );
  REQUIRE( m2.error() == "hello" );
}

TEST_CASE( "[expected] equality" ) {
  SECTION( "int" ) {
    E<int, string> m0 = "hello";
    E<int, string> m1 = "hello";
    E<int, string> m2 = "world";
    E<int, string> m3 = 5;
    E<int, string> m4 = 7;
    E<int, string> m5 = 5;

    REQUIRE( !m0.has_value() );
    REQUIRE( !m1.has_value() );
    REQUIRE( !m2.has_value() );
    REQUIRE( m3.has_value() );
    REQUIRE( m4.has_value() );
    REQUIRE( m5.has_value() );

    REQUIRE( m0 == E<int, string>( "hello" ) );
    REQUIRE( m1 == E<int, string>( "hello" ) );
    REQUIRE( m3 == E<int, string>( 5 ) );

    REQUIRE( m0 == m0 );
    REQUIRE( m0 == m1 );
    REQUIRE( m0 != m2 );
    REQUIRE( m0 != m3 );
    REQUIRE( m0 != m4 );
    REQUIRE( m0 != m5 );

    REQUIRE( m1 == m0 );
    REQUIRE( m1 == m1 );
    REQUIRE( m1 != m2 );
    REQUIRE( m1 != m3 );
    REQUIRE( m1 != m4 );
    REQUIRE( m1 != m5 );

    REQUIRE( m2 != m0 );
    REQUIRE( m2 != m1 );
    REQUIRE( m2 == m2 );
    REQUIRE( m2 != m3 );
    REQUIRE( m2 != m4 );
    REQUIRE( m2 != m5 );

    REQUIRE( m3 != m0 );
    REQUIRE( m3 != m1 );
    REQUIRE( m3 != m2 );
    REQUIRE( m3 == m3 );
    REQUIRE( m3 != m4 );
    REQUIRE( m3 == m5 );

    REQUIRE( m4 != m0 );
    REQUIRE( m4 != m1 );
    REQUIRE( m4 != m2 );
    REQUIRE( m4 != m3 );
    REQUIRE( m4 == m4 );
    REQUIRE( m4 != m5 );

    REQUIRE( m5 != m0 );
    REQUIRE( m5 != m1 );
    REQUIRE( m5 != m2 );
    REQUIRE( m5 == m3 );
    REQUIRE( m5 != m4 );
    REQUIRE( m5 == m5 );
  }
  SECTION( "string" ) {
    E<string, double> m0 = 5.5;
    E<string, double> m1 = 5.5;
    E<string, double> m2 = 6.5;
    E<string, double> m3 = "hello";
    E<string, double> m4 = "world";
    E<string, double> m5 = "hello";

    REQUIRE( m0 == E<string, double>( 5.5 ) );
    REQUIRE( m1 == E<string, double>( 5.5 ) );
    REQUIRE( m1 == 5.5 );
    REQUIRE( m3 == E<string, double>( "hello" ) );

    REQUIRE( m1 == m0 );
    REQUIRE( m1 == m1 );
    REQUIRE( m1 != m2 );
    REQUIRE( m1 != m3 );
    REQUIRE( m1 != m4 );
    REQUIRE( m1 != m5 );

    REQUIRE( m2 != m0 );
    REQUIRE( m2 != m1 );
    REQUIRE( m2 == m2 );
    REQUIRE( m2 != m3 );
    REQUIRE( m2 != m4 );
    REQUIRE( m2 != m5 );

    REQUIRE( m3 != m0 );
    REQUIRE( m3 != m1 );
    REQUIRE( m3 != m2 );
    REQUIRE( m3 == m3 );
    REQUIRE( m3 != m4 );
    REQUIRE( m3 == m5 );

    REQUIRE( m4 != m0 );
    REQUIRE( m4 != m1 );
    REQUIRE( m4 != m2 );
    REQUIRE( m4 != m3 );
    REQUIRE( m4 == m4 );
    REQUIRE( m4 != m5 );

    REQUIRE( m5 != m0 );
    REQUIRE( m5 != m1 );
    REQUIRE( m5 != m2 );
    REQUIRE( m5 == m3 );
    REQUIRE( m5 != m4 );
    REQUIRE( m5 == m5 );
  }
}

TEST_CASE( "[expected] comparison with value" ) {
  {
    E<string, int> m = 3;
    REQUIRE( m != string( "hello" ) );
    REQUIRE( m != "hello" );
    REQUIRE( string( "hello" ) != m );
    REQUIRE( "hello" != m );
    m = "hello";
    REQUIRE( m == string( "hello" ) );
    REQUIRE( m == "hello" );
    REQUIRE( string( "hello" ) == m );
    REQUIRE( "hello" == m );
  }
  {
    E<int, string> m = 3;
    REQUIRE( m != string( "hello" ) );
    REQUIRE( string( "hello" ) != m );
    REQUIRE( string( "hello" ) != m );
    m = "hello";
    REQUIRE( m == string( "hello" ) );
    REQUIRE( string( "hello" ) == m );
  }
}

TEST_CASE( "[expected] value()" ) {
  SECTION( "int" ) {
    E<int, string> m1 = "hello";
    try {
      (void)m1.value( source_location{} );
      // Should not be here.
      REQUIRE( false );
    } catch( bad_expect_access const& e ) {
      REQUIRE_THAT(
          e.what(),
          Contains( fmt::format(
              ":0: value() called on an inactive expect" ) ) );
    }
    m1 = 5;
    REQUIRE( m1.has_value() );
    REQUIRE( m1.value() == 5 );
    const E<int, string> m2 = 5;
    REQUIRE( m2.value() == 5 );
  }
  SECTION( "NoCopy" ) {
    E<NoCopy, string> m = "hello";
    m.emplace( 'a' );
    REQUIRE( m.has_value() );
    REQUIRE( m->c == 'a' );

    NoCopy nc = std::move( m ).value();
    REQUIRE( nc.c == 'a' );
  }
}

TEST_CASE( "[expected] error()" ) {
  SECTION( "int" ) {
    E<int, string> m1 = 5;
    try {
      (void)m1.error( source_location{} );
      // Should not be here.
      REQUIRE( false );
    } catch( bad_expect_access const& e ) {
      REQUIRE_THAT(
          e.what(),
          Contains( fmt::format(
              ":0: error() called on an active expect" ) ) );
    }
    m1 = "hello";
    REQUIRE( !m1.has_value() );
    REQUIRE( m1.error() == "hello" );
    const E<int, string> m2 = "world";
    REQUIRE( m2.error() == "world" );
  }
  SECTION( "NoCopy" ) {
    E<string, NoCopy> m = NoCopy{ 'a' };
    REQUIRE( !m.has_value() );
    REQUIRE( m.error().c == 'a' );

    NoCopy nc = std::move( m ).error();
    REQUIRE( nc.c == 'a' );
  }
}

TEST_CASE( "[expected] implicit conversion with same E" ) {
  static_assert( is_constructible_v<int, Intable> );
  static_assert( is_constructible_v<Intable, int> );
  static_assert( is_convertible_v<int, Intable> );
  static_assert( is_convertible_v<Intable, int> );
  SECTION( "int to Intable" ) {
    E<int, string> m1 = 4;

    E<Intable, string> m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );

    m1 = 3;
    m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
  }
  SECTION( "Intable to int" ) {
    E<Intable, string> m1 = Intable{ 4 };

    E<int, string> m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );

    m1 = Intable{ 3 };
    m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
  }
}

TEST_CASE( "[expected] implicit conversion with same T" ) {
  static_assert( is_constructible_v<int, Intable> );
  static_assert( is_constructible_v<Intable, int> );
  static_assert( is_convertible_v<int, Intable> );
  static_assert( is_convertible_v<Intable, int> );
  SECTION( "int to Intable" ) {
    E<string, int> m1 = 4;

    E<string, Intable> m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 4 );

    m1 = 3;
    m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );
  }
  SECTION( "Intable to int" ) {
    E<string, Intable> m1 = Intable{ 4 };

    E<string, int> m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 4 );

    m1 = Intable{ 3 };
    m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );
  }
}

TEST_CASE( "[expected] implicit conv. with different T/E" ) {
  static_assert( is_constructible_v<int, Intable> );
  static_assert( is_constructible_v<Intable, int> );
  static_assert( is_convertible_v<int, Intable> );
  static_assert( is_convertible_v<Intable, int> );
  static_assert( is_constructible_v<string, Stringable> );
  static_assert( is_constructible_v<Stringable, string> );
  static_assert( is_convertible_v<string, Stringable> );
  static_assert( is_convertible_v<Stringable, string> );
  SECTION( "int to Intable" ) {
    E<string, int> m1 = 4;

    E<Stringable, Intable> m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 4 );

    m1 = 3;
    m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );

    m1 = "hello";

    m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2.value().s == "hello" );

    m1 = "world";
    m2 = m1;
    REQUIRE( m2.has_value() );
    REQUIRE( m2.value().s == "world" );
  }
  SECTION( "Intable to int" ) {
    E<Stringable, Intable> m1 = Intable{ 4 };

    E<string, int> m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 4 );

    m1 = Intable{ 3 };
    m2 = m1;
    REQUIRE( !m2.has_value() );
    REQUIRE( m2.error() == 3 );
  }
}

consteval E<int, char> Calculate1( E<int, char> num,
                                   E<int, char> den ) {
  if( !num ) return 'e';
  if( !den ) return 'e';
  if( *den == 0 ) return 'e';
  return ( *num ) / ( *den );
}

consteval E<Constexpr, char> Calculate2(
    E<Constexpr, char> num, E<Constexpr, char> den ) {
  if( !num ) return 'e';
  if( !den ) return 'e';
  if( den->n == 0 ) return 'e';
  return Constexpr( num->n / den->n );
}

TEST_CASE( "[expected] consteexpr" ) {
  SECTION( "int" ) {
    constexpr E<int, char> res1 = Calculate1( 8, 4 );
    static_assert( res1 );
    static_assert( *res1 == 2 );
    constexpr E<int, char> res2 = Calculate1( 'e', 4 );
    static_assert( res2 == 'e' );
    static_assert( !res2 );
    constexpr E<int, char> res3 = Calculate1( 8, 'e' );
    static_assert( res3 == 'e' );
    static_assert( !res3 );
    constexpr E<int, char> res4 = Calculate1( 'e', 'e' );
    static_assert( res4 == 'e' );
    static_assert( !res4 );
    constexpr E<int, char> res5 = Calculate1( 8, 0 );
    static_assert( res5 == 'e' );
    static_assert( !res5 );
    constexpr E<int, char> res6 = Calculate1( 9, 4 );
    static_assert( res6 );
    static_assert( *res6 == 2 );

    constexpr E<int, char> res7 = res6;
    static_assert( res7 );
    static_assert( *res7 == 2 );

    static_assert( res7 == res6 );

    constexpr E<int, char> res8 = E<int, char>{ 5 };
    static_assert( res8 );
    static_assert( *res8 == 5 );
  }
  SECTION( "Constexpr type" ) {
    constexpr E<Constexpr, char> res1 =
        Calculate2( Constexpr{ 8 }, Constexpr{ 4 } );
    static_assert( res1 );
    static_assert( res1->n == 2 );
    constexpr E<Constexpr, char> res2 =
        Calculate2( 'e', Constexpr{ 4 } );
    static_assert( res2 == 'e' );
    static_assert( !res2 );
    constexpr E<Constexpr, char> res3 =
        Calculate2( Constexpr{ 8 }, 'e' );
    static_assert( res3 == 'e' );
    static_assert( !res3 );
    constexpr E<Constexpr, char> res4 = Calculate2( 'e', 'e' );
    static_assert( res4 == 'e' );
    static_assert( !res4 );
    constexpr E<Constexpr, char> res5 =
        Calculate2( Constexpr{ 8 }, Constexpr{ 0 } );
    static_assert( res5 == 'e' );
    static_assert( !res5 );
    constexpr E<Constexpr, char> res6 =
        Calculate2( Constexpr{ 9 }, Constexpr{ 4 } );
    static_assert( res6->n == 2 );

    constexpr E<Constexpr, char> res7 = res6;
    static_assert( res7 );
    static_assert( res7->n == 2 );

    static_assert( res7 == res6 );

    constexpr E<Constexpr, char> res8 =
        E<Constexpr, char>{ Constexpr{ 5 } };
    static_assert( res8 );
    static_assert( res8->n == 5 );
  }
}

TEST_CASE( "[expected] reference_wrapper" ) {
  int               a = 5;
  int               b = 7;
  E<RR<int>, Empty> m = Empty{};
  REQUIRE( !m.has_value() );
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( m->get() == 5 );
  m = b;
  REQUIRE( a == 5 );
  REQUIRE( m.has_value() );
  REQUIRE( m->get() == 7 );

  REQUIRE( a == 5 );
  REQUIRE( b == 7 );

  E<RR<int>, Empty> m2 = m;
  REQUIRE( m2.has_value() );

  // Test implicit conversion.
  E<RR<int>, string> m3 = a;
  E<int&, string>    m4 = m3;
  REQUIRE( m4.has_value() );
  REQUIRE( m4 == 5 );
  ++a;
  REQUIRE( m4 == 6 );

  E<RR<int const>, string> m5 = a;
  E<int const&, string>    m6 = m5;
  REQUIRE( m6.has_value() );
  REQUIRE( m6 == 6 );
  ++a;
  REQUIRE( m6 == 7 );

  E<RR<string>, Empty> m7 = Empty{};
  E<string&, Empty>    m8 = m7;
  REQUIRE( !m8.has_value() );
}

TEST_CASE( "[expected] fmap" ) {
  SECTION( "int" ) {
    auto           f = []( int n ) { return n + 1; };
    E<int, string> m = "hello";
    REQUIRE( m.fmap( f ).error() == "hello" );
    m = 7;
    REQUIRE( m.fmap( f ) == 8 );
    REQUIRE( E<int, string>{ "hello" }.fmap( f ).error() ==
             "hello" );
    REQUIRE( E<int, string>{ 3 }.fmap( f ) == 4 );
  }
  SECTION( "string" ) {
    auto           f = []( string s ) { return s + s; };
    E<string, int> m = 5;
    REQUIRE( m.fmap( f ) == 5 );
    m = "7";
    REQUIRE( m.fmap( f ) == "77" );
    REQUIRE( E<string, int>{ 3 }.fmap( f ) == 3 );
    REQUIRE( E<string, int>{ "xy" }.fmap( f ) == "xyxy" );
  }
  SECTION( "NoCopy" ) {
    auto f = []( NoCopy const& nc ) {
      return NoCopy{ char( nc.c + 1 ) };
    };
    E<NoCopy, string> m = "hello";
    REQUIRE( m.fmap( f ).error() == "hello" );
    m = NoCopy{ 'R' };
    REQUIRE( m.fmap( f ) == NoCopy{ 'S' } );
    REQUIRE( E<NoCopy, string>{ "hello" }.fmap( f ).error() ==
             "hello" );
    REQUIRE( E<NoCopy, string>{ 'y' }.fmap( f ) ==
             NoCopy{ 'z' } );
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    auto f = []( Tracker const& nc ) { return nc; };
    E<Tracker, string> m = "hello";
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.fmap( f ).error() == "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.fmap( f ) != string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>{ "hello" }.fmap( f ).error() ==
             "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>( in_place ).fmap( f ) !=
             string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 3 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
  SECTION( "Tracker auto" ) {
    Tracker::reset();
    auto f = []<typename T>( T&& nc ) {
      return std::forward<T>( nc );
    };
    E<Tracker, string> m = "hello";
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.fmap( f ).error() == "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.fmap( f ) != string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>{ "hello" }.fmap( f ).error() ==
             "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>( in_place ).fmap( f ) !=
             string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 3 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 2 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
  SECTION( "pointer to member" ) {
    struct A {
      A( int m ) : n( m ) {}
      int                 get_n() const { return n + 1; }
      expect<int, string> maybe_get_n() const {
        return ( n > 5 ) ? expected<string>( n ) : "hello";
      }
      int n;
    };
    E<A, string> m = "hello";
    REQUIRE( m.fmap( &A::get_n ).error() == "hello" );
    m = A{ 2 };
    REQUIRE( m.fmap( &A::get_n ) == 3 );
    REQUIRE( E<A, string>{ 2 }.fmap( &A::get_n ) == 3 );

    m = "world";
    REQUIRE( m.bind( &A::maybe_get_n ).error() == "world" );
    m = A{ 2 };
    REQUIRE( m.bind( &A::maybe_get_n ).error() == "hello" );
    m = A{ 6 };
    REQUIRE( m.bind( &A::maybe_get_n ) == 6 );
    REQUIRE( E<A, string>{ 6 }.bind( &A::maybe_get_n ) == 6 );
  }
}

TEST_CASE( "[expected] bind" ) {
  SECTION( "int" ) {
    auto f = []( int n ) {
      return ( n > 5 ) ? E<int, string>{ n } : "hello";
    };
    E<int, string> m = "hello";
    REQUIRE( m.bind( f ).error() == "hello" );
    m = 4;
    REQUIRE( m.bind( f ).error() == "hello" );
    m = 6;
    REQUIRE( m.bind( f ) == 6 );
    REQUIRE( E<int, string>{ "hello" }.bind( f ).error() ==
             "hello" );
    REQUIRE( E<int, string>{ 3 }.bind( f ).error() == "hello" );
    REQUIRE( E<int, string>{ 7 }.bind( f ) == 7 );
  }
  SECTION( "string" ) {
    auto f = []( string s ) {
      return ( s.size() > 2 ) ? E<string, int>{ s } : 6;
    };
    E<string, int> m = 3;
    REQUIRE( m.bind( f ).error() == 3 );
    m = "a";
    REQUIRE( m.bind( f ).error() == 6 );
    m = "aaaa";
    REQUIRE( m.bind( f ) == "aaaa" );
    REQUIRE( E<string, int>{ 4 }.bind( f ).error() == 4 );
    REQUIRE( E<string, int>{ "a" }.bind( f ).error() == 6 );
    REQUIRE( E<string, int>{ "aaa" }.bind( f ) == "aaa" );
  }
  SECTION( "NoCopy" ) {
    auto f = []( NoCopy nc ) {
      return ( nc.c == 'g' )
                 ? E<NoCopy, string>{ std::move( nc ) }
                 : "world";
    };
    E<NoCopy, string> m = "hello";
    REQUIRE( E<NoCopy, string>{ "a" }.bind( f ).error() == "a" );
    REQUIRE( E<NoCopy, string>{ 'a' }.bind( f ).error() ==
             "world" );
    REQUIRE( E<NoCopy, string>{ 'g' }.bind( f ) ==
             NoCopy{ 'g' } );
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    auto f = []( Tracker const& nc ) {
      return E<Tracker, string>{ nc };
    };
    E<Tracker, string> m = "hello";
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.bind( f ).error() == "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.bind( f ) != string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>{ "hello" }.bind( f ).error() ==
             "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>( in_place ).bind( f ) !=
             string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
  SECTION( "Tracker auto" ) {
    Tracker::reset();
    auto f = []<typename T>( T&& nc ) {
      return E<Tracker, string>{ std::forward<T>( nc ) };
    };
    E<Tracker, string> m = "hello";
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.bind( f ).error() == "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.bind( f ) != string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>{ "hello" }.bind( f ).error() ==
             "hello" );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( E<Tracker, string>( in_place ).bind( f ) !=
             string( "hello" ) );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
}

TEST_CASE( "[expected] expected" ) {
  auto m1 = expected<string>( 5 );
  ASSERT_VAR_TYPE( m1, expect<int, string> );

  auto m2 = expected<string>( "hello" );
  ASSERT_VAR_TYPE( m2, expect<char const*, string> );

  auto m3 = expected<Empty>( "hello"s );
  ASSERT_VAR_TYPE( m3, expect<string, Empty> );

  string const& s1 = "hello";
  auto          m4 = expected<Empty>( s1 );
  ASSERT_VAR_TYPE( m4, expect<string, Empty> );

  auto m5 = expected<string>( NoCopy{ 'a' } );
  ASSERT_VAR_TYPE( m5, expect<NoCopy, string> );

  auto m6 = expected<NoCopyNoMove, string>( in_place, 'a' );
  ASSERT_VAR_TYPE( m6, expect<NoCopyNoMove, string> );

  auto m7 = expected<string, int>( std::in_place );
  ASSERT_VAR_TYPE( m7, expect<string, int> );
}

TEST_CASE( "[expected] is_value_truish" ) {
  SECTION( "int" ) {
    E<int, string> m = "hello";
    REQUIRE( !m.is_value_truish() );
    m = 0;
    REQUIRE( !m.is_value_truish() );
    m = 1;
    REQUIRE( m.is_value_truish() );
    m = 2;
    REQUIRE( m.is_value_truish() );
    m = -2;
    REQUIRE( m.is_value_truish() );
  }
  SECTION( "bool" ) {
    E<bool, string> m = string( "hello" );
    REQUIRE( !m.is_value_truish() );
    m = false;
    REQUIRE( !m.is_value_truish() );
    m = true;
    REQUIRE( m.is_value_truish() );
  }
  SECTION( "Boolable" ) {
    E<Boolable, Empty> m = Empty{};
    REQUIRE( !m.is_value_truish() );
    m = false;
    REQUIRE( !m.is_value_truish() );
    m = true;
    REQUIRE( m.is_value_truish() );
  }
}

TEST_CASE( "[expected-ref] construction" ) {
  SECTION( "non-const" ) {
    int m = 9;

    E<int&, string> m0 = "hello";
    REQUIRE( !m0.has_value() );
    REQUIRE( m0.error() == "hello" );
    REQUIRE( !bool( m0 ) );
    try {
      (void)m0.value();
      // Should not be here.
      REQUIRE( false );
    } catch( bad_expect_access const& ) {}
    REQUIRE( m0.value_or( m ) == 9 );

    E<int&, string> m1 = "hello";
    REQUIRE( !m1.has_value() );
    REQUIRE( !bool( m1 ) );
    REQUIRE( !m1.is_value_truish() );

    int             n  = 5;
    E<int&, string> m3 = n;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    REQUIRE( m3 == 5 );
    REQUIRE( m3.value() == 5 );
    REQUIRE( m3.value_or( m ) == 5 );
    REQUIRE( bool( m3 ) );
    REQUIRE( m3.is_value_truish() );

    struct B {
      int n = 2;
    };
    struct A : B {
      int m = 3;
    };

    A a;
    B b;

    E<B&, string> m4 = b;
    REQUIRE( m4.has_value() );
    REQUIRE( m4->n == 2 );
    E<B&, string> m5 = a;
    REQUIRE( m5.has_value() );
    REQUIRE( m5->n == 2 );
    REQUIRE( bool( m5 ) );

    E<A&, string> m6 = a;
    REQUIRE( m6.has_value() );
    REQUIRE( m6->n == 2 );
    REQUIRE( m6->m == 3 );
    REQUIRE( m6.value().n == 2 );
    REQUIRE( m6.value().m == 3 );
    REQUIRE( bool( m6 ) );

    E<bool&, string> m7 = "hello";
    REQUIRE( !m7.has_value() );
    REQUIRE( m7.error() == "hello" );

    int             z  = 0;
    E<int&, string> m8 = z;
    REQUIRE( m8 == 0 );
    REQUIRE( !m8.is_value_truish() );
    z = 2;
    REQUIRE( m8 == 2 );
    REQUIRE( m8.is_value_truish() );

    NoCopyNoMove             ncnm{ 'a' };
    E<NoCopyNoMove&, string> m9 = ncnm;
    REQUIRE( m9.has_value() );
    REQUIRE( m9->c == 'a' );

    string            s   = "hello";
    E<string&, Empty> m10 = s;
    REQUIRE( m10.has_value() );
    REQUIRE( m10 == "hello" );
    s = "world";
    REQUIRE( m10 == "world" );
  }
  SECTION( "non-const" ) {
    int m = 9;

    E<int const&, string> m0 = "hello";
    REQUIRE( !m0.has_value() );
    REQUIRE( !bool( m0 ) );
    try {
      (void)m0.value();
      // Should not be here.
      REQUIRE( false );
    } catch( bad_expect_access const& ) {}
    REQUIRE( m0.value_or( m ) == 9 );

    E<int const&, string> m1 = "hello";
    REQUIRE( !m1.has_value() );
    REQUIRE( !bool( m1 ) );
    REQUIRE( !m1.is_value_truish() );

    int                   n  = 5;
    E<int const&, string> m3 = n;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    REQUIRE( m3 == 5 );
    REQUIRE( m3.value() == 5 );
    REQUIRE( m3.value_or( m ) == 5 );
    REQUIRE( bool( m3 ) );
    REQUIRE( m3.is_value_truish() );

    struct B {
      int n = 2;
    };
    struct A : B {
      int m = 3;
    };

    A a;
    B b;

    E<B const&, string> m4 = b;
    REQUIRE( m4.has_value() );
    REQUIRE( m4->n == 2 );
    E<B const&, string> m5 = a;
    REQUIRE( m5.has_value() );
    REQUIRE( m5->n == 2 );
    REQUIRE( bool( m5 ) );

    E<A const&, string> m6 = a;
    REQUIRE( m6.has_value() );
    REQUIRE( m6->n == 2 );
    REQUIRE( m6->m == 3 );
    REQUIRE( m6.value().n == 2 );
    REQUIRE( m6.value().m == 3 );
    REQUIRE( bool( m6 ) );

    E<bool const&, Empty> m7 = Empty{};
    REQUIRE( !m7.has_value() );

    int                   z  = 0;
    E<int const&, string> m8 = z;
    REQUIRE( m8 == 0 );
    REQUIRE( !m8.is_value_truish() );
    z = 2;
    REQUIRE( m8 == 2 );
    REQUIRE( m8.is_value_truish() );

    NoCopyNoMove                   ncnm{ 'a' };
    E<NoCopyNoMove const&, string> m9 = ncnm;
    REQUIRE( m9.has_value() );
    REQUIRE( m9->c == 'a' );

    string                  s   = "hello";
    E<string const&, Empty> m10 = s;
    REQUIRE( m10.has_value() );
    REQUIRE( m10 == "hello" );
    s = "world";
    REQUIRE( m10 == "world" );
  }
}

TEST_CASE( "[expected-ref] fmap" ) {
  SECTION( "ref to value, int" ) {
    int             n  = 5;
    E<int&, string> m1 = n;

    auto inc = []( int m ) { return m + 1; };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<int, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 6 );
  }
  SECTION( "ref to value, string" ) {
    string          n  = "hello";
    E<string&, int> m1 = n;

    auto inc = []( string const& s ) { return s + s; };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<string, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "hellohello" );
  }
  SECTION( "ref to ref, int" ) {
    int             n  = 5;
    E<int&, string> m1 = n;

    auto inc = []( int ) -> int& {
      static int x = 8;
      return x;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<int&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 8 );
  }
  SECTION( "ref to ref, string" ) {
    string          n  = "hello";
    E<string&, int> m1 = n;

    auto inc = []( string const& ) -> string const& {
      static const string s = "world";
      return s;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<string const&, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
  }
  SECTION( "value to ref, int" ) {
    E<int, string> m1 = 5;

    auto inc = []( int ) -> int& {
      static int x = 8;
      return x;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<int&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 8 );
  }
  SECTION( "value to ref, string" ) {
    E<string, int> m1 = "hello";

    auto inc = []( string const& ) -> string const& {
      static const string s = "world";
      return s;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, expect<string const&, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
  }
}

TEST_CASE( "[expected-ref] bind" ) {
  SECTION( "ref to value, int" ) {
    int             n  = 5;
    E<int&, string> m1 = n;

    auto inc = []( int m ) -> E<int, string> {
      if( m < 5 ) return "hello";
      return m + 1;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<int, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 6 );

    n       = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<int, string> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );
  }
  SECTION( "ref to value, string" ) {
    string          n  = "hello";
    E<string&, int> m1 = n;

    auto inc = []( string const& m ) -> E<string, int> {
      if( m != "hello" ) return 5;
      return "world";
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<string, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );

    n       = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<string, int> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == 5 );
  }
  SECTION( "ref to ref, int" ) {
    int             n  = 5;
    E<int&, string> m1 = n;

    auto inc = []( int m ) -> E<int const&, string> {
      static const int x = 3;
      if( m < 5 ) return "hello";
      return x;
    };

    auto const& m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<int const&, string> const& );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
    REQUIRE( *m2 == 3 );

    n       = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<int const&, string> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );
  }
  SECTION( "ref to ref, string" ) {
    string          n  = "hello";
    E<string&, int> m1 = n;

    auto inc = []( string const& m ) -> E<string&, int> {
      static string x = "world";
      if( m != "hello" ) return 5;
      return x;
    };

    auto const& m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<string&, int> const& );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );

    n       = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<string&, int> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == 5 );
  }
  SECTION( "value to ref, int" ) {
    E<int, string> m1 = 5;

    auto inc = []( int m ) -> E<int const&, string> {
      static const int x = 3;
      if( m < 5 ) return "hello";
      return x;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<int const&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
    REQUIRE( *m2 == 3 );

    m1      = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<int const&, string> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3.error() == "hello" );
  }
  SECTION( "value to ref, string" ) {
    E<string, int> m1 = "hello";

    auto inc = []( string const& m ) -> E<string&, int> {
      static string x = "world";
      if( m != "hello" ) return 5;
      return x;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, E<string&, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );

    m1      = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, E<string&, int> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == 5 );
  }
}

TEST_CASE( "[expected-ref] expect-ref" ) {
  SECTION( "int" ) {
    int  n  = 5;
    auto m1 = expected_ref<string>( n );
    ASSERT_VAR_TYPE( m1, E<int&, string> );
    REQUIRE( m1.has_value() );
    REQUIRE( m1 == 5 );
    REQUIRE( *m1 == 5 );

    int const m  = 5;
    auto      m2 = expected_ref<string>( m );
    ASSERT_VAR_TYPE( m2, expect<int const&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 5 );
    REQUIRE( *m2 == 5 );
  }
  SECTION( "string" ) {
    string s  = "hello";
    auto   m1 = expected_ref<int>( s );
    ASSERT_VAR_TYPE( m1, expect<string&, int> );
    REQUIRE( m1.has_value() );
    REQUIRE( m1 == "hello" );
    REQUIRE( *m1 == "hello" );

    string const m  = "world";
    auto         m2 = expected_ref<int>( m );
    ASSERT_VAR_TYPE( m2, expect<string const&, int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );
  }
}

TEST_CASE( "[expected-ref] comparison" ) {
  SECTION( "int" ) {
    int                   n  = 5;
    E<int&, string>       m1 = n;
    int                   m  = 6;
    E<int const&, string> m2 = m;

    REQUIRE( m1.has_value() );
    REQUIRE( m2.has_value() );

    REQUIRE( m1 != m2 );
    m = 5;
    REQUIRE( m1 == m2 );
  }
  SECTION( "string" ) {
    string          n  = "hello";
    E<string&, int> m1 = n;
    string          m  = "world";
    E<string&, int> m2 = m;

    REQUIRE( m1.has_value() );
    REQUIRE( m2.has_value() );

    REQUIRE( m1 != m2 );
    m = "hello";
    REQUIRE( m1 == m2 );
  }
}

TEST_CASE( "[expected-ref] with error" ) {
  int             n  = 5;
  E<int&, string> m1 = n;
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );
  n = 6;
  REQUIRE( *m1 == 6 );
  *m1 = 7;
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 7 );

  E<int&, string> m2 = "hello";
  REQUIRE( !m2.has_value() );
  REQUIRE( m2.error() == "hello" );
}

TEST_CASE( "[expected] ref to member" ) {
  SECTION( "val to ref, member" ) {
    struct A {
      int n;
    };

    E<A, string> m = A{ 4 };

    auto m2 = m.member( &A::n );
    ASSERT_VAR_TYPE( m2, E<int&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    auto m3 = as_const( m ).member( &A::n );
    ASSERT_VAR_TYPE( m3, E<int const&, string> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );
  }
  SECTION( "ref to ref, member" ) {
    struct A {
      int n;
    };

    A             a{ 4 };
    E<A&, string> m = a;

    auto m2 = m.member( &A::n );
    ASSERT_VAR_TYPE( m2, E<int&, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    auto m3 = as_const( m ).member( &A::n );
    ASSERT_VAR_TYPE( m3, E<int&, string> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );

    // Test const.
    E<A const&, string> m4 = a;

    auto m5 = m4.member( &A::n );
    ASSERT_VAR_TYPE( m5, E<int const&, string> );
    REQUIRE( m5.has_value() );
    REQUIRE( m5 == 3 );
    REQUIRE( *m5 == 3 );
  }
  SECTION( "val to ref, maybe_member" ) {
    struct A {
      maybe<int> n;
    };

    E<A, string> m = A{ 4 };

    auto m2 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m2, E<maybe<int&>, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );
    REQUIRE( ( *m2 ).has_value() );
    REQUIRE( ( *m2 ) == 4 );
    REQUIRE( **m2 == 4 );

    // Assign through.
    **m2 = 3;

    auto m3 = as_const( m ).maybe_member( &A::n );
    ASSERT_VAR_TYPE( m3, E<maybe<int const&>, string> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );
    REQUIRE( ( *m3 ).has_value() );
    REQUIRE( ( *m3 ) == 3 );
    REQUIRE( **m3 == 3 );

    **m2 = 4;

    // We can't have a E<T&&>, so that means that when we
    // reference a temporary we use T const&.
    // Edit 2023-03-13: this overload is now deleted.
    // auto m4 = std::move( m ).maybe_member( &A::n );
    // ASSERT_VAR_TYPE( m4, E<maybe<int const&>, string> );
    // REQUIRE( m4.has_value() );
    // REQUIRE( m4 == 4 );
    // REQUIRE( *m4 == 4 );
    // REQUIRE( ( *m4 ).has_value() );
    // REQUIRE( ( *m4 ) == 4 );
    // REQUIRE( **m4 == 4 );
  }
  SECTION( "ref to ref, maybe_member" ) {
    struct A {
      maybe<int> n;
    };

    A             a{};
    E<A&, string> m = a;

    auto m1 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m1, E<maybe<int&>, string> );
    REQUIRE( m1.has_value() );
    REQUIRE( *m1 == nothing );

    a.n = 4;

    auto m2 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m2, E<maybe<int&>, string> );
    REQUIRE( m2.has_value() );
    REQUIRE( **m2 == 4 );

    // Assign through.
    **m2 = 3;

    // consting the maybe object does nothing.
    auto m3 = as_const( m ).maybe_member( &A::n );
    ASSERT_VAR_TYPE( m3, E<maybe<int&>, string> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );
    REQUIRE( **m3 == 3 );

    // consting the inner object does something.
    E<A const&, string> m4 = a;

    auto m5 = m4.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m5, E<maybe<int const&>, string> );
    REQUIRE( m5.has_value() );
    REQUIRE( m5 == 3 );
    REQUIRE( *m5 == 3 );
    REQUIRE( **m5 == 3 );
  }
}

TEST_CASE( "[expected] get_if" ) {
  SECTION( "value, std::variant" ) {
    using V = std::variant<int, string, double>;
    EE<V> m = Empty{};
    REQUIRE( !m.get_if<double>().has_value() );
    SECTION( "int" ) {
      m        = 3;
      auto res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, EE<maybe<int&>> );
      REQUIRE( res.has_value() );
      REQUIRE( res->has_value() );
      **res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
    SECTION( "string" ) {
      m        = "hello";
      auto res = as_const( m ).get_if<string>();
      ASSERT_VAR_TYPE( res, EE<maybe<string const&>> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
  }
  SECTION( "value, base::variant" ) {
    using V = base::variant<int, string, double>;
    EE<V> m = Empty{};
    REQUIRE( !m.get_if<string>().has_value() );
    SECTION( "int" ) {
      m        = 3;
      auto res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, EE<maybe<int&>> );
      REQUIRE( res.has_value() );
      **res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
    SECTION( "string" ) {
      m        = "hello";
      auto res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, EE<maybe<string&>> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
      m = "world";
      REQUIRE( m == V{ "world" } );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
  }
  SECTION( "ref, std::variant" ) {
    using V = std::variant<int, string, double>;
    SECTION( "nothing" ) {
      EE<V&> m = Empty{};
      REQUIRE( !m.get_if<string>().has_value() );
    }
    SECTION( "int" ) {
      V      v   = 3;
      EE<V&> m   = v;
      auto   res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, EE<maybe<int&>> );
      REQUIRE( res.has_value() );
      **res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
    SECTION( "string" ) {
      V      v   = "hello";
      EE<V&> m   = v;
      auto   res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, EE<maybe<string&>> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
  }
  SECTION( "ref, base::variant" ) {
    using V = base::variant<int, string, double>;
    SECTION( "int" ) {
      V      v   = 3;
      EE<V&> m   = v;
      auto   res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, EE<maybe<int&>> );
      REQUIRE( res.has_value() );
      **res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
    SECTION( "string" ) {
      V const      v   = "hello";
      EE<V const&> m   = v;
      auto         res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, EE<maybe<string const&>> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>().has_value() );
      REQUIRE( !m.get_if<double>()->has_value() );
    }
  }
}

TEST_CASE( "[expected] implicit conversion to expected-ref" ) {
  {
    E<int, string>  m1 = 5;
    E<int&, string> m2 = m1;
    E<int&, string> m3 = static_cast<E<int&, string>>( m1 );
    (void)m2;
    (void)m3;
    static_assert(
        is_convertible_v<E<int, string>, E<int&, string>> );
    REQUIRE( m2 == 5 );
    *m1 = 6;
    REQUIRE( m2 == 6 );
  }
  {
    // Not allowed.
    // E<int&> m2 = as_const( m1 );
    // E<int&> m3 = static_cast<E<int&>>( as_const( m1 ) );
    static_assert( !is_convertible_v<E<int, string> const,
                                     E<int&, string>> );
  }
  {
    E<int, string>        m1 = 5;
    E<int const&, string> m2 = as_const( m1 );
    E<int const&, string> m3 =
        static_cast<E<int const&, string>>( as_const( m1 ) );
    (void)m2;
    (void)m3;
    static_assert( is_convertible_v<E<int, string> const,
                                    E<int const&, string>> );
    REQUIRE( m2 == 5 );
    *m1 = 6;
    REQUIRE( m2 == 6 );
  }
}

TEST_CASE( "[expected] stringification" ) {
  E<int, string>  e1 = 5;
  E<int, string>  e2 = "hello";
  int             n  = 3;
  E<int&, string> e3 = n;

  REQUIRE( fmt::format( "{}", e1 ) == "5" );
  REQUIRE( fmt::format( "{}", e2 ) == "unexpected{hello}" );
  REQUIRE( fmt::format( "{}", e3 ) == "3" );

  REQUIRE( to_str( e1 ) == "5" );
  REQUIRE( to_str( e2 ) == "unexpected{hello}" );
  REQUIRE( to_str( e3 ) == "3" );
}

} // namespace
} // namespace base
