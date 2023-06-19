/****************************************************************
**keyval.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Unit tests for the src/keyval.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "base/keyval.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <unordered_map>

namespace base {
namespace {

using namespace std;

TEST_CASE( "[keyval] lookup" ) {
  unordered_map<string, int> m1{
      { "hello", 3 },
      { "world", 7 },
      { "happy", 8 },
      { "birthday", 10 },
  };

  static_assert(
      is_same_v<decltype( lookup( m1, "" ) ), maybe<int&>> );
  static_assert(
      is_same_v<decltype( lookup( as_const( m1 ), "" ) ),
                maybe<int const&>> );

  REQUIRE( lookup( m1, "123" ) == nothing );
  REQUIRE( lookup( m1, "world" ) == 7 );
  REQUIRE( lookup( m1, "hello" ) == 3 );
  REQUIRE( lookup( m1, "birthday" ) == 10 );

  unordered_map<string, int> m2{
      { "hello", 3 },
      { "world", 7 },
      { "happy", 8 },
      { "birthday", 10 },
  };

  static_assert(
      is_same_v<decltype( lookup( m2, "" ) ), maybe<int&>> );
  static_assert(
      is_same_v<decltype( lookup( as_const( m2 ), "" ) ),
                maybe<int const&>> );

  REQUIRE( lookup( m2, "123" ) == nothing );
  REQUIRE( lookup( m2, "world" ) == 7 );
  REQUIRE( lookup( m2, "hello" ) == 3 );
  REQUIRE( lookup( m2, "birthday" ) == 10 );
}

TEST_CASE( "[keyval] find" ) {
  unordered_map<string, int> m1{
      { "hello", 3 },
      { "world", 7 },
      { "happy", 8 },
      { "birthday", 10 },
  };

  REQUIRE( base::find( m1, "123" ) == m1.end() );
  REQUIRE( base::find( m1, "world" ) != m1.end() );
  REQUIRE( ( *base::find( m1, "world" ) ).second == 7 );
  REQUIRE( base::find( m1, "hello" ) != m1.end() );
  REQUIRE( ( *base::find( m1, "hello" ) ).second == 3 );
  REQUIRE( base::find( m1, "birthday" ) != m1.end() );
  REQUIRE( ( *::base::find( m1, "birthday" ) ).second == 10 );

  unordered_map<string, int> m2{
      { "hello", 3 },
      { "world", 7 },
      { "happy", 8 },
      { "birthday", 10 },
  };

  REQUIRE( base::find( m2, "123" ) == m2.end() );
  REQUIRE( base::find( m2, "world" ) != m2.end() );
  REQUIRE( ( *::base::find( m2, "world" ) ).second == 7 );
  REQUIRE( base::find( m2, "hello" ) != m2.end() );
  REQUIRE( ( *::base::find( m2, "hello" ) ).second == 3 );
  REQUIRE( base::find( m2, "birthday" ) != m2.end() );
  REQUIRE( ( *::base::find( m2, "birthday" ) ).second == 10 );
}

struct HasFindMember {
  using const_iterator = int;

  const_iterator find( int ) {
    find_member_called = true;
    return 123;
  }

  const_iterator end() { return 0; };

  bool find_member_called = false;
};

TEST_CASE( "[keyval] calls correct find" ) {
  HasFindMember hfm;
  REQUIRE( hfm.find_member_called == false );
  REQUIRE( base::find( hfm, 0 ) == 123 );
  REQUIRE( hfm.find_member_called == true );
}

} // namespace
} // namespace base
