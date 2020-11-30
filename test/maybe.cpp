/****************************************************************
**maybe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-28.
*
* Description: An alternative to std::optional, used in the RN
*              code base.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "base/maybe.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <experimental/type_traits>
#include <functional>

namespace base {
namespace {

using namespace std;

using ::Catch::Equals;
using ::Catch::Matchers::Contains;
using ::std::experimental::is_detected_v;

template<typename T>
using M = ::base::maybe<T>;

template<typename T>
using RR = ::std::reference_wrapper<T>;

/****************************************************************
** Tracker
*****************************************************************/
// Tracks number of constructions and destructions.
struct Tracker {
  static int  constructed;
  static int  destructed;
  static int  copied;
  static int  move_constructed;
  static int  move_assigned;
  static void reset() {
    constructed = destructed = copied = move_constructed =
        move_assigned                 = 0;
  }

  Tracker() noexcept { ++constructed; }
  Tracker( Tracker const& ) noexcept { ++copied; }
  Tracker( Tracker&& ) noexcept { ++move_constructed; }
  ~Tracker() noexcept { ++destructed; }

  Tracker& operator=( Tracker const& ) = delete;
  Tracker& operator                    =( Tracker&& ) noexcept {
    ++move_assigned;
    return *this;
  }
};
int Tracker::constructed      = 0;
int Tracker::destructed       = 0;
int Tracker::copied           = 0;
int Tracker::move_constructed = 0;
int Tracker::move_assigned    = 0;

/****************************************************************
** Constexpr type
*****************************************************************/
struct Constexpr {
  constexpr Constexpr() = default;
  constexpr Constexpr( int n_ ) noexcept : n( n_ ) {}
  constexpr Constexpr( Constexpr const& ) = default;
  constexpr Constexpr( Constexpr&& )      = default;
  constexpr Constexpr& operator=( Constexpr const& ) = default;
  constexpr Constexpr& operator=( Constexpr&& )       = default;
  constexpr bool operator==( Constexpr const& ) const = default;

  int n;
};

/****************************************************************
** Non-Copyable
*****************************************************************/
struct NoCopy {
  explicit NoCopy( char c_ ) : c( c_ ) {}
  NoCopy( NoCopy const& ) = delete;
  NoCopy( NoCopy&& )      = default;
  NoCopy& operator=( NoCopy const& ) = delete;
  NoCopy& operator=( NoCopy&& )              = default;
  bool    operator==( NoCopy const& ) const& = default;
  char    c;
};
static_assert( !is_copy_constructible_v<NoCopy> );
static_assert( is_move_constructible_v<NoCopy> );
static_assert( !is_copy_assignable_v<NoCopy> );
static_assert( is_move_assignable_v<NoCopy> );

/****************************************************************
** Non-Copyable, Non-Movable
*****************************************************************/
struct NoCopyNoMove {
  NoCopyNoMove( char c_ ) : c( c_ ) {}
  NoCopyNoMove( NoCopyNoMove const& ) = delete;
  NoCopyNoMove( NoCopyNoMove&& )      = delete;
  NoCopyNoMove& operator=( NoCopyNoMove const& ) = delete;
  NoCopyNoMove& operator=( NoCopyNoMove&& ) = delete;
  NoCopyNoMove& operator=( char c_ ) noexcept {
    c = c_;
    return *this;
  }
  bool operator==( NoCopyNoMove const& ) const& = default;
  char c;
};
static_assert( !is_copy_constructible_v<NoCopyNoMove> );
static_assert( !is_move_constructible_v<NoCopyNoMove> );
static_assert( !is_copy_assignable_v<NoCopyNoMove> );
static_assert( !is_move_assignable_v<NoCopyNoMove> );

/****************************************************************
** Thrower
*****************************************************************/
struct Throws {
  Throws( bool should_throw = true ) {
    if( should_throw )
      throw runtime_error( "default construction" );
  }
  Throws( Throws const& ) {
    throw runtime_error( "copy construction" );
  }
  Throws( Throws&& ) {
    throw runtime_error( "move construction" );
  }
  Throws& operator=( Throws const& ) {
    throw runtime_error( "copy assignment" );
  }
  Throws& operator=( Throws&& ) {
    throw runtime_error( "move assignment" );
  }
};

/****************************************************************
** Trivial Everything
*****************************************************************/
struct Trivial {
  Trivial()                 = default;
  ~Trivial()                = default;
  Trivial( Trivial const& ) = default;
  Trivial( Trivial&& )      = default;
  Trivial& operator=( Trivial const& ) = default;
  Trivial& operator=( Trivial&& ) = default;

  double d;
  int    n;
};

/****************************************************************
** Convertibles
*****************************************************************/
struct Intable {
  Intable() = default;
  Intable( int m ) : n( m ) {}
  // clang-format off
  operator int() const { return n; }
  // clang-format on
  int n = {};
};

struct Stringable {
  Stringable() = default;
  Stringable( string s_ ) : s( s_ ) {}
  // clang-format off
  operator string() const { return s; }
  // clang-format on
  string s = {};
};

/****************************************************************
** [static] nothing_t
*****************************************************************/
constexpr nothing_t n0thing( 0 );
static_assert( sizeof( decltype( n0thing ) ) == 1 );

/****************************************************************
** [static] Invalid value types.
*****************************************************************/
static_assert( is_detected_v<M, int> );
static_assert( is_detected_v<M, string> );
static_assert( is_detected_v<M, NoCopy> );
static_assert( is_detected_v<M, NoCopyNoMove> );
static_assert( is_detected_v<M, double> );
static_assert( !is_detected_v<M, std::in_place_t> );
static_assert( !is_detected_v<M, std::in_place_t&> );
static_assert( !is_detected_v<M, std::in_place_t const&> );
static_assert( !is_detected_v<M, nothing_t> );
static_assert( !is_detected_v<M, nothing_t&> );
static_assert( !is_detected_v<M, nothing_t const&> );

/****************************************************************
** [static] Propagation of noexcept.
*****************************************************************/
// `int` should always be nothrow.
static_assert( is_nothrow_default_constructible_v<M<int>> );
static_assert( is_nothrow_constructible_v<M<int>> );
static_assert( is_nothrow_constructible_v<M<int>, int> );
static_assert( is_nothrow_constructible_v<M<int>, nothing_t> );
static_assert( is_nothrow_move_constructible_v<M<int>> );
static_assert( is_nothrow_move_assignable_v<M<int>> );
static_assert( is_nothrow_copy_constructible_v<M<int>> );
static_assert( is_nothrow_copy_assignable_v<M<int>> );

// `string` should only throw on copies.
static_assert( is_nothrow_default_constructible_v<M<string>> );
static_assert( is_nothrow_constructible_v<M<string>> );
static_assert( is_nothrow_constructible_v<M<string>, string> );
static_assert(
    is_nothrow_constructible_v<M<string>, nothing_t> );
static_assert( is_nothrow_move_constructible_v<M<string>> );
static_assert( is_nothrow_move_assignable_v<M<string>> );
static_assert( !is_nothrow_copy_constructible_v<M<string>> );
static_assert( !is_nothrow_copy_assignable_v<M<string>> );

// Always throws except on default construction or equivalent.
static_assert( is_nothrow_default_constructible_v<M<Throws>> );
static_assert(
    is_nothrow_constructible_v<M<Throws>, nothing_t> );
static_assert( !is_nothrow_constructible_v<M<Throws>, Throws> );
static_assert( !is_nothrow_move_constructible_v<M<Throws>> );
static_assert( !is_nothrow_move_assignable_v<M<Throws>> );
static_assert( !is_nothrow_copy_constructible_v<M<Throws>> );
static_assert( !is_nothrow_copy_assignable_v<M<Throws>> );

/****************************************************************
** [static] Propagation of triviality.
*****************************************************************/
// p0848r3.html
#ifdef HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
static_assert( is_trivially_copy_constructible_v<M<int>> );
static_assert( is_trivially_move_constructible_v<M<int>> );
static_assert( is_trivially_copy_assignable_v<M<int>> );
static_assert( is_trivially_move_assignable_v<M<int>> );
static_assert( is_trivially_destructible_v<M<int>> );

static_assert( is_trivially_copy_constructible_v<M<Trivial>> );
static_assert( is_trivially_move_constructible_v<M<Trivial>> );
static_assert( is_trivially_copy_assignable_v<M<Trivial>> );
static_assert( is_trivially_move_assignable_v<M<Trivial>> );
static_assert( is_trivially_destructible_v<M<Trivial>> );

static_assert( !is_trivially_copy_constructible_v<M<string>> );
static_assert( !is_trivially_move_constructible_v<M<string>> );
static_assert( !is_trivially_copy_assignable_v<M<string>> );
static_assert( !is_trivially_move_assignable_v<M<string>> );
static_assert( !is_trivially_destructible_v<M<string>> );
#endif

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[maybe] default construction" ) {
  SECTION( "int" ) {
    Tracker::reset();
    {
      M<int> m;
      REQUIRE( !m.has_value() );
      M<int> m2 = {};
      REQUIRE( !m2.has_value() );

      // Verify that the value type does not get instantiated
      // upon default construction of the maybe.
      M<Tracker> construction_tracker;
      REQUIRE( Tracker::constructed == 0 );
      REQUIRE( Tracker::destructed == 0 );
      construction_tracker = Tracker{};
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 1 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
  }
}

TEST_CASE( "[maybe] has_value/bool" ) {
  M<int> m1;
  REQUIRE( !m1.has_value() );
  REQUIRE( !bool( m1 ) );
  m1 = 5;
  REQUIRE( m1.has_value() );
  REQUIRE( bool( m1 ) );
  m1.reset();
  REQUIRE( !m1.has_value() );
  REQUIRE( !bool( m1 ) );

  if( m1 ) {
    REQUIRE( false );
  } else {
    REQUIRE( true );
  }
}

TEST_CASE( "[maybe] reset" ) {
  SECTION( "int" ) {
    M<int> m;
    REQUIRE( !m.has_value() );
    m.reset();
    REQUIRE( !m.has_value() );
    m = 5;
    REQUIRE( m.has_value() );
    m.reset();
    REQUIRE( !m.has_value() );
    m.reset();
    REQUIRE( !m.has_value() );
  }
  SECTION( "TruckionTracker" ) {
    Tracker::reset();
    {
      M<Tracker> m;
      REQUIRE( !m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      m.emplace();
      REQUIRE( m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 1 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 1 );
  }
}

TEST_CASE( "[maybe] T with no default constructor" ) {
  struct A {
    A() = delete;
    A( int m ) : n( m ) {}
    int n;
  };

  M<A> m;
  REQUIRE( !m.has_value() );
  m = A{ 2 };
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 2 );
  A a( 6 );
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 6 );
}

TEST_CASE( "[maybe] value construction" ) {
  M<int> m( 5 );
  REQUIRE( m.has_value() );
  REQUIRE( *m == 5 );

  M<string> m2( "hello" );
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == "hello" );

  M<string> m3( string( "hello" ) );
  REQUIRE( m3.has_value() );
  REQUIRE( *m3 == "hello" );

  M<NoCopy> m4( NoCopy( 'h' ) );
  REQUIRE( m4.has_value() );
  REQUIRE( *m4 == NoCopy{ 'h' } );
}

TEST_CASE( "[maybe] converting value construction" ) {
  struct A {
    A() = default;
    operator int() const { return 7; }
  };

  A a;

  M<int> m1{ a };
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 7 );

  M<int> m2{ A{} };
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == 7 );
}

TEST_CASE( "[maybe] copy construction" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m3( m );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );

    M<int> m4;
    M<int> m5( m4 );
    REQUIRE( !m5.has_value() );
  }
  SECTION( "string" ) {
    M<string> m{ string( "hello" ) };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m3( m );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );

    M<string> m4;
    M<string> m5( m4 );
    REQUIRE( !m5.has_value() );
  }
}

TEST_CASE( "[maybe] converting copy construction" ) {
  SECTION( "int" ) {
    M<Intable> m1;
    M<int>     m2( m1 );
    REQUIRE( !m2.has_value() );

    M<Intable> m3 = Intable{ 5 };
    M<int>     m4( m3 );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 5 );
  }
  SECTION( "string" ) {
    M<Stringable> m1;
    M<string>     m2( m1 );
    REQUIRE( !m2.has_value() );

    M<Stringable> m3 = Stringable{ "hello" };
    M<string>     m4( m3 );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hello" );
  }
}

TEST_CASE( "[maybe] state after move" ) {
  M<int> m;
  REQUIRE( !m.has_value() );

  M<int> m2 = std::move( m );
  REQUIRE( !m2.has_value() );
  REQUIRE( !m.has_value() );

  m = 5;
  REQUIRE( m.has_value() );

  M<int> m3{ std::move( m ) };
  REQUIRE( m3.has_value() );
  // `maybe`s (like `optional`s) with values that are moved from
  // still have values.
  REQUIRE( m.has_value() );
}

TEST_CASE( "[maybe] move construction" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    // `maybe`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m.has_value() );

    M<int> m3( M<int>{} );
    REQUIRE( !m3.has_value() );

    M<int> m4( M<int>{ 0 } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 0 );

    M<int> m5( std::move( m4 ) );
    // `maybe`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == 0 );

    M<int> m6{ std::move( m2 ) };
    REQUIRE( m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == 4 );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );

    M<string> m3( M<string>{} );
    REQUIRE( !m3.has_value() );

    M<string> m4( M<string>{ "hellm2" } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hellm2" );

    M<string> m5( std::move( m4 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == "hellm2" );

    M<string> m6{ std::move( m2 ) };
    REQUIRE( m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == "hello" );
  }
}

TEST_CASE( "[maybe] converting move construction" ) {
  SECTION( "int" ) {
    M<Intable> m1;
    M<int>     m2( std::move( m1 ) );
    REQUIRE( !m2.has_value() );

    M<Intable> m3 = Intable{ 5 };
    M<int>     m4( std::move( m3 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 5 );
  }
  SECTION( "string" ) {
    M<Stringable> m1;
    M<string>     m2( std::move( m1 ) );
    REQUIRE( !m2.has_value() );

    M<Stringable> m3 = Stringable{ "hello" };
    M<string>     m4( std::move( m3 ) );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hello" );
  }
}

TEST_CASE( "[maybe] in place construction" ) {
  struct A {
    A( int n_, string s_, double d_ )
      : n( n_ ), s( s_ ), d( d_ ) {}
    int    n;
    string s;
    double d;
  };

  M<A> m( in_place, 5, "hello", 4.5 );
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 5 );
  REQUIRE( m->s == "hello" );
  REQUIRE( m->d == 4.5 );

  M<vector<int>> m2( in_place, { 4, 5 } );
  REQUIRE( m2.has_value() );
  REQUIRE_THAT( *m2, Equals( vector<int>{ 4, 5 } ) );
  REQUIRE( m2->size() == 2 );

  M<vector<int>> m3( in_place, 4, 5 );
  REQUIRE( m3.has_value() );
  REQUIRE_THAT( *m3, Equals( vector<int>{ 5, 5, 5, 5 } ) );
  REQUIRE( m3->size() == 4 );
}

TEST_CASE( "[maybe] copy assignment" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2;
    m2 = m;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = m2;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2;
    m2 = m;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = m2;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
  }
}

TEST_CASE( "[maybe] move assignment" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2;
    m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    // `maybe`s (like `optional`s) with values that are moved
    // from still have values.
    REQUIRE( m.has_value() );

    M<int> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( m2.has_value() );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2;
    m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );

    M<string> m3( "yes" );
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
      M<Tracker> m1;
      M<Tracker> m2;
      m1 = std::move( m2 );
      REQUIRE( Tracker::constructed == 0 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 0 );
    {
      Tracker::reset();
      M<Tracker> m1;
      m1.emplace();
      M<Tracker> m2;
      m1 = std::move( m2 );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 1 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 1 );
    {
      Tracker::reset();
      M<Tracker> m1;
      M<Tracker> m2;
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
      M<Tracker> m1;
      m1.emplace();
      M<Tracker> m2;
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

TEST_CASE( "[maybe] converting assignments" ) {
  M<int> m = 5;
  REQUIRE( m.has_value() );
  REQUIRE( *m == 5 );

  Intable a{ 7 };
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( *m == 7 );

  m = Intable{ 9 };
  REQUIRE( m.has_value() );
  REQUIRE( *m == 9 );

  M<Intable> m2;
  REQUIRE( !m2.has_value() );
  m2 = Intable{ 3 };

  m = m2;
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == 3 );

  m = std::move( m2 );
  REQUIRE( m.has_value() );
  REQUIRE( m2.has_value() );
  REQUIRE( *m == 3 );
}

TEST_CASE( "[maybe] dereference" ) {
  M<int> m1 = 5;
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );

  static_assert( is_same_v<decltype( *m1 ), int&> );
  static_assert(
      is_same_v<decltype( as_const( *m1 ) ), int const&> );

  static_assert( is_same_v<decltype( *M<int>{} ), int&&> );

  M<NoCopy> m2{ 'c' };
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == NoCopy{ 'c' } );
  REQUIRE( m2->c == 'c' );

  static_assert( is_same_v<decltype( *m2 ), NoCopy&> );
  static_assert(
      is_same_v<decltype( as_const( *m2 ) ), NoCopy const&> );

  static_assert( is_same_v<decltype( *M<NoCopy>{} ), NoCopy&&> );

  static_assert( is_same_v<decltype( m2->c ), char> );
  static_assert(
      is_same_v<decltype( as_const( m2 )->c ), char> );

  static_assert( is_same_v<decltype( &m2->c ), char*> );
  static_assert(
      is_same_v<decltype( &as_const( m2 )->c ), char const*> );
}

TEST_CASE( "[maybe] emplace" ) {
  SECTION( "int" ) {
    M<int> m;
    REQUIRE( !m.has_value() );
    m.emplace( 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );
    m.emplace( 0 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 0 );
  }
  SECTION( "string" ) {
    M<string> m;
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
      A( int n_, string s_, double d_ )
        : n( n_ ), s( s_ ), d( d_ ) {}
      int    n;
      string s;
      double d;
    };
    M<A> m;
    REQUIRE( !m.has_value() );
    m.emplace( 5, "hello", 4.0 );
    REQUIRE( m.has_value() );
    REQUIRE( m->n == 5 );
    REQUIRE( m->s == "hello" );
    REQUIRE( m->d == 4.0 );
  }
  SECTION( "non-copy non-movable" ) {
    NoCopyNoMove    ncnm( 'b' );
    M<NoCopyNoMove> m;
    REQUIRE( !m.has_value() );
    m.emplace( 'g' );
    REQUIRE( m.has_value() );
    REQUIRE( *m == NoCopyNoMove( 'g' ) );
  }
  SECTION( "initializer list" ) {
    M<vector<int>> m;
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
      M<Tracker> m;
      m.emplace();
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 0 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 0 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 1 );
  }
  SECTION( "exception safety" ) {
    // The exception safety says that if the constructor throws
    // during emplace that the object will be left without an ob-
    // ject.
    M<Throws> m;
    m.emplace( /*should_throw=*/false );
    REQUIRE( m.has_value() );
    try {
      m.emplace( /*should_throw=*/true );
      // Should not be here.
      REQUIRE( false );
    } catch( std::runtime_error const& e ) {
      REQUIRE( e.what() == "default construction"s );
    }
    // The test.
    REQUIRE( !m.has_value() );
  }
}

TEST_CASE( "[maybe] swap" ) {
  SECTION( "int" ) {
    SECTION( "both empty" ) {
      M<int> m1;
      M<int> m2;
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
    }
    SECTION( "both have values" ) {
      M<int> m1 = 7;
      M<int> m2 = 9;
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
    SECTION( "one has value" ) {
      M<int> m1;
      M<int> m2 = 9;
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 9 );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( *m1 == 9 );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == 9 );
    }
  }
  SECTION( "string" ) {
    SECTION( "both empty" ) {
      M<string> m1;
      M<string> m2;
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
    }
    SECTION( "both have values" ) {
      M<string> m1 = "hello";
      M<string> m2 = "world";
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
    SECTION( "one has value" ) {
      M<string> m1;
      M<string> m2 = "world";
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "world" );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( *m1 == "world" );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == "world" );
    }
  }
  SECTION( "non-copyable" ) {
    SECTION( "both empty" ) {
      M<NoCopy> m1;
      M<NoCopy> m2;
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
      m1.swap( m2 );
      REQUIRE( !m1.has_value() );
      REQUIRE( !m2.has_value() );
    }
    SECTION( "both have values" ) {
      M<NoCopy> m1 = NoCopy{ 'h' };
      M<NoCopy> m2 = NoCopy{ 'w' };
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
    SECTION( "one has value" ) {
      M<NoCopy> m1;
      M<NoCopy> m2 = NoCopy{ 'w' };
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == NoCopy{ 'w' } );
      m1.swap( m2 );
      REQUIRE( m1.has_value() );
      REQUIRE( !m2.has_value() );
      REQUIRE( *m1 == NoCopy{ 'w' } );
      m2.swap( m1 );
      REQUIRE( !m1.has_value() );
      REQUIRE( m2.has_value() );
      REQUIRE( *m2 == NoCopy{ 'w' } );
    }
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    SECTION( "both empty" ) {
      {
        M<Tracker> m1;
        M<Tracker> m2;
        m1.swap( m2 );
        REQUIRE( Tracker::constructed == 0 );
        REQUIRE( Tracker::destructed == 0 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 0 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 0 );
    }
    SECTION( "both have values" ) {
      {
        M<Tracker> m1;
        M<Tracker> m2;
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
    SECTION( "one has value" ) {
      {
        M<Tracker> m1;
        M<Tracker> m2;
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 1 );
        REQUIRE( Tracker::destructed == 2 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 2 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 3 );
    }
    SECTION( "other one has value" ) {
      {
        M<Tracker> m1;
        M<Tracker> m2;
        m1.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( Tracker::constructed == 1 );
        REQUIRE( Tracker::destructed == 2 );
        REQUIRE( Tracker::copied == 0 );
        REQUIRE( Tracker::move_constructed == 2 );
        REQUIRE( Tracker::move_assigned == 0 );
      }
      REQUIRE( Tracker::destructed == 3 );
    }
  }
}

TEST_CASE( "[maybe] value_or" ) {
  SECTION( "int" ) {
    M<int> m;
    REQUIRE( m.value_or( 4 ) == 4 );
    m = 6;
    REQUIRE( m.value_or( 4 ) == 6 );
    m.reset();
    REQUIRE( m.value_or( 4 ) == 4 );
    REQUIRE( M<int>{ 4 }.value_or( 5 ) == 4 );
    REQUIRE( M<int>{}.value_or( 5 ) == 5 );
  }
  SECTION( "string" ) {
    M<string> m;
    REQUIRE( m.value_or( "hello" ) == "hello" );
    m = "world";
    REQUIRE( m.value_or( "hello" ) == "world" );
    m.reset();
    REQUIRE( m.value_or( "hello" ) == "hello" );
    REQUIRE( M<string>{ "hello" }.value_or( "world" ) ==
             "hello" );
    REQUIRE( M<string>{}.value_or( "world" ) == "world" );
  }
  SECTION( "NoCopy" ) {
    REQUIRE( M<NoCopy>{ 'c' }.value_or( NoCopy{ 'v' } ) ==
             NoCopy{ 'c' } );
    REQUIRE( M<NoCopy>{}.value_or( NoCopy{ 'v' } ) ==
             NoCopy{ 'v' } );
  }
  SECTION( "Tracker" ) {
    {
      Tracker::reset();
      M<Tracker> m;
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
      M<Tracker> m;
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
      (void)M<Tracker>{}.value_or( Tracker{} );
      REQUIRE( Tracker::constructed == 1 );
      REQUIRE( Tracker::destructed == 2 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 1 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 2 );
    {
      Tracker::reset();
      (void)M<Tracker>{ Tracker{} }.value_or( Tracker{} );
      REQUIRE( Tracker::constructed == 2 );
      REQUIRE( Tracker::destructed == 4 );
      REQUIRE( Tracker::copied == 0 );
      REQUIRE( Tracker::move_constructed == 2 );
      REQUIRE( Tracker::move_assigned == 0 );
    }
    REQUIRE( Tracker::destructed == 4 );
  }
}

TEST_CASE( "[maybe] non-copyable non-movable T" ) {
  M<NoCopyNoMove> m;
  REQUIRE( !m.has_value() );
  m = 'c';
  REQUIRE( m.has_value() );
  REQUIRE( *m == NoCopyNoMove{ 'c' } );
  m.reset();
  REQUIRE( !m.has_value() );
}

TEST_CASE( "[maybe] deduction guides" ) {
  SECTION( "implicit" ) {
    auto m1 = maybe{ 5 };
    static_assert( is_same_v<decltype( m1 ), M<int>> );
    auto m2 = maybe{ string( "hello" ) };
    static_assert( is_same_v<decltype( m2 ), M<string>> );
  }
  // These would fail if we didn't have the explicit deduction
  // guide.
  SECTION( "explicit" ) {
    int  arr[6];
    auto m1 = maybe{ arr };
    static_assert( is_same_v<decltype( m1 ), M<int*>> );
    auto m2 = maybe{ "hello" };
    static_assert( is_same_v<decltype( m2 ), M<char const*>> );
  }
}

TEST_CASE( "[maybe] std::swap overload" ) {
  M<int> m1;
  M<int> m2 = 5;
  REQUIRE( !m1.has_value() );
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == 5 );

  std::swap( m1, m2 );

  REQUIRE( m1.has_value() );
  REQUIRE( !m2.has_value() );
  REQUIRE( *m1 == 5 );
}

TEST_CASE( "[maybe] make_maybe" ) {
  auto m1 = make_maybe( 5 );
  static_assert( is_same_v<decltype( m1 ), M<int>> );
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );
  auto m2 = make_maybe( NoCopy{ 'c' } );
  static_assert( is_same_v<decltype( m2 ), M<NoCopy>> );
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == NoCopy( 'c' ) );

  auto m3 = make_maybe<NoCopy>( 'c' );
  REQUIRE( m3.has_value() );
  REQUIRE( *m3 == NoCopy{ 'c' } );

  auto m4 = make_maybe<vector<int>>( { 4, 5 } );
  REQUIRE( m4.has_value() );
  REQUIRE( m4->size() == 2 );
  REQUIRE_THAT( *m4, Equals( vector<int>{ 4, 5 } ) );

  auto m5 = make_maybe<vector<int>>( 4, 5 );
  REQUIRE( m5.has_value() );
  REQUIRE( m5->size() == 4 );
  REQUIRE_THAT( *m5, Equals( vector<int>{ 5, 5, 5, 5 } ) );

  // T need not be movable due to guaranteed copy elision.
  auto m6 = make_maybe<NoCopyNoMove>( 'd' );
  REQUIRE( m6.has_value() );
  REQUIRE( m6->c == 'd' );
}

TEST_CASE( "[maybe] equality" ) {
  SECTION( "int" ) {
    M<int> m1;
    M<int> m2;
    M<int> m3 = 5;
    M<int> m4 = 7;
    M<int> m5 = 5;

    REQUIRE( m1 == nothing );
    REQUIRE( m2 == nothing );
    REQUIRE( m3 != nothing );
    REQUIRE( m4 != nothing );
    REQUIRE( m5 != nothing );

    REQUIRE( m1 == m1 );
    REQUIRE( m1 == m2 );
    REQUIRE( m1 != m3 );
    REQUIRE( m1 != m4 );
    REQUIRE( m1 != m5 );

    REQUIRE( m2 == m1 );
    REQUIRE( m2 == m2 );
    REQUIRE( m2 != m3 );
    REQUIRE( m2 != m4 );
    REQUIRE( m2 != m5 );

    REQUIRE( m3 != m1 );
    REQUIRE( m3 != m2 );
    REQUIRE( m3 == m3 );
    REQUIRE( m3 != m4 );
    REQUIRE( m3 == m5 );

    REQUIRE( m4 != m1 );
    REQUIRE( m4 != m2 );
    REQUIRE( m4 != m3 );
    REQUIRE( m4 == m4 );
    REQUIRE( m4 != m5 );

    REQUIRE( m5 != m1 );
    REQUIRE( m5 != m2 );
    REQUIRE( m5 == m3 );
    REQUIRE( m5 != m4 );
    REQUIRE( m5 == m5 );
  }
  SECTION( "string" ) {
    M<string> m1;
    M<string> m2;
    M<string> m3 = "hello";
    M<string> m4 = "world";
    M<string> m5 = "hello";

    REQUIRE( nothing == m1 );
    REQUIRE( nothing == m2 );
    REQUIRE( nothing != m3 );
    REQUIRE( nothing != m4 );
    REQUIRE( nothing != m5 );

    REQUIRE( m1 == m1 );
    REQUIRE( m1 == m2 );
    REQUIRE( m1 != m3 );
    REQUIRE( m1 != m4 );
    REQUIRE( m1 != m5 );

    REQUIRE( m2 == m1 );
    REQUIRE( m2 == m2 );
    REQUIRE( m2 != m3 );
    REQUIRE( m2 != m4 );
    REQUIRE( m2 != m5 );

    REQUIRE( m3 != m1 );
    REQUIRE( m3 != m2 );
    REQUIRE( m3 == m3 );
    REQUIRE( m3 != m4 );
    REQUIRE( m3 == m5 );

    REQUIRE( m4 != m1 );
    REQUIRE( m4 != m2 );
    REQUIRE( m4 != m3 );
    REQUIRE( m4 == m4 );
    REQUIRE( m4 != m5 );

    REQUIRE( m5 != m1 );
    REQUIRE( m5 != m2 );
    REQUIRE( m5 == m3 );
    REQUIRE( m5 != m4 );
    REQUIRE( m5 == m5 );
  }
}

TEST_CASE( "[maybe] comparison with value" ) {
  M<string> m;
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

TEST_CASE( "[maybe] value()" ) {
  SECTION( "int" ) {
    M<int> m1;
    try {
      (void)m1.value( SourceLoc{} );
      // Should not be here.
      REQUIRE( false );
    } catch( bad_maybe_access const& e ) {
      REQUIRE_THAT( e.what(), Contains( fmt::format(
                                  "unknown:0: value() called on "
                                  "an inactive maybe" ) ) );
    }

    m1 = 5;
    REQUIRE( m1.value() == 5 );
    const M<int> m2 = 5;
    REQUIRE( m2.value() == 5 );
  }
  SECTION( "NoCopy" ) {
    M<NoCopy> m;
    m.emplace( 'a' );
    REQUIRE( m.has_value() );
    REQUIRE( m->c == 'a' );

    NoCopy nc = std::move( m ).value();
    REQUIRE( nc.c == 'a' );
  }
}

TEST_CASE( "[maybe] nothing_t" ) {
  M<int> m1( nothing );
  REQUIRE( m1 == nothing );
  REQUIRE( !m1.has_value() );

  m1 = 5;
  REQUIRE( m1.has_value() );
  REQUIRE( m1 != nothing );
  REQUIRE( *m1 == 5 );

  m1 = nothing;
  REQUIRE( !m1.has_value() );
  REQUIRE( m1 == nothing );
}

consteval M<int> Calculate1( M<int> num, M<int> den ) {
  if( !num ) return nothing;
  if( !den ) return nothing;
  if( *den == 0 ) return nothing;
  return ( *num ) / ( *den );
}

consteval M<Constexpr> Calculate2( M<Constexpr> num,
                                   M<Constexpr> den ) {
  if( !num ) return nothing;
  if( !den ) return nothing;
  if( den->n == 0 ) return nothing;
  return Constexpr( num->n / den->n );
}

TEST_CASE( "[maybe] consteexpr" ) {
  SECTION( "int" ) {
    constexpr M<int> res1 = Calculate1( 8, 4 );
    static_assert( res1 );
    static_assert( *res1 == 2 );
    constexpr M<int> res2 = Calculate1( nothing, 4 );
    static_assert( res2 == nothing );
    static_assert( !res2 );
    constexpr M<int> res3 = Calculate1( 8, nothing );
    static_assert( res3 == nothing );
    static_assert( !res3 );
    constexpr M<int> res4 = Calculate1( nothing, nothing );
    static_assert( res4 == nothing );
    static_assert( !res4 );
    constexpr M<int> res5 = Calculate1( 8, 0 );
    static_assert( res5 == nothing );
    static_assert( !res5 );
    constexpr M<int> res6 = Calculate1( 9, 4 );
    static_assert( res6 );
    static_assert( *res6 == 2 );

    constexpr M<int> res7 = res6;
    static_assert( res7 );
    static_assert( *res7 == 2 );

    static_assert( res7 == res6 );

    constexpr M<int> res8 = M<int>{ 5 };
    static_assert( res8 );
    static_assert( *res8 == 5 );
  }
  SECTION( "Constexpr type" ) {
    constexpr M<Constexpr> res1 =
        Calculate2( Constexpr{ 8 }, Constexpr{ 4 } );
    static_assert( res1 );
    static_assert( res1->n == 2 );
    constexpr M<Constexpr> res2 =
        Calculate2( nothing, Constexpr{ 4 } );
    static_assert( res2 == nothing );
    static_assert( !res2 );
    constexpr M<Constexpr> res3 =
        Calculate2( Constexpr{ 8 }, nothing );
    static_assert( res3 == nothing );
    static_assert( !res3 );
    constexpr M<Constexpr> res4 = Calculate2( nothing, nothing );
    static_assert( res4 == nothing );
    static_assert( !res4 );
    constexpr M<Constexpr> res5 =
        Calculate2( Constexpr{ 8 }, Constexpr{ 0 } );
    static_assert( res5 == nothing );
    static_assert( !res5 );
    constexpr M<Constexpr> res6 =
        Calculate2( Constexpr{ 9 }, Constexpr{ 4 } );
    static_assert( res6->n == 2 );

    constexpr M<Constexpr> res7 = res6;
    static_assert( res7 );
    static_assert( res7->n == 2 );

    static_assert( res7 == res6 );

    constexpr M<Constexpr> res8 = M<Constexpr>{ Constexpr{ 5 } };
    static_assert( res8 );
    static_assert( res8->n == 5 );
  }
}

TEST_CASE( "[maybe] reference_wrapper" ) {
  int        a = 5;
  int        b = 7;
  M<RR<int>> m;
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

  M<RR<int>> m2 = m;
  REQUIRE( m2.has_value() );
}

TEST_CASE( "[maybe] nothing_t constructors" ) {
  M<int> m = 5;
  REQUIRE( m.has_value() );
  REQUIRE( m == 5 );
  // This syntax for disengaging the maybe relies on nothing_t
  // having the right constructor properties, otherwise it would
  // be ambiguous.
  m = {};
  REQUIRE( !m.has_value() );

  m = 5;
  REQUIRE( m.has_value() );
  REQUIRE( m == 5 );
  m = nothing;
  REQUIRE( !m.has_value() );
}

} // namespace
} // namespace base
