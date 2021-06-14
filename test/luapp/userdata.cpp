/****************************************************************
**userdata.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Unit tests for the src/luapp/userdata.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/userdata.hpp"

// Testing
#include "test/luapp/common.hpp"
#include "test/monitoring-types.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matches;
using ::testing::monitoring_types::Empty;
using ::testing::monitoring_types::Formattable;
using ::testing::monitoring_types::Tracker;

LUA_TEST_CASE( "[userdata] userdata type name" ) {
  REQUIRE( userdata_typename<Empty>() ==
           "testing::monitoring_types::Empty" );
  REQUIRE( userdata_typename<Empty const&>() ==
           "testing::monitoring_types::Empty const&" );
  REQUIRE( userdata_typename<Empty&>() ==
           "testing::monitoring_types::Empty&" );
  REQUIRE( userdata_typename<Empty const>() ==
           "testing::monitoring_types::Empty const" );
  REQUIRE( userdata_typename<int>() == "int" );
  REQUIRE( userdata_typename<int const&>() == "int const&" );
}

LUA_TEST_CASE( "[userdata] userdata create by value" ) {
  REQUIRE( push_userdata_by_value( L, Empty{} ) );
  REQUIRE( C.stack_size() == 1 );

  // Test data size.
  REQUIRE( C.rawlen( -1 ) == sizeof( Empty ) );
  // This is not really needed, but just so we have an idea of
  // what the numbers should be.
  static_assert( sizeof( Empty ) == 1 );

  // Test that the userdata's metatable has the right name.
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getfield( -1, "__name" ) == type::string );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.get<string>( -1 ) ==
           "testing::monitoring_types::Empty" );
  C.pop( 1 );
  REQUIRE( C.stack_size() == 2 );
  // Stack:
  //   metatable1
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::userdata );

  // Now set a second object of the same type and ensure that
  // the metatable gets reused, and actually verify it.
  REQUIRE_FALSE( push_userdata_by_value( L, Empty{} ) );
  // Stack:
  //   userdata2
  //   metatable1
  //   userdata1
  C.swap_top();
  // Stack:
  //   metatable1
  //   userdata2
  //   userdata1
  C.getmetatable( -2 );
  // Stack:
  //   metatable2
  //   metatable1
  //   userdata2
  //   userdata1
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::table );
  // Ensure that they are equal.
  REQUIRE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
  // Stack:
  //   userdata2
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.type_of( -2 ) == type::userdata );
  C.pop( 2 );
}

LUA_TEST_CASE( "[userdata] userdata created by ref" ) {
  Empty e;
  REQUIRE( push_userdata_by_ref( L, e ) );
  REQUIRE( C.stack_size() == 1 );

  // Test data size.
  REQUIRE( C.rawlen( -1 ) == sizeof( Empty* ) );
  // This is not really needed, but just so we have an idea of
  // what the numbers should be.
  static_assert( sizeof( Empty* ) == 8 );

  // Test that the userdata's metatable has the right name.
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getfield( -1, "__name" ) == type::string );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.get<string>( -1 ) ==
           "testing::monitoring_types::Empty&" );
  C.pop( 1 );
  REQUIRE( C.stack_size() == 2 );
  // Stack:
  //   metatable1
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::userdata );

  // Now set a second object of the same type and ensure that
  // the metatable gets reused, and actually verify it.
  REQUIRE_FALSE( push_userdata_by_ref( L, e ) );
  // Stack:
  //   userdata2
  //   metatable1
  //   userdata1
  C.swap_top();
  // Stack:
  //   metatable1
  //   userdata2
  //   userdata1
  C.getmetatable( -2 );
  // Stack:
  //   metatable2
  //   metatable1
  //   userdata2
  //   userdata1
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::table );
  // Ensure that they are equal.
  REQUIRE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
  // Stack:
  //   userdata2
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.type_of( -2 ) == type::userdata );
  C.pop( 2 );

  // Now set a third object but this time one that is const,
  // and ensure that a new metatable is created.
  REQUIRE( push_userdata_by_ref( L, std::as_const( e ) ) );
  C.pop();
}

LUA_TEST_CASE( "[userdata] userdata created by const ref" ) {
  Empty const e;
  REQUIRE( push_userdata_by_ref( L, e ) );
  REQUIRE( C.stack_size() == 1 );

  // Test data size.
  REQUIRE( C.rawlen( -1 ) == sizeof( Empty* ) );
  // This is not really needed, but just so we have an idea of
  // what the numbers should be.
  static_assert( sizeof( Empty* ) == 8 );

  // Test that the userdata's metatable has the right name.
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getfield( -1, "__name" ) == type::string );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.get<string>( -1 ) ==
           "testing::monitoring_types::Empty const&" );
  C.pop( 1 );
  REQUIRE( C.stack_size() == 2 );
  // Stack:
  //   metatable1
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::userdata );

  // Now set a second object of the same type and ensure that
  // the metatable gets reused, and actually verify it.
  REQUIRE_FALSE( push_userdata_by_ref( L, e ) );
  // Stack:
  //   userdata2
  //   metatable1
  //   userdata1
  C.swap_top();
  // Stack:
  //   metatable1
  //   userdata2
  //   userdata1
  C.getmetatable( -2 );
  // Stack:
  //   metatable2
  //   metatable1
  //   userdata2
  //   userdata1
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.type_of( -2 ) == type::table );
  // Ensure that they are equal.
  REQUIRE( C.compare_eq( -2, -1 ) );
  C.pop( 2 );
  // Stack:
  //   userdata2
  //   userdata1
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.type_of( -2 ) == type::userdata );
  C.pop( 2 );

  // Now set a third object but this time one that is not const,
  // and ensure that a new metatable is created.
  Empty e2;
  REQUIRE( push_userdata_by_ref( L, e2 ) );
  C.pop();
}

LUA_TEST_CASE( "[userdata] userdata tostring" ) {
  C.openlibs();

  SECTION( "object" ) {
    REQUIRE( push_userdata_by_value( L, Empty{} ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "empty" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( empty )" ) == valid );
    REQUIRE( C.stack_size() == 1 );
    UNWRAP_CHECK( name, C.get<string>( -1 ) );
    REQUIRE_THAT(
        name,
        Matches(
            "testing::monitoring_types::Empty: 0x[0-9a-z]+$" ) );
    C.pop();
  }
  SECTION( "int" ) {
    REQUIRE( push_userdata_by_value( L, int{ 5 } ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "int" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( int )" ) == valid );
    REQUIRE( C.stack_size() == 1 );
    UNWRAP_CHECK( name, C.get<string>( -1 ) );
    // This one (int) of course is formattable via fmt, so we get
    // an enhanced stringification.
    REQUIRE_THAT( name, Matches( "int@0x[0-9a-z]+: 5" ) );
    C.pop();
  }
  SECTION( "Formattable by value" ) {
    REQUIRE( push_userdata_by_value( L, Formattable{} ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "fmtable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( fmtable )" ) ==
             valid );
    REQUIRE( C.stack_size() == 1 );
    UNWRAP_CHECK( name, C.get<string>( -1 ) );
    // This one (int) of course is formattable via fmt, so we get
    // an enhanced stringification.
    REQUIRE_THAT(
        name,
        Matches(
            "testing::monitoring_types::Formattable@0x[0-9a-z]+:"
            " Formattable\\{n=5,d=7.7,s=hello\\}" ) );
    C.pop();
  }
  SECTION( "Formattable by reference" ) {
    Formattable fmtable;
    REQUIRE( push_userdata_by_ref( L, fmtable ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "fmtable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( fmtable )" ) ==
             valid );
    REQUIRE( C.stack_size() == 1 );
    UNWRAP_CHECK( name, C.get<string>( -1 ) );
    // This one (int) of course is formattable via fmt, so we get
    // an enhanced stringification.
    REQUIRE_THAT(
        name,
        Matches(
            "testing::monitoring_types::Formattable&@0x[0-9a-z]+"
            ": Formattable\\{n=5,d=7.7,s=hello\\}" ) );
    C.pop();
  }
  SECTION( "Formattable by const reference" ) {
    Formattable const fmtable;
    REQUIRE( push_userdata_by_ref( L, fmtable ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "fmtable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( fmtable )" ) ==
             valid );
    REQUIRE( C.stack_size() == 1 );
    UNWRAP_CHECK( name, C.get<string>( -1 ) );
    // This one (int) of course is formattable via fmt, so we get
    // an enhanced stringification.
    REQUIRE_THAT(
        name, Matches( "testing::monitoring_types::Formattable "
                       "const&@0x[0-9a-z]+: "
                       "Formattable\\{n=5,d=7.7,s=hello\\}" ) );
    C.pop();
  }
}

LUA_TEST_CASE( "[userdata] userdata with tracker" ) {
  Tracker::reset();

  SECTION( "by value" ) {
    REQUIRE( push_userdata_by_value( L, Tracker{} ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    // Test that the userdata's metatable has the right name.
    REQUIRE( C.type_of( -1 ) == type::userdata );
    REQUIRE( C.getmetatable( -1 ) );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.getfield( -1, "__name" ) == type::string );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<string>( -1 ) ==
             "testing::monitoring_types::Tracker" );
    C.pop( 3 );
    REQUIRE( C.stack_size() == 0 );

    // Now set a second object of the same type and ensure that
    // the metatable gets reused.
    REQUIRE_FALSE( push_userdata_by_value( L, Tracker{} ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    st.close();
    // !! do not call any lua functions after this.

    // Ensure that precisely two closures get destroyed (will
    // happen when `st` goes out of scope and Lua calls the
    // final- izers on the userdatas for the two closures that we
    // created above).
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 2 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
  }

  SECTION( "by reference" ) {
    Tracker tr;
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    REQUIRE( push_userdata_by_ref( L, tr ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    // Test that the userdata's metatable has the right name.
    REQUIRE( C.type_of( -1 ) == type::userdata );
    REQUIRE( C.getmetatable( -1 ) );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.getfield( -1, "__name" ) == type::string );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.get<string>( -1 ) ==
             "testing::monitoring_types::Tracker&" );
    C.pop( 3 );
    REQUIRE( C.stack_size() == 0 );

    // Now set a second object of the same type and ensure that
    // the metatable gets reused.
    REQUIRE_FALSE( push_userdata_by_ref( L, tr ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    st.close();
    // !! do not call any lua functions after this.

    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
}

} // namespace
} // namespace lua
