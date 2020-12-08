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
#include "base/maybe-util.hpp"
#include "base/maybe.hpp"
#include "base/variant.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <experimental/type_traits>
#include <functional>

#define ASSERT_VAR_TYPE( var, ... ) \
  static_assert( std::is_same_v<decltype( var ), __VA_ARGS__> )

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

} // namespace
} // namespace base

DEFINE_FORMAT_( base::Tracker, "Tracker" );
FMT_TO_CATCH( base::Tracker );

/****************************************************************
** Constexpr type
*****************************************************************/
namespace base {
namespace {

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

} // namespace
} // namespace base

DEFINE_FORMAT( base::NoCopy, "NoCopy{{c={}}}", o.c );
FMT_TO_CATCH( base::NoCopy );

/****************************************************************
** Non-Copyable, Non-Movable
*****************************************************************/
namespace base {
namespace {

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
struct Boolable {
  Boolable() = default;
  Boolable( bool m ) : n( m ) {}
  // clang-format off
  operator bool() const { return n; }
  // clang-format on
  bool n = {};
};

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
** Base/Derived
*****************************************************************/
struct BaseClass {};
struct DerivedClass : BaseClass {};

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
** [static] Constructability.
*****************************************************************/
static_assert( is_constructible_v<M<int>, int> );
static_assert( is_constructible_v<M<int>, char> );
static_assert( is_constructible_v<M<int>, long> );
static_assert( is_constructible_v<M<int>, int&> );
static_assert( is_constructible_v<M<int>, char&> );
static_assert( is_constructible_v<M<int>, long&> );
static_assert( is_constructible_v<M<char>, int> );
static_assert( is_constructible_v<M<char>, double> );
static_assert( !is_constructible_v<M<char*>, long> );
static_assert( !is_constructible_v<M<long>, char*> );

static_assert( is_constructible_v<M<BaseClass>, BaseClass> );
static_assert( is_constructible_v<M<BaseClass>, DerivedClass> );
// Fails on gcc !?
// static_assert( !is_constructible_v<M<DerivedClass>, BaseClass>
// );
static_assert(
    is_constructible_v<M<DerivedClass>, DerivedClass> );

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
** [static] Avoiding bool ambiguity.
*****************************************************************/
static_assert( !is_constructible_v<bool, M<bool>> );
static_assert( is_constructible_v<bool, M<int>> );
static_assert( is_constructible_v<bool, M<string>> );

/****************************************************************
** [static] is_value_truish.
*****************************************************************/
template<typename T>
using is_value_truish_t = decltype( &M<T>::is_value_truish );

static_assert( is_detected_v<is_value_truish_t, int> );
static_assert( !is_detected_v<is_value_truish_t, string> );
static_assert( is_detected_v<is_value_truish_t, Boolable> );
static_assert( is_detected_v<is_value_truish_t, Intable> );
static_assert( !is_detected_v<is_value_truish_t, Stringable> );

/****************************************************************
** [static] maybe-of-reference.
*****************************************************************/
static_assert( is_same_v<M<int&>::value_type, int&> );
static_assert(
    is_same_v<M<int const&>::value_type, int const&> );

static_assert( !is_constructible_v<M<int&>, int> );
static_assert( is_constructible_v<M<int&>, int&> );
static_assert( !is_constructible_v<M<int&>, int&&> );
static_assert( !is_constructible_v<M<int&>, int const&> );

static_assert( !is_constructible_v<M<int const&>, int> );
static_assert( is_constructible_v<M<int const&>, int&> );
static_assert( !is_constructible_v<M<int const&>, int&&> );
static_assert( is_constructible_v<M<int const&>, int const&> );

static_assert( !is_assignable_v<M<int&>, int> );
static_assert( !is_assignable_v<M<int&>, int&> );
static_assert( !is_assignable_v<M<int&>, int const&> );
static_assert( !is_assignable_v<M<int&>, int&&> );

static_assert( !is_assignable_v<M<int const&>, int> );
static_assert( !is_assignable_v<M<int const&>, int&> );
static_assert( !is_assignable_v<M<int const&>, int const&> );
static_assert( !is_assignable_v<M<int const&>, int&&> );

static_assert( !is_constructible_v<M<int&>, char&> );
static_assert( !is_constructible_v<M<int&>, long&> );

static_assert( !is_convertible_v<M<int&>, int&> );

static_assert( is_constructible_v<bool, M<int&>> );
static_assert( !is_constructible_v<bool, M<bool&>> );

static_assert( is_constructible_v<M<BaseClass&>, BaseClass&> );
static_assert(
    is_constructible_v<M<BaseClass&>, DerivedClass&> );
static_assert(
    !is_constructible_v<M<DerivedClass&>, BaseClass&> );
static_assert(
    is_constructible_v<M<DerivedClass&>, DerivedClass&> );

// Make sure that we can't invoke just_ref on an rvalue.
static_assert( !is_invocable_v<decltype( just_ref<int> ), int> );
static_assert( is_invocable_v<decltype( just_ref<int> ), int&> );
static_assert(
    !is_invocable_v<decltype( just_ref<int> ), int const&> );
static_assert( is_invocable_v<decltype( just_ref<int const> ),
                              int const&> );

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

  ASSERT_VAR_TYPE( *m1, int& );
  ASSERT_VAR_TYPE( as_const( *m1 ), int const& );

  ASSERT_VAR_TYPE( *M<int>{}, int&& );

  M<NoCopy> m2{ 'c' };
  REQUIRE( m2.has_value() );
  REQUIRE( *m2 == NoCopy{ 'c' } );
  REQUIRE( m2->c == 'c' );

  ASSERT_VAR_TYPE( *m2, NoCopy& );
  ASSERT_VAR_TYPE( as_const( *m2 ), NoCopy const& );

  ASSERT_VAR_TYPE( *M<NoCopy>{}, NoCopy && );

  ASSERT_VAR_TYPE( m2->c, char );
  ASSERT_VAR_TYPE( as_const( m2 )->c, char );

  ASSERT_VAR_TYPE( &m2->c, char* );
  ASSERT_VAR_TYPE( &as_const( m2->c ), char const* );
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
    ASSERT_VAR_TYPE( m1, M<int> );
    auto m2 = maybe{ string( "hello" ) };
    ASSERT_VAR_TYPE( m2, M<string> );
  }
  // These would fail if we didn't have the explicit deduction
  // guide.
  SECTION( "explicit" ) {
    int  arr[6];
    auto m1 = maybe{ arr };
    ASSERT_VAR_TYPE( m1, M<int*> );
    auto m2 = maybe{ "hello" };
    ASSERT_VAR_TYPE( m2, M<char const*> );
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
  ASSERT_VAR_TYPE( m1, M<int> );
  REQUIRE( m1.has_value() );
  REQUIRE( *m1 == 5 );
  auto m2 = make_maybe( NoCopy{ 'c' } );
  ASSERT_VAR_TYPE( m2, M<NoCopy> );
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

  // Test implicit conversion.
  M<RR<int>> m3 = a;
  M<int&>    m4 = m3;
  REQUIRE( m4.has_value() );
  REQUIRE( m4 == 5 );
  ++a;
  REQUIRE( m4 == 6 );

  M<RR<int const>> m5 = a;
  M<int const&>    m6 = m5;
  REQUIRE( m6.has_value() );
  REQUIRE( m6 == 6 );
  ++a;
  REQUIRE( m6 == 7 );

  M<RR<string>> m7 = nothing;
  M<string&>    m8 = m7;
  REQUIRE( !m8.has_value() );
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

TEST_CASE( "[maybe] cat_maybes" ) {
  SECTION( "int" ) {
    vector<M<int>> v{ M<int>{},    M<int>{ 5 }, M<int>{},
                      M<int>{ 4 }, M<int>{},    M<int>{ 3 } };

    REQUIRE_THAT( cat_maybes( v ),
                  Equals( vector<int>{ 5, 4, 3 } ) );
    REQUIRE_THAT( cat_maybes( std::move( v ) ),
                  Equals( vector<int>{ 5, 4, 3 } ) );
  }
  SECTION( "string" ) {
    vector<M<string>> v{ M<string>{}, M<string>{ "5" },
                         M<string>{}, M<string>{ "4" },
                         M<string>{}, M<string>{ "3" } };

    REQUIRE_THAT( cat_maybes( v ),
                  Equals( vector<string>{ "5", "4", "3" } ) );
    REQUIRE_THAT( cat_maybes( std::move( v ) ),
                  Equals( vector<string>{ "5", "4", "3" } ) );
  }
}

TEST_CASE( "[maybe] fmap" ) {
  SECTION( "int" ) {
    auto   f = []( int n ) { return n + 1; };
    M<int> m;
    REQUIRE( m.fmap( f ) == nothing );
    m = 7;
    REQUIRE( m.fmap( f ) == 8 );
    REQUIRE( M<int>{}.fmap( f ) == nothing );
    REQUIRE( M<int>{ 3 }.fmap( f ) == 4 );
  }
  SECTION( "string" ) {
    auto      f = []( string s ) { return s + s; };
    M<string> m;
    REQUIRE( m.fmap( f ) == nothing );
    m = "7";
    REQUIRE( m.fmap( f ) == "77" );
    REQUIRE( M<string>{}.fmap( f ) == nothing );
    REQUIRE( M<string>{ "xy" }.fmap( f ) == "xyxy" );
  }
  SECTION( "NoCopy" ) {
    auto f = []( NoCopy const& nc ) {
      return NoCopy{ char( nc.c + 1 ) };
    };
    M<NoCopy> m;
    REQUIRE( m.fmap( f ) == nothing );
    m = NoCopy{ 'R' };
    REQUIRE( m.fmap( f ) == NoCopy{ 'S' } );
    REQUIRE( M<NoCopy>{}.fmap( f ) == nothing );
    REQUIRE( M<NoCopy>{ 'y' }.fmap( f ) == NoCopy{ 'z' } );
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    auto       f = []( Tracker const& nc ) { return nc; };
    M<Tracker> m;
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.fmap( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.fmap( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>{}.fmap( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>( in_place ).fmap( f ) != nothing );
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
    M<Tracker> m;
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.fmap( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.fmap( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>{}.fmap( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>( in_place ).fmap( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 3 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 2 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
  SECTION( "pointer to member" ) {
    struct A {
      A( int m ) : n( m ) {}
      int        get_n() const { return n + 1; }
      maybe<int> maybe_get_n() const {
        return ( n > 5 ) ? just( n ) : nothing;
      }
      int n;
    };
    M<A> m;
    REQUIRE( m.fmap( &A::get_n ) == nothing );
    m = 2;
    REQUIRE( m.fmap( &A::get_n ) == 3 );
    REQUIRE( M<A>{ 2 }.fmap( &A::get_n ) == 3 );

    m.reset();
    REQUIRE( m.bind( &A::maybe_get_n ) == nothing );
    m = 2;
    REQUIRE( m.bind( &A::maybe_get_n ) == nothing );
    m = 6;
    REQUIRE( m.bind( &A::maybe_get_n ) == 6 );
    REQUIRE( M<A>{ 6 }.bind( &A::maybe_get_n ) == 6 );
  }
}

TEST_CASE( "[maybe] bind" ) {
  SECTION( "int" ) {
    auto f = []( int n ) {
      return ( n > 5 ) ? M<int>{ n } : nothing;
    };
    M<int> m;
    REQUIRE( m.bind( f ) == nothing );
    m = 4;
    REQUIRE( m.bind( f ) == nothing );
    m = 6;
    REQUIRE( m.bind( f ) == 6 );
    REQUIRE( M<int>{}.bind( f ) == nothing );
    REQUIRE( M<int>{ 3 }.bind( f ) == nothing );
    REQUIRE( M<int>{ 7 }.bind( f ) == 7 );
  }
  SECTION( "string" ) {
    auto f = []( string s ) {
      return ( s.size() > 2 ) ? M<string>{ s } : nothing;
    };
    M<string> m;
    REQUIRE( m.bind( f ) == nothing );
    m = "a";
    REQUIRE( m.bind( f ) == nothing );
    m = "aaaa";
    REQUIRE( m.bind( f ) == "aaaa" );
    REQUIRE( M<string>{}.bind( f ) == nothing );
    REQUIRE( M<string>{ "a" }.bind( f ) == nothing );
    REQUIRE( M<string>{ "aaa" }.bind( f ) == "aaa" );
  }
  SECTION( "NoCopy" ) {
    auto f = []( NoCopy nc ) {
      return ( nc.c == 'g' ) ? M<NoCopy>{ std::move( nc ) }
                             : nothing;
    };
    M<NoCopy> m;
    REQUIRE( M<NoCopy>{}.bind( f ) == nothing );
    REQUIRE( M<NoCopy>{ 'a' }.bind( f ) == nothing );
    REQUIRE( M<NoCopy>{ 'g' }.bind( f ) == NoCopy{ 'g' } );
  }
  SECTION( "Tracker" ) {
    Tracker::reset();
    auto f = []( Tracker const& nc ) {
      return M<Tracker>{ nc };
    };
    M<Tracker> m;
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.bind( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.bind( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>{}.bind( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>( in_place ).bind( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 3 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
  SECTION( "Tracker auto" ) {
    Tracker::reset();
    auto f = []<typename T>( T&& nc ) {
      return M<Tracker>{ std::forward<T>( nc ) };
    };
    M<Tracker> m;
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( m.bind( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    m.emplace();
    REQUIRE( m.bind( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 1 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>{}.bind( f ) == nothing );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );

    Tracker::reset();
    REQUIRE( M<Tracker>( in_place ).bind( f ) != nothing );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 3 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 2 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
}

TEST_CASE( "[maybe] just" ) {
  auto m1 = just( 5 );
  ASSERT_VAR_TYPE( m1, maybe<int> );

  auto m2 = just( "hello" );
  ASSERT_VAR_TYPE( m2, maybe<char const*> );

  auto m3 = just( "hello"s );
  ASSERT_VAR_TYPE( m3, maybe<string> );

  string const& s1 = "hello";
  auto          m4 = just( s1 );
  ASSERT_VAR_TYPE( m4, maybe<string> );

  auto m5 = just( NoCopy{ 'a' } );
  ASSERT_VAR_TYPE( m5, maybe<NoCopy> );

  auto m6 = just<NoCopyNoMove>( in_place, 'a' );
  ASSERT_VAR_TYPE( m6, maybe<NoCopyNoMove> );

  auto m7 = just<string>( std::in_place );
  ASSERT_VAR_TYPE( m7, maybe<string> );
}

TEST_CASE( "[maybe] is_value_truish" ) {
  SECTION( "int" ) {
    M<int> m;
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
    M<bool> m;
    REQUIRE( !m.is_value_truish() );
    m = false;
    REQUIRE( !m.is_value_truish() );
    m = true;
    REQUIRE( m.is_value_truish() );
  }
  SECTION( "Boolable" ) {
    M<Boolable> m;
    REQUIRE( !m.is_value_truish() );
    m = false;
    REQUIRE( !m.is_value_truish() );
    m = true;
    REQUIRE( m.is_value_truish() );
  }
}

TEST_CASE( "[maybe-ref] construction" ) {
  SECTION( "non-const" ) {
    int m = 9;

    M<int&> m0 = nothing;
    REQUIRE( !m0.has_value() );
    REQUIRE( !bool( m0 ) );
    try {
      (void)m0.value();
      // Should not be here.
      REQUIRE( false );
    } catch( bad_maybe_access const& ) {}
    REQUIRE( m0.value_or( m ) == 9 );

    M<int&> m1;
    REQUIRE( !m1.has_value() );
    REQUIRE( !bool( m1 ) );
    REQUIRE( !m1.is_value_truish() );

    int     n  = 5;
    M<int&> m3 = n;
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

    M<B&> m4 = b;
    REQUIRE( m4.has_value() );
    REQUIRE( m4->n == 2 );
    M<B&> m5 = a;
    REQUIRE( m5.has_value() );
    REQUIRE( m5->n == 2 );
    REQUIRE( bool( m5 ) );

    M<A&> m6 = a;
    REQUIRE( m6.has_value() );
    REQUIRE( m6->n == 2 );
    REQUIRE( m6->m == 3 );
    REQUIRE( m6.value().n == 2 );
    REQUIRE( m6.value().m == 3 );
    REQUIRE( bool( m6 ) );

    M<bool&> m7;
    REQUIRE( !m7.has_value() );

    int     z  = 0;
    M<int&> m8 = z;
    REQUIRE( m8 == 0 );
    REQUIRE( !m8.is_value_truish() );
    z = 2;
    REQUIRE( m8 == 2 );
    REQUIRE( m8.is_value_truish() );

    NoCopyNoMove     ncnm{ 'a' };
    M<NoCopyNoMove&> m9 = ncnm;
    REQUIRE( m9.has_value() );
    REQUIRE( m9->c == 'a' );

    string     s   = "hello";
    M<string&> m10 = s;
    REQUIRE( m10.has_value() );
    REQUIRE( m10 == "hello" );
    s = "world";
    REQUIRE( m10 == "world" );
  }
  SECTION( "non-const" ) {
    int m = 9;

    M<int const&> m0 = nothing;
    REQUIRE( !m0.has_value() );
    REQUIRE( !bool( m0 ) );
    try {
      (void)m0.value();
      // Should not be here.
      REQUIRE( false );
    } catch( bad_maybe_access const& ) {}
    REQUIRE( m0.value_or( m ) == 9 );

    M<int const&> m1;
    REQUIRE( !m1.has_value() );
    REQUIRE( !bool( m1 ) );
    REQUIRE( !m1.is_value_truish() );

    int           n  = 5;
    M<int const&> m3 = n;
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

    M<B const&> m4 = b;
    REQUIRE( m4.has_value() );
    REQUIRE( m4->n == 2 );
    M<B const&> m5 = a;
    REQUIRE( m5.has_value() );
    REQUIRE( m5->n == 2 );
    REQUIRE( bool( m5 ) );

    M<A const&> m6 = a;
    REQUIRE( m6.has_value() );
    REQUIRE( m6->n == 2 );
    REQUIRE( m6->m == 3 );
    REQUIRE( m6.value().n == 2 );
    REQUIRE( m6.value().m == 3 );
    REQUIRE( bool( m6 ) );

    M<bool const&> m7;
    REQUIRE( !m7.has_value() );

    int           z  = 0;
    M<int const&> m8 = z;
    REQUIRE( m8 == 0 );
    REQUIRE( !m8.is_value_truish() );
    z = 2;
    REQUIRE( m8 == 2 );
    REQUIRE( m8.is_value_truish() );

    NoCopyNoMove           ncnm{ 'a' };
    M<NoCopyNoMove const&> m9 = ncnm;
    REQUIRE( m9.has_value() );
    REQUIRE( m9->c == 'a' );

    string           s   = "hello";
    M<string const&> m10 = s;
    REQUIRE( m10.has_value() );
    REQUIRE( m10 == "hello" );
    s = "world";
    REQUIRE( m10 == "world" );
  }
}

TEST_CASE( "[maybe-ref] fmap" ) {
  SECTION( "ref to value, int" ) {
    int     n  = 5;
    M<int&> m1 = n;

    auto inc = []( int m ) { return m + 1; };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 6 );
  }
  SECTION( "ref to value, string" ) {
    string     n  = "hello";
    M<string&> m1 = n;

    auto inc = []( string const& s ) { return s + s; };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "hellohello" );
  }
  SECTION( "ref to ref, int" ) {
    int     n  = 5;
    M<int&> m1 = n;

    auto inc = []( int ) -> int& {
      static int x = 8;
      return x;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 8 );
  }
  SECTION( "ref to ref, string" ) {
    string     n  = "hello";
    M<string&> m1 = n;

    auto inc = []( string const& ) -> string const& {
      static const string s = "world";
      return s;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<string const&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
  }
  SECTION( "value to ref, int" ) {
    M<int> m1 = 5;

    auto inc = []( int ) -> int& {
      static int x = 8;
      return x;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 8 );
  }
  SECTION( "value to ref, string" ) {
    M<string> m1 = "hello";

    auto inc = []( string const& ) -> string const& {
      static const string s = "world";
      return s;
    };

    auto m2 = m1.fmap( inc );
    ASSERT_VAR_TYPE( m2, maybe<string const&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
  }
}

TEST_CASE( "[maybe-ref] bind" ) {
  SECTION( "ref to value, int" ) {
    int     n  = 5;
    M<int&> m1 = n;

    auto inc = []( int m ) -> maybe<int> {
      if( m < 5 ) return nothing;
      return m + 1;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<int> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 6 );

    n       = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<int> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
  SECTION( "ref to value, string" ) {
    string     n  = "hello";
    M<string&> m1 = n;

    auto inc = []( string const& m ) -> maybe<string> {
      if( m != "hello" ) return nothing;
      return "world";
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<string> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );

    n       = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<string> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
  SECTION( "ref to ref, int" ) {
    int     n  = 5;
    M<int&> m1 = n;

    auto inc = []( int m ) -> maybe<int const&> {
      static const int x = 3;
      if( m < 5 ) return nothing;
      return x;
    };

    auto const& m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<int const&> const& );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
    REQUIRE( *m2 == 3 );

    n       = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<int const&> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
  SECTION( "ref to ref, string" ) {
    string     n  = "hello";
    M<string&> m1 = n;

    auto inc = []( string const& m ) -> maybe<string&> {
      static string x = "world";
      if( m != "hello" ) return nothing;
      return x;
    };

    auto const& m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<string&> const& );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );

    n       = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<string&> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
  SECTION( "value to ref, int" ) {
    M<int> m1 = 5;

    auto inc = []( int m ) -> maybe<int const&> {
      static const int x = 3;
      if( m < 5 ) return nothing;
      return x;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<int const&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 3 );
    REQUIRE( *m2 == 3 );

    m1      = 3;
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<int const&> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
  SECTION( "value to ref, string" ) {
    M<string> m1 = "hello";

    auto inc = []( string const& m ) -> maybe<string&> {
      static string x = "world";
      if( m != "hello" ) return nothing;
      return x;
    };

    auto m2 = m1.bind( inc );
    ASSERT_VAR_TYPE( m2, maybe<string&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );

    m1      = "world";
    auto m3 = m1.bind( inc );
    ASSERT_VAR_TYPE( m3, maybe<string&> );
    REQUIRE( !m3.has_value() );
    REQUIRE( m3 == nothing );
  }
}

TEST_CASE( "[maybe-ref] just-ref" ) {
  SECTION( "int" ) {
    int  n  = 5;
    auto m1 = just_ref( n );
    ASSERT_VAR_TYPE( m1, maybe<int&> );
    REQUIRE( m1.has_value() );
    REQUIRE( m1 == 5 );
    REQUIRE( *m1 == 5 );
    REQUIRE( m1 != nothing );

    int const m  = 5;
    auto      m2 = just_ref( m );
    ASSERT_VAR_TYPE( m2, maybe<int const&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 5 );
    REQUIRE( *m2 == 5 );
  }
  SECTION( "string" ) {
    string s  = "hello";
    auto   m1 = just_ref( s );
    ASSERT_VAR_TYPE( m1, maybe<string&> );
    REQUIRE( m1.has_value() );
    REQUIRE( m1 == "hello" );
    REQUIRE( *m1 == "hello" );
    REQUIRE( m1 != nothing );

    string const m  = "world";
    auto         m2 = just_ref( m );
    ASSERT_VAR_TYPE( m2, maybe<string const&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == "world" );
    REQUIRE( *m2 == "world" );
  }
}

TEST_CASE( "[maybe-ref] comparison" ) {
  SECTION( "int" ) {
    int           n  = 5;
    M<int&>       m1 = n;
    int           m  = 6;
    M<int const&> m2 = m;

    REQUIRE( m1.has_value() );
    REQUIRE( m2.has_value() );
    REQUIRE( m1 != nothing );
    REQUIRE( m2 != nothing );

    REQUIRE( m1 != m2 );
    m = 5;
    REQUIRE( m1 == m2 );
  }
  SECTION( "string" ) {
    string     n  = "hello";
    M<string&> m1 = n;
    string     m  = "world";
    M<string&> m2 = m;

    REQUIRE( m1.has_value() );
    REQUIRE( m2.has_value() );
    REQUIRE( m1 != nothing );
    REQUIRE( m2 != nothing );

    REQUIRE( m1 != m2 );
    m = "hello";
    REQUIRE( m1 == m2 );
  }
}

TEST_CASE( "[maybe] ref to member" ) {
  SECTION( "val to ref, member" ) {
    struct A {
      int n;
    };

    M<A> m = A{ 4 };

    auto m2 = m.member( &A::n );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    auto m3 = as_const( m ).member( &A::n );
    ASSERT_VAR_TYPE( m3, maybe<int const&> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );
  }
  SECTION( "ref to ref, member" ) {
    struct A {
      int n;
    };

    A     a{ 4 };
    M<A&> m = a;

    auto m2 = m.member( &A::n );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    auto m3 = as_const( m ).member( &A::n );
    ASSERT_VAR_TYPE( m3, maybe<int&> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );

    // Test const.
    M<A const&> m4 = a;

    auto m5 = m4.member( &A::n );
    ASSERT_VAR_TYPE( m5, maybe<int const&> );
    REQUIRE( m5.has_value() );
    REQUIRE( m5 == 3 );
    REQUIRE( *m5 == 3 );
  }
  SECTION( "val to ref, maybe_member" ) {
    struct A {
      maybe<int> n;
    };

    M<A> m = A{ 4 };

    auto m2 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    auto m3 = as_const( m ).maybe_member( &A::n );
    ASSERT_VAR_TYPE( m3, maybe<int const&> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );

    *m2 = 4;

    // We can't have a maybe<T&&>, so that means that when we
    // reference a temporary we use T const&.
    auto m4 = std::move( m ).maybe_member( &A::n );
    ASSERT_VAR_TYPE( m4, maybe<int const&> );
    REQUIRE( m4.has_value() );
    REQUIRE( m4 == 4 );
    REQUIRE( *m4 == 4 );
  }
  SECTION( "ref to ref, maybe_member" ) {
    struct A {
      maybe<int> n;
    };

    A     a{};
    M<A&> m = a;

    auto m1 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m1, maybe<int&> );
    REQUIRE( !m1.has_value() );
    REQUIRE( m1 == nothing );

    a.n = 4;

    auto m2 = m.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m2, maybe<int&> );
    REQUIRE( m2.has_value() );
    REQUIRE( m2 == 4 );
    REQUIRE( *m2 == 4 );

    // Assign through.
    *m2 = 3;

    // consting the maybe object does nothing.
    auto m3 = as_const( m ).maybe_member( &A::n );
    ASSERT_VAR_TYPE( m3, maybe<int&> );
    REQUIRE( m3.has_value() );
    REQUIRE( m3 == 3 );
    REQUIRE( *m3 == 3 );

    // consting the inner object does something.
    M<A const&> m4 = a;

    auto m5 = m4.maybe_member( &A::n );
    ASSERT_VAR_TYPE( m5, maybe<int const&> );
    REQUIRE( m5.has_value() );
    REQUIRE( m5 == 3 );
    REQUIRE( *m5 == 3 );
  }
}

TEST_CASE( "[maybe] get_if" ) {
  SECTION( "value, std::variant" ) {
    using V = std::variant<int, string, double>;
    M<V> m;
    REQUIRE( m.get_if<double>() == nothing );
    SECTION( "int" ) {
      m        = 3;
      auto res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, maybe<int&> );
      REQUIRE( res.has_value() );
      *res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>() == nothing );
    }
    SECTION( "string" ) {
      m        = "hello";
      auto res = as_const( m ).get_if<string>();
      ASSERT_VAR_TYPE( res, maybe<string const&> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>() == nothing );
    }
  }
  SECTION( "value, base::variant" ) {
    using V = base::variant<int, string, double>;
    M<V> m;
    REQUIRE( m.get_if<string>() == nothing );
    SECTION( "int" ) {
      m        = 3;
      auto res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, maybe<int&> );
      REQUIRE( res.has_value() );
      *res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>() == nothing );
    }
    SECTION( "string" ) {
      m        = "hello";
      auto res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, maybe<string&> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>() == nothing );
      m = "world";
      REQUIRE( m == V{ "world" } );
      REQUIRE( m.get_if<double>() == nothing );
    }
  }
  SECTION( "ref, std::variant" ) {
    using V = std::variant<int, string, double>;
    SECTION( "nothing" ) {
      M<V&> m = nothing;
      REQUIRE( m.get_if<string>() == nothing );
    }
    SECTION( "int" ) {
      V     v   = 3;
      M<V&> m   = v;
      auto  res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, maybe<int&> );
      REQUIRE( res.has_value() );
      *res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>() == nothing );
    }
    SECTION( "string" ) {
      V     v   = "hello";
      M<V&> m   = v;
      auto  res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, maybe<string&> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>() == nothing );
    }
  }
  SECTION( "ref, base::variant" ) {
    using V = base::variant<int, string, double>;
    SECTION( "int" ) {
      V     v   = 3;
      M<V&> m   = v;
      auto  res = m.get_if<int>();
      ASSERT_VAR_TYPE( res, maybe<int&> );
      REQUIRE( res.has_value() );
      *res = 4;
      REQUIRE( m == V{ 4 } );
      REQUIRE( m.get_if<double>() == nothing );
    }
    SECTION( "string" ) {
      V const     v   = "hello";
      M<V const&> m   = v;
      auto        res = m.get_if<string>();
      ASSERT_VAR_TYPE( res, maybe<string const&> );
      REQUIRE( res.has_value() );
      REQUIRE( m.get_if<double>() == nothing );
    }
  }
}

TEST_CASE( "[maybe] implicit conversion to maybe-ref" ) {
  {
    M<int>  m1;
    M<int&> m2 = m1;
    M<int&> m3 = static_cast<M<int&>>( m1 );
    (void)m2;
    (void)m3;
    static_assert( is_convertible_v<M<int>, M<int&>> );
  }
  {
    // Not allowed.
    // M<int&> m2 = as_const( m1 );
    // M<int&> m3 = static_cast<M<int&>>( as_const( m1 ) );
    static_assert( !is_convertible_v<M<int> const, M<int&>> );
  }
  {
    M<int>        m1;
    M<int const&> m2 = as_const( m1 );
    M<int const&> m3 =
        static_cast<M<int const&>>( as_const( m1 ) );
    (void)m2;
    (void)m3;
    static_assert(
        is_convertible_v<M<int> const, M<int const&>> );
  }
}

} // namespace
} // namespace base
