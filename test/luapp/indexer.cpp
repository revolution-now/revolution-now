/****************************************************************
**indexer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-10.
*
* Description: Unit tests for the src/luapp/indexer.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/indexer.hpp"
#include "src/luapp/thing.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

// FMT_TO_CATCH( ::rn::UnitId );

namespace lua {
namespace {

using namespace std;

using ::base::valid;

template<typename Derived>
struct TableBase : reference {
  TableBase( lua_State* st, int ref ) : reference( st, ref ) {}

  template<typename U>
  auto operator[]( U&& idx ) noexcept {
    return indexer<U, Derived>( std::forward<U>( idx ),
                                static_cast<Derived&>( *this ) );
  }
};

struct EmptyTable : TableBase<EmptyTable> {
  using Base = TableBase<EmptyTable>;

  EmptyTable( lua_State* L )
    : Base( L, [L] {
        c_api C = c_api::view( L );
        CHECK( C.dostring( "return {}" ) );
        return C.ref_registry();
      }() ) {}
};

struct GlobalTable : TableBase<EmptyTable> {
  using Base = TableBase<EmptyTable>;

  GlobalTable( lua_State* L )
    : Base( L, [L] {
        c_api C = c_api::view( L );
        C.pushglobaltable();
        return C.ref_registry();
      }() ) {}
};

struct SomeTable : TableBase<EmptyTable> {
  using Base = TableBase<EmptyTable>;

  SomeTable( lua_State* L )
    : Base( L, [L] {
        c_api C = c_api::view( L );
        CHECK( C.dostring( R"(
          return {
            [5] = {
              [1] = {
                hello = "payload"
              }
            }
          }
        )" ) );
        return C.ref_registry();
      }() ) {}
};

TEST_CASE( "[indexer] construct" ) {
  c_api      C;
  lua_State* L = C.state();
  EmptyTable mt( L );

  indexer idxr1 = mt[5];
  indexer idxr2 = mt["5"];
  int     n     = 2;
  indexer idxr3 = mt[n];

  (void)idxr1;
  (void)idxr2;
  (void)idxr3;
}

TEST_CASE( "[indexer] index" ) {
  c_api      C;
  lua_State* L = C.state();
  EmptyTable mt( L );

  auto idxr = mt[5][1]["hello"]['c'][string( "hello" )];

  using expected_t = indexer<
      string,
      indexer<char,
              indexer<char const( & )[6],
                      indexer<int, indexer<int, EmptyTable>>>>>;
  static_assert( is_same_v<decltype( idxr ), expected_t> );
}

TEST_CASE( "[indexer] push" ) {
  c_api      C;
  lua_State* L = C.state();
  SomeTable  mt( L );

  REQUIRE( C.stack_size() == 0 );

  push( C.state(), mt[5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<string>( -1 ) == "payload" );

  push( C.state(), mt[5][1]["hello"] );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<string>( -1 ) == "payload" );

  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );
}

TEST_CASE( "[indexer] assignment" ) {
  c_api C;
  C.openlibs();
  lua_State* L = C.state();

  REQUIRE( C.stack_size() == 0 );

  EmptyTable mt( L );
  mt[5]             = EmptyTable( L );
  mt[5][1]          = EmptyTable( L );
  mt[5][1]["hello"] = "payload";

  push( C.state(), mt[5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  REQUIRE( C.get<string>( -1 ) == "payload" );
  C.pop();

  mt[5][1]["hello"] = 42;
  push( C.state(), mt[5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE( C.get<int>( -1 ) == 42 );
  C.pop();

  mt[5][1]["hello"] = "world";
  push( C.state(), mt[5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  REQUIRE( C.get<string>( -1 ) == "world" );
  C.pop();

  mt[5]["x"] = SomeTable( L );
  push( C.state(), mt[5]["x"][5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  REQUIRE( C.get<string>( -1 ) == "payload" );
  C.pop();

  mt[5]["x"][5][1]["hello"] = true;
  push( C.state(), mt[5]["x"][5][1]["hello"] );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::boolean );
  REQUIRE( C.get<bool>( -1 ) == true );
  C.pop();

  // NOTE: since key iteration order can be non-deterministic, we
  // should only use this with simple tables that have at most
  // one key per table.
  char const* dump_table = R"(
    function dump( o )
      if type( o ) == 'table' then
        local s = '{ '
        for k,v in pairs( o ) do
          if type( k ) ~= 'number' then k = "'" .. k .. "'" end
          s = s .. '['..k..'] = ' .. dump( v ) .. ','
        end
        return s .. '} '
      else
        return tostring( o )
      end
    end
  )";
  CHECK( C.dostring( dump_table ) == valid );

  GlobalTable{ L }["my_table"] = mt;
  CHECK( C.dostring( R"(
    return dump( my_table )
  )" ) == valid );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  // The following is what we're expecting, modulo some spacing.
  //  table = {
  //    [5] = {
  //      [1] = {
  //        ['hello'] = world,
  //      },
  //      ['x'] = {
  //        [5] = {
  //          [1] = {
  //            ['hello'] = true,
  //          },
  //        },
  //      },
  //    },
  //  }
  string full_table =
      "{ [5] = { [1] = { ['hello'] = world,} ,['x'] = { [5] = { "
      "[1] = { ['hello'] = true,} ,} ,} ,} ,} ";
  REQUIRE( *C.get<string>( -1 ) == full_table );
  C.pop();

  REQUIRE( C.stack_size() == 0 );
}

} // namespace
} // namespace lua
