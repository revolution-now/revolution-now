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

// luapp
#include "src/luapp/as.hpp"
#include "src/luapp/iter.hpp"
#include "src/luapp/ruserdata.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::valid;
using ::Catch::Matches;
using ::testing::monitoring_types::Empty;
using ::testing::monitoring_types::Eq;
using ::testing::monitoring_types::Formattable;
using ::testing::monitoring_types::NonEq;
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
  userdata userdata1( L, C.ref_registry() );
  lua::push( L, userdata1 );
  REQUIRE( C.stack_size() == 1 );
  // Stack:
  //   userdata1

  // Test data size.
  REQUIRE( C.rawlen( -1 ) == sizeof( Empty ) );
  // This is not really needed, but just so we have an idea of
  // what the numbers should be.
  static_assert( sizeof( Empty ) == 1 );

  // Test that the userdata's metatable has the right contents.
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  table metatable1( L, C.ref_registry() );
  REQUIRE( C.stack_size() == 1 );
  // Stack:
  //   userdata1

  // Metatable should have:
  //   __gc
  //   __tostring
  //   __eq
  //   __index
  //   __newindex
  //   __name
  //   member_types
  //   member_getters
  //   member_setters
  //   is_owned_by_lua
  REQUIRE( distance( begin( metatable1 ), end( metatable1 ) ) ==
           10 );
  REQUIRE( metatable1["is_owned_by_lua"].type() ==
           type::boolean );
  REQUIRE( metatable1["is_owned_by_lua"] == true );

  // check __index.
  rfunction m__index = as<rfunction>( metatable1["__index"] );
  // No checks yet.
  (void)m__index;

  // check __newindex.
  rfunction m__newindex =
      as<rfunction>( metatable1["__newindex"] );

  // check __gc.
  rfunction m__gc = as<rfunction>( metatable1["__gc"] );
  // Stack:
  //   userdata1

  // check __tostring.
  rfunction m__tostring =
      as<rfunction>( metatable1["__tostring"] );

  // check __eq.
  rfunction m__eq = as<rfunction>( metatable1["__eq"] );

  // check __name.
  string m__name = as<string>( metatable1["__name"] );
  REQUIRE( m__name == "testing::monitoring_types::Empty" );

  // check member_types.
  table member_types = as<table>( metatable1["member_types"] );
  REQUIRE( distance( begin( member_types ),
                     end( member_types ) ) == 0 );

  // check member_getters.
  table member_getters =
      as<table>( metatable1["member_getters"] );
  REQUIRE( distance( begin( member_getters ),
                     end( member_getters ) ) == 0 );

  // check member_setters.
  table member_setters =
      as<table>( metatable1["member_setters"] );
  REQUIRE( distance( begin( member_setters ),
                     end( member_setters ) ) == 0 );

  // Stack:
  //   userdata1
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::userdata );

  // Now set a second object of the same type and ensure that
  // the metatable gets reused, and actually verify it.
  REQUIRE_FALSE( push_userdata_by_value( L, Empty{} ) );
  // Stack:
  //   userdata2
  //   userdata1
  C.getmetatable( -2 );
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
  userdata userdata1( L, C.ref_registry() );
  lua::push( L, userdata1 );
  REQUIRE( C.stack_size() == 1 );
  // Stack:
  //   userdata1

  // Test data size.
  REQUIRE( C.rawlen( -1 ) == sizeof( Empty* ) );
  // This is not really needed, but just so we have an idea of
  // what the numbers should be.
  static_assert( sizeof( Empty* ) == 8 );

  // Test that the userdata's metatable has the right contents.
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  table metatable1( L, C.ref_registry() );
  REQUIRE( C.stack_size() == 1 );
  // Stack:
  //   userdata1

  // Metatable should have:
  //   __tostring
  //   __eq
  //   __index
  //   __newindex
  //   __name
  //   member_types
  //   member_getters
  //   member_setters
  //   is_owned_by_lua
  //
  // __gc is not in the list because this is by ref.
  REQUIRE( distance( begin( metatable1 ), end( metatable1 ) ) ==
           9 );
  REQUIRE( metatable1["is_owned_by_lua"].type() ==
           type::boolean );
  REQUIRE( metatable1["is_owned_by_lua"] == false );

  // check __index.
  rfunction m__index = as<rfunction>( metatable1["__index"] );
  // No checks yet.
  (void)m__index;

  // check __newindex.
  rfunction m__newindex =
      as<rfunction>( metatable1["__newindex"] );

  // check __tostring.
  rfunction m__tostring =
      as<rfunction>( metatable1["__tostring"] );

  // check __eq.
  rfunction m__eq = as<rfunction>( metatable1["__eq"] );

  // check __name.
  string m__name = as<string>( metatable1["__name"] );
  REQUIRE( m__name == "testing::monitoring_types::Empty&" );

  // check member_types.
  table member_types = as<table>( metatable1["member_types"] );
  REQUIRE( distance( begin( member_types ),
                     end( member_types ) ) == 0 );

  // check member_getters.
  table member_getters =
      as<table>( metatable1["member_getters"] );
  REQUIRE( distance( begin( member_getters ),
                     end( member_getters ) ) == 0 );

  // check member_setters.
  table member_setters =
      as<table>( metatable1["member_setters"] );
  REQUIRE( distance( begin( member_setters ),
                     end( member_setters ) ) == 0 );

  // Stack:
  //   userdata1
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::userdata );

  // Now set a second object of the same type and ensure that
  // the metatable gets reused, and actually verify it.
  REQUIRE_FALSE( push_userdata_by_ref( L, e ) );
  // Stack:
  //   userdata2
  //   userdata1
  C.getmetatable( -2 );
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
    REQUIRE_THAT( name,
                  Matches( "testing::monitoring_types::Empty@0x["
                           "0-9a-z]+: Empty\\{\\}$" ) );
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
    C.setglobal( "showable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( showable )" ) ==
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
    Formattable showable;
    REQUIRE( push_userdata_by_ref( L, showable ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "showable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( showable )" ) ==
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
    Formattable const showable;
    REQUIRE( push_userdata_by_ref( L, showable ) );
    REQUIRE( C.stack_size() == 1 );
    C.setglobal( "showable" );
    REQUIRE( C.stack_size() == 0 );

    REQUIRE( C.dostring( "return tostring( showable )" ) ==
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

    st.free();
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

    st.free();
    // !! do not call any lua functions after this.

    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
  }
}

LUA_TEST_CASE( "[userdata] __eq by reference" ) {
  static_assert( equality_comparable<Eq> );
  static_assert( !equality_comparable<NonEq> );

  SECTION( "NonEq" ) {
    SECTION( "one NonEq compared with same lua value" ) {
      NonEq const e;

      REQUIRE( push_userdata_by_ref( L, e ) );
      REQUIRE( C.stack_size() == 1 );

      REQUIRE( C.compare_eq( -1, -1 ) );

      REQUIRE( C.stack_size() == 1 );
      C.pop( 1 );
    }

    SECTION(
        "one NonEq compared with self, but different "
        "userdata" ) {
      NonEq const e;

      REQUIRE( push_userdata_by_ref( L, e ) );
      REQUIRE( !push_userdata_by_ref( L, e ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( !C.compare_eq( -2, -1 ) );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }

    SECTION( "two NonEq" ) {
      NonEq const e1;
      NonEq const e2;

      REQUIRE( push_userdata_by_ref( L, e1 ) );
      REQUIRE( !push_userdata_by_ref( L, e2 ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( !C.compare_eq( -2, -1 ) );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }
  }

  SECTION( "Eq" ) {
    SECTION( "one Eq compared with same lua value" ) {
      Eq const e;

      REQUIRE( push_userdata_by_ref( L, e ) );
      REQUIRE( C.stack_size() == 1 );

      REQUIRE( C.compare_eq( -1, -1 ) );

      REQUIRE( e.eq_count() == 0 );

      REQUIRE( C.stack_size() == 1 );
      C.pop( 1 );
    }

    SECTION(
        "one Eq compared with self, but different userdata" ) {
      Eq const e;

      REQUIRE( push_userdata_by_ref( L, e ) );
      REQUIRE( !push_userdata_by_ref( L, e ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( C.compare_eq( -2, -1 ) );

      // Eq's operator== will be bypassed by our C++ metamethod
      // wrappers test for pointer equality which will short cir-
      // cuit a real operator== call.
      REQUIRE( e.eq_count() == 0 );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }

    SECTION( "two Eq" ) {
      Eq const e1;
      Eq const e2;

      REQUIRE( push_userdata_by_ref( L, e1 ) );
      REQUIRE( !push_userdata_by_ref( L, e2 ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( C.compare_eq( -2, -1 ) );

      REQUIRE( e1.eq_count() == 1 );
      REQUIRE( e2.eq_count() == 1 );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }

    SECTION( "two Eq, different contents" ) {
      Eq const e1( 1 );
      Eq const e2( 2 );

      REQUIRE( push_userdata_by_ref( L, e1 ) );
      REQUIRE( !push_userdata_by_ref( L, e2 ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( !C.compare_eq( -2, -1 ) );

      REQUIRE( e1.eq_count() == 1 );
      REQUIRE( e2.eq_count() == 1 );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }

    SECTION( "two Eq, non-const" ) {
      Eq e1;
      Eq e2;

      REQUIRE( push_userdata_by_ref( L, e1 ) );
      REQUIRE( !push_userdata_by_ref( L, e2 ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( C.compare_eq( -2, -1 ) );

      REQUIRE( e1.eq_count() == 1 );
      REQUIRE( e2.eq_count() == 1 );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }
  }
}

LUA_TEST_CASE( "[userdata] __eq by value" ) {
  static_assert( equality_comparable<Eq> );
  static_assert( !equality_comparable<NonEq> );

  SECTION( "NonEq" ) {
    SECTION( "one NonEq compared with same lua value" ) {
      REQUIRE( push_userdata_by_value( L, NonEq{} ) );
      REQUIRE( C.stack_size() == 1 );

      REQUIRE( C.compare_eq( -1, -1 ) );

      REQUIRE( C.stack_size() == 1 );
      C.pop( 1 );
    }

    SECTION( "two NonEq" ) {
      REQUIRE( push_userdata_by_value( L, NonEq{} ) );
      REQUIRE( !push_userdata_by_value( L, NonEq{} ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( !C.compare_eq( -2, -1 ) );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }
  }

  SECTION( "Eq" ) {
    SECTION( "one Eq compared with same lua value" ) {
      REQUIRE( push_userdata_by_value( L, Eq{} ) );
      REQUIRE( C.stack_size() == 1 );

      REQUIRE( C.compare_eq( -1, -1 ) );

      REQUIRE( C.stack_size() == 1 );
      C.pop( 1 );
    }

    SECTION( "two Eq" ) {
      REQUIRE( push_userdata_by_value( L, Eq{} ) );
      REQUIRE( !push_userdata_by_value( L, Eq{} ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( C.compare_eq( -2, -1 ) );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }

    SECTION( "two Eq, different contents" ) {
      REQUIRE( push_userdata_by_value( L, Eq( 1 ) ) );
      REQUIRE( !push_userdata_by_value( L, Eq( 2 ) ) );
      REQUIRE( C.stack_size() == 2 );

      REQUIRE( !C.compare_eq( -2, -1 ) );

      REQUIRE( C.stack_size() == 2 );
      C.pop( 2 );
    }
  }
}

} // namespace
} // namespace lua
