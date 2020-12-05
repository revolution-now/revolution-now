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
#include "testing.hpp"

// Under test.
#include "src/base/expect.hpp"

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

using ::std::experimental::is_detected_v;

template<typename T, typename E>
using E = ::base::expect<T, E>;

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
** [static] Invalid value types.
*****************************************************************/
static_assert( is_detected_v<E, int, char> );
static_assert( is_detected_v<E, string, int> );
static_assert( is_detected_v<E, NoCopy, int> );
static_assert( is_detected_v<E, NoCopyNoMove, int> );
static_assert( is_detected_v<E, double, int> );
static_assert( !is_detected_v<E, std::in_place_t, int> );
static_assert( !is_detected_v<E, std::in_place_t&, int> );
static_assert( !is_detected_v<E, std::in_place_t const&, int> );
static_assert( !is_detected_v<E, nothing_t, int> );
static_assert( !is_detected_v<E, nothing_t&, int> );
static_assert( !is_detected_v<E, nothing_t const&, int> );

/****************************************************************
** [static] Invalid error types.
*****************************************************************/
static_assert( is_detected_v<E, int, char> );
static_assert( is_detected_v<E, int, string> );
static_assert( is_detected_v<E, int, NoCopy> );
static_assert( is_detected_v<E, int, NoCopyNoMove> );
static_assert( is_detected_v<E, int, double> );
static_assert( !is_detected_v<E, int, std::in_place_t> );
static_assert( !is_detected_v<E, int, std::in_place_t&> );
static_assert( !is_detected_v<E, int, std::in_place_t const&> );
static_assert( !is_detected_v<E, int, nothing_t> );
static_assert( !is_detected_v<E, int, nothing_t&> );
static_assert( !is_detected_v<E, int, nothing_t const&> );

/****************************************************************
** [static] Propagation of noexcept.
*****************************************************************/
/* clang-format off */
// `int` should always be nothrow.
static_assert(  is_nothrow_constructible_v<E<int,  char>,  int>   );
static_assert(  is_nothrow_constructible_v<E<int,  char>,  int>   );
static_assert(  is_nothrow_constructible_v<E<int,  char>,  char>  );
static_assert(  is_nothrow_constructible_v<E<int,  char>,  char>  );

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

#if 0
/****************************************************************
** [static] Constructability.
*****************************************************************/
static_assert( !is_default_constructible_v<E<int, char>> );
static_assert( !is_default_constructible_v<E<string, char>> );
static_assert( !is_default_constructible_v<E<Throws, char>> );
static_assert( is_constructible_v<E<int>, int> );
static_assert( is_constructible_v<E<int>, char> );
static_assert( is_constructible_v<E<int>, long> );
static_assert( is_constructible_v<E<int>, int&> );
static_assert( is_constructible_v<E<int>, char&> );
static_assert( is_constructible_v<E<int>, long&> );
static_assert( is_constructible_v<E<char>, int> );
static_assert( is_constructible_v<E<char>, double> );
static_assert( !is_constructible_v<E<char*>, long> );
static_assert( !is_constructible_v<E<long>, char*> );

static_assert( is_constructible_v<E<BaseClass>, BaseClass> );
static_assert( is_constructible_v<E<BaseClass>, DerivedClass> );
// Fails on gcc !?
// static_assert( !is_constructible_v<E<DerivedClass>, BaseClass>
// );
static_assert(
    is_constructible_v<E<DerivedClass>, DerivedClass> );

/****************************************************************
** [static] Propagation of triviality.
*****************************************************************/
// p0848r3.html
#  ifdef HAS_CONDITIONALLY_TRIVIAL_SPECIAL_MEMBERS
static_assert( is_trivially_copy_constructible_v<E<int>> );
static_assert( is_trivially_move_constructible_v<E<int>> );
static_assert( is_trivially_copy_assignable_v<E<int>> );
static_assert( is_trivially_move_assignable_v<E<int>> );
static_assert( is_trivially_destructible_v<E<int>> );

static_assert( is_trivially_copy_constructible_v<E<Trivial>> );
static_assert( is_trivially_move_constructible_v<E<Trivial>> );
static_assert( is_trivially_copy_assignable_v<E<Trivial>> );
static_assert( is_trivially_move_assignable_v<E<Trivial>> );
static_assert( is_trivially_destructible_v<E<Trivial>> );

static_assert( !is_trivially_copy_constructible_v<E<string>> );
static_assert( !is_trivially_move_constructible_v<E<string>> );
static_assert( !is_trivially_copy_assignable_v<E<string>> );
static_assert( !is_trivially_move_assignable_v<E<string>> );
static_assert( !is_trivially_destructible_v<E<string>> );
#  endif

/****************************************************************
** [static] Avoiding bool ambiguity.
*****************************************************************/
static_assert( !is_constructible_v<bool, E<bool>> );
static_assert( is_constructible_v<bool, E<int>> );
static_assert( is_constructible_v<bool, E<string>> );

/****************************************************************
** [static] is_value_truish.
*****************************************************************/
template<typename T>
using is_value_truish_t = decltype( &E<T>::is_value_truish );

static_assert( is_detected_v<is_value_truish_t, int> );
static_assert( !is_detected_v<is_value_truish_t, string> );
static_assert( is_detected_v<is_value_truish_t, Boolable> );
static_assert( is_detected_v<is_value_truish_t, Intable> );
static_assert( !is_detected_v<is_value_truish_t, Stringable> );

/****************************************************************
** [static] maybe-of-reference.
*****************************************************************/
static_assert( is_same_v<E<int&>::value_type, int&> );
static_assert(
    is_same_v<E<int const&>::value_type, int const&> );

static_assert( !is_constructible_v<E<int&>, int> );
static_assert( is_constructible_v<E<int&>, int&> );
static_assert( !is_constructible_v<E<int&>, int&&> );
static_assert( !is_constructible_v<E<int&>, int const&> );

static_assert( !is_constructible_v<E<int const&>, int> );
static_assert( is_constructible_v<E<int const&>, int&> );
static_assert( !is_constructible_v<E<int const&>, int&&> );
static_assert( is_constructible_v<E<int const&>, int const&> );

static_assert( !is_assignable_v<E<int&>, int> );
static_assert( !is_assignable_v<E<int&>, int&> );
static_assert( !is_assignable_v<E<int&>, int const&> );
static_assert( !is_assignable_v<E<int&>, int&&> );

static_assert( !is_assignable_v<E<int const&>, int> );
static_assert( !is_assignable_v<E<int const&>, int&> );
static_assert( !is_assignable_v<E<int const&>, int const&> );
static_assert( !is_assignable_v<E<int const&>, int&&> );

static_assert( !is_constructible_v<E<int&>, char&> );
static_assert( !is_constructible_v<E<int&>, long&> );

static_assert( !is_convertible_v<E<int&>, int&> );

static_assert( is_constructible_v<bool, E<int&>> );
static_assert( !is_constructible_v<bool, E<bool&>> );

static_assert( is_constructible_v<E<BaseClass&>, BaseClass&> );
static_assert(
    is_constructible_v<E<BaseClass&>, DerivedClass&> );
static_assert(
    !is_constructible_v<E<DerivedClass&>, BaseClass&> );
static_assert(
    is_constructible_v<E<DerivedClass&>, DerivedClass&> );

// Make sure that we can't invoke just_ref on an rvalue.
static_assert( !is_invocable_v<decltype( just_ref<int> ), int> );
static_assert( is_invocable_v<decltype( just_ref<int> ), int&> );
static_assert(
    !is_invocable_v<decltype( just_ref<int> ), int const&> );
static_assert( is_invocable_v<decltype( just_ref<int const> ),
                              int const&> );
#endif

TEST_CASE( "[expect] some test" ) {
  //
}

} // namespace
} // namespace base
