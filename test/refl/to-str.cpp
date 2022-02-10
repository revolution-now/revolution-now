/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-09.
*
* Description: Unit tests for the src/refl/to-str.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/to-str.hpp"

// rds
#include "rds/testing.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace refl {
namespace {

using namespace std;

/****************************************************************
** MyStructWrapper
*****************************************************************/
struct MyStructWrapper {
  MyStructWrapper() = default;

  MyStructWrapper( rn::MyStruct ms )
    : wrapped( std::move( ms ) ) {}

  // Implement refl::WrapsReflected.
  rn::MyStruct const&          refl() const { return wrapped; }
  static constexpr string_view refl_ns   = "other";
  static constexpr string_view refl_name = "MyStructWrapper";

  bool operator==( MyStructWrapper const& ) const = default;

  rn::MyStruct wrapped;
};

static_assert( WrapsReflected<MyStructWrapper> );
static_assert( base::Show<MyStructWrapper> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[refl/to-str] singleton enum" ) {
  using namespace ::rn;
  REQUIRE( base::to_str( e_single::hello ) == "hello" );
}

TEST_CASE( "[refl/to-str] enum" ) {
  using namespace ::rn;
  REQUIRE( base::to_str( e_color::red ) == "red" );
  REQUIRE( base::to_str( e_color::green ) == "green" );
  REQUIRE( base::to_str( e_color::blue ) == "blue" );
}

TEST_CASE( "[refl/to-str] empty struct" ) {
  using namespace ::rn;
  REQUIRE( base::to_str( EmptyStruct{} ) == "EmptyStruct" );
}

TEST_CASE( "[refl/to-str] struct" ) {
  using namespace ::rn;
  MyStruct ms{
      .xxx     = 5,
      .yyy     = 2.3,
      .zzz_map = { { "hello", "1" }, { "world", "2" } },
  };
  string s = base::to_str( ms );
  REQUIRE( ( s == "MyStruct{xxx=5,yyy=2.3,zzz_map=[(hello,1),("
                  "world,2)]}" ||
             s == "MyStruct{xxx=5,yyy=2.3,zzz_map=[(world,2),("
                  "hello,1)]}" ) );
}

TEST_CASE( "[refl/to-str] template struct" ) {
  using namespace ::rn;
  test::MyTemplateStruct<int, string> mts{
      .xxx     = 5,
      .yyy     = 2.3,
      .zzz_map = { { "hello", "1" }, { "world", "2" } },
  };
  string s = base::to_str( mts );
  // Need to test start and finish to avoid the std::string in
  // the template arguments whose name will depend on standard
  // library used.
  REQUIRE( s.starts_with( "test::MyTemplateStruct<int,std::" ) );
  REQUIRE( (
      s.ends_with(
          ">{xxx=5,yyy=2.3,zzz_map=[(hello,1),(world,2)]}" ) ||
      s.ends_with(
          ">{xxx=5,yyy=2.3,zzz_map=[(world,2),(hello,1)]}" ) ) );
}

TEST_CASE( "[refl/to-str] wrapper" ) {
  using namespace ::rn;
  MyStructWrapper msw( MyStruct{
      .xxx     = 5,
      .yyy     = 2.3,
      .zzz_map = { { "hello", "1" }, { "world", "2" } },
  } );
  string          s = base::to_str( msw );
  REQUIRE( ( s == "other::MyStructWrapper{xxx=5,yyy=2.3,zzz_map="
                  "[(hello,1),(world,2)]}" ||
             s == "other::MyStructWrapper{xxx=5,yyy=2.3,zzz_map="
                  "[(world,2),(hello,1)]}" ) );
}

} // namespace
} // namespace refl
