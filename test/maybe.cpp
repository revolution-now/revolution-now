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

// C++ standard library
#include <experimental/type_traits>

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

using ::Catch::Equals;
using ::Catch::Matchers::Contains;

template<typename T>
using M = ::base::maybe<T>;

/****************************************************************
** TructionTracker
*****************************************************************/
// Tracks number of constructions and destructions.
struct TructionTracker {
  static int  constructed;
  static int  destructed;
  static int  copied;
  static int  moved;
  static int  move_assigned;
  static void reset() {
    constructed = destructed = copied = moved = move_assigned =
        0;
  }

  TructionTracker() noexcept { ++constructed; }
  TructionTracker( TructionTracker const& ) noexcept {
    ++copied;
  }
  TructionTracker( TructionTracker&& ) noexcept { ++moved; }
  ~TructionTracker() noexcept { ++destructed; }

  TructionTracker& operator=( TructionTracker const& ) = delete;
  TructionTracker& operator=( TructionTracker&& ) noexcept {
    ++move_assigned;
    return *this;
  }
};
int TructionTracker::constructed   = 0;
int TructionTracker::destructed    = 0;
int TructionTracker::copied        = 0;
int TructionTracker::moved         = 0;
int TructionTracker::move_assigned = 0;

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
  NoCopyNoMove& operator=( NoCopyNoMove&& )     = delete;
  bool operator==( NoCopyNoMove const& ) const& = default;
  char c;
};
static_assert( !is_copy_constructible_v<NoCopyNoMove> );
static_assert( !is_move_constructible_v<NoCopyNoMove> );
static_assert( !is_copy_assignable_v<NoCopyNoMove> );
static_assert( !is_move_assignable_v<NoCopyNoMove> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[maybe] default construction" ) {
  SECTION( "int" ) {
    TructionTracker::reset();
    {
      M<int> m;
      REQUIRE( !m.has_value() );
      M<int> m2 = {};
      REQUIRE( !m2.has_value() );

      // Verify that the value type does not get instantiated
      // upon default construction of the maybe.
      M<TructionTracker> construction_tracker;
      REQUIRE( TructionTracker::constructed == 0 );
      REQUIRE( TructionTracker::destructed == 0 );
      construction_tracker = TructionTracker{};
      REQUIRE( TructionTracker::constructed == 1 );
      REQUIRE( TructionTracker::destructed == 1 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 1 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 2 );
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
    TructionTracker::reset();
    {
      M<TructionTracker> m;
      REQUIRE( !m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      m.emplace();
      REQUIRE( m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      m.reset();
      REQUIRE( !m.has_value() );
      REQUIRE( TructionTracker::constructed == 1 );
      REQUIRE( TructionTracker::destructed == 1 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 0 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 1 );
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
  REQUIRE( !m.has_value() );
}

TEST_CASE( "[maybe] move construction" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( !m.has_value() );

    M<int> m3( M<int>{} );
    REQUIRE( !m3.has_value() );

    M<int> m4( M<int>{ 0 } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 0 );

    M<int> m5( std::move( m4 ) );
    REQUIRE( !m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == 0 );

    M<int> m6{ std::move( m2 ) };
    REQUIRE( !m2.has_value() );
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
    REQUIRE( !m.has_value() );

    M<string> m3( M<string>{} );
    REQUIRE( !m3.has_value() );

    M<string> m4( M<string>{ "hellm2" } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hellm2" );

    M<string> m5( std::move( m4 ) );
    REQUIRE( !m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == "hellm2" );

    M<string> m6{ std::move( m2 ) };
    REQUIRE( !m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == "hello" );
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
    REQUIRE( !m.has_value() );

    M<int> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( !m2.has_value() );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2;
    m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( !m.has_value() );

    M<string> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
    REQUIRE( !m2.has_value() );
  }
}

TEST_CASE( "[maybe] converting assignments" ) {
  M<int> m = 5;
  REQUIRE( m.has_value() );
  REQUIRE( *m == 5 );

  struct A {
    A() = default;
    A( int m ) : n( m ) {}
        operator int() const { return n; }
    int n = {};
  };

  A a{ 7 };
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( *m == 7 );

  m = A{ 9 };
  REQUIRE( m.has_value() );
  REQUIRE( *m == 9 );

  M<A> m2;
  REQUIRE( !m2.has_value() );
  m2 = A{ 3 };

  m = m2;
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == 3 );
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
  SECTION( "TructionTracker" ) {
    TructionTracker::reset();
    {
      M<TructionTracker> m;
      m.emplace();
      REQUIRE( TructionTracker::constructed == 1 );
      REQUIRE( TructionTracker::destructed == 0 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 0 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 1 );
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
  SECTION( "TructionTracker" ) {
    TructionTracker::reset();
    SECTION( "both empty" ) {
      {
        M<TructionTracker> m1;
        M<TructionTracker> m2;
        m1.swap( m2 );
        REQUIRE( TructionTracker::constructed == 0 );
        REQUIRE( TructionTracker::destructed == 0 );
        REQUIRE( TructionTracker::copied == 0 );
        REQUIRE( TructionTracker::moved == 0 );
        REQUIRE( TructionTracker::move_assigned == 0 );
      }
      REQUIRE( TructionTracker::destructed == 0 );
    }
    SECTION( "both have values" ) {
      {
        M<TructionTracker> m1;
        M<TructionTracker> m2;
        m1.emplace();
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( TructionTracker::constructed == 2 );
        REQUIRE( TructionTracker::destructed == 2 );
        REQUIRE( TructionTracker::copied == 0 );
        // Should call std::swap.
        REQUIRE( TructionTracker::moved == 2 );
        REQUIRE( TructionTracker::move_assigned == 4 );
      }
      REQUIRE( TructionTracker::destructed == 4 );
    }
    SECTION( "one has value" ) {
      {
        M<TructionTracker> m1;
        M<TructionTracker> m2;
        m2.emplace();
        m1.swap( m2 );
        m2.swap( m1 );
        REQUIRE( TructionTracker::constructed == 1 );
        REQUIRE( TructionTracker::destructed == 2 );
        REQUIRE( TructionTracker::copied == 0 );
        REQUIRE( TructionTracker::moved == 2 );
        REQUIRE( TructionTracker::move_assigned == 0 );
      }
      REQUIRE( TructionTracker::destructed == 3 );
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
    //
  }
  SECTION( "TructionTracker" ) {
    {
      TructionTracker::reset();
      M<TructionTracker> m;
      m.value_or( TructionTracker{} );
      REQUIRE( TructionTracker::constructed == 1 );
      REQUIRE( TructionTracker::destructed == 2 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 1 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 2 );
    {
      TructionTracker::reset();
      M<TructionTracker> m;
      m.emplace();
      m.value_or( TructionTracker{} );
      REQUIRE( TructionTracker::constructed == 2 );
      REQUIRE( TructionTracker::destructed == 2 );
      REQUIRE( TructionTracker::copied == 1 );
      REQUIRE( TructionTracker::moved == 0 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 3 );
    {
      TructionTracker::reset();
      M<TructionTracker>{}.value_or( TructionTracker{} );
      REQUIRE( TructionTracker::constructed == 1 );
      REQUIRE( TructionTracker::destructed == 2 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 1 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 2 );
    {
      TructionTracker::reset();
      M<TructionTracker>{ TructionTracker{} }.value_or(
          TructionTracker{} );
      REQUIRE( TructionTracker::constructed == 2 );
      REQUIRE( TructionTracker::destructed == 4 );
      REQUIRE( TructionTracker::copied == 0 );
      REQUIRE( TructionTracker::moved == 2 );
      REQUIRE( TructionTracker::move_assigned == 0 );
    }
    REQUIRE( TructionTracker::destructed == 4 );
  }
}

TEST_CASE( "[maybe] non-copyable non-movable T" ) {
  M<NoCopyNoMove> m;
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
}

TEST_CASE( "[maybe] equality" ) {
  SECTION( "int" ) {
    M<int> m1;
    M<int> m2;
    M<int> m3 = 5;
    M<int> m4 = 7;
    M<int> m5 = 5;

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

TEST_CASE( "[maybe] value()" ) {
  M<int> m1;
  try {
    m1.value();
    // Should not be here.
    REQUIRE( false );
  } catch( bad_maybe_access const& e ) {
    REQUIRE_THAT(
        e.what(),
        Contains( "value() called on an inactive maybe" ) );
  }

  m1 = 5;
  REQUIRE( m1.value() == 5 );
  const M<int> m2 = 5;
  REQUIRE( m2.value() == 5 );
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

} // namespace
} // namespace base
