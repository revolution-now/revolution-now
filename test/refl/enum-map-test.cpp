/****************************************************************
**enum-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-10.
*
* Description: Unit tests for the src/enum-map.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/enum-map.hpp"

// testing
#include "test/luapp/common.hpp"
#include "test/rds/testing.rds.hpp"

// luapp
#include "src/luapp/enum.hpp"
#include "src/luapp/ext-refl.hpp"
#include "src/luapp/ext.hpp"

// refl
#include "refl/to-str.hpp"

// cdr
#include "src/cdr/ext-std.hpp"
#include "src/cdr/ext.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {

struct EnumMapValueUserdata {
  int n = 5;

  bool operator==( EnumMapValueUserdata const& ) const = default;

  friend void to_str( EnumMapValueUserdata const& o,
                      std::string& out,
                      base::tag<EnumMapValueUserdata> ) {
    out += fmt::format( "EnumMapValueUserdata{{n={}}}", o.n );
  }
};

LUA_USERDATA_TRAITS( EnumMapValueUserdata, owned_by_cpp ){};

static void define_usertype_for( state& st,
                                 tag<EnumMapValueUserdata> ) {
  using U = EnumMapValueUserdata;
  auto u  = st.usertype.create<U>();
  u["n"]  = &U::n;
}

} // namespace lua

namespace refl {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::Catch::Matches;
using ::cdr::testing::conv_from_bt;
using ::rn::e_color;
using ::rn::e_count;
using ::rn::e_empty;

// If this changes then the below tests may need to be updated.
static_assert( refl::enum_count<e_color> == 3 );

static_assert(
    is_default_constructible_v<enum_map<e_color, int>> );
static_assert( is_move_constructible_v<enum_map<e_color, int>> );
static_assert( is_move_assignable_v<enum_map<e_color, int>> );
static_assert(
    is_nothrow_move_constructible_v<enum_map<e_color, int>> );
static_assert(
    is_nothrow_move_assignable_v<enum_map<e_color, int>> );

// Our iterable element is a type of pair of Enum and Value; make
// sure that the first component is const as it should be for a
// map-like object.
static_assert(
    is_const_v<decltype( declval<enum_map<e_color, int>>()
                             .begin()
                             ->first )> );

TEST_CASE( "[enum-map] enum_map empty" ) {
  enum_map<e_empty, int> m;
  static_assert( m.kSize == 0 );
}

TEST_CASE( "[enum-map] enum_map non-primitive construction" ) {
  enum_map<e_color, string> m;
  static_assert( m.kSize == 3 );
  REQUIRE( m[e_color::red] == "" );
  REQUIRE( m[e_color::green] == "" );
  REQUIRE( m[e_color::blue] == "" );
}

TEST_CASE( "[enum-map] enum_map primitive initialization" ) {
  enum_map<e_color, int> m;
  static_assert( m.kSize == 3 );
  REQUIRE( m[e_color::red] == 0 );
  REQUIRE( m[e_color::green] == 0 );
  REQUIRE( m[e_color::blue] == 0 );
}

TEST_CASE( "[enum-map] enum_map initializer list init" ) {
  enum_map<e_color, int> m1{
    { e_color::red, 4 },
    { e_color::blue, 5 },
    { e_color::green, 6 },
  };
  enum_map<e_color, int> m2 = {
    { e_color::red, 5 },
    { e_color::blue, 6 },
    { e_color::green, 7 },
  };
  REQUIRE( m1[e_color::red] == 4 );
  REQUIRE( m1[e_color::blue] == 5 );
  REQUIRE( m1[e_color::green] == 6 );
  REQUIRE( m2[e_color::red] == 5 );
  REQUIRE( m2[e_color::blue] == 6 );
  REQUIRE( m2[e_color::green] == 7 );
}

TEST_CASE( "[enum-map] enum_map indexing" ) {
  enum_map<e_color, int> m;
  m.at( e_color::red ) = 5;
  REQUIRE( m[e_color::red] == 5 );
  m[e_color::green] = 7;
  REQUIRE( m.at( e_color::green ) == 7 );
  enum_map<e_color, int> const m_const;
  REQUIRE( m_const[e_color::red] == 0 );
  REQUIRE( m_const.at( e_color::green ) == 0 );
}

TEST_CASE( "[enum-map] enum_map equality" ) {
  enum_map<e_color, int> m1;
  enum_map<e_color, int> m2;
  REQUIRE( m1 == m2 );
  REQUIRE( m1[e_color::blue] == 0 );
  REQUIRE( m1 == m2 );
  m1[e_color::blue] = 5;
  REQUIRE( m1 != m2 );
  m1[e_color::blue] = 0;
  REQUIRE( m1 == m2 );
  m1[e_color::red] = 2;
  REQUIRE( m1 != m2 );
  m2 = m1;
  REQUIRE( m1 == m2 );
  REQUIRE( m1[e_color::red] == 2 );
  REQUIRE( m1[e_color::green] == 0 );
  REQUIRE( m1[e_color::blue] == 0 );
}

enum_map<e_color, string> const native_colors1{
  { e_color::red, "one" },
  /*{ e_color::green, "" },*/
  { e_color::blue, "three" },
};

cdr::value const cdr_colors1 = cdr::table{
  "red"_key   = "one",
  "green"_key = "",
  "blue"_key  = "three",
};

cdr::value const cdr_colors1_extra_field = cdr::table{
  "red"_key    = "one",
  "green"_key  = "",
  "blue"_key   = "three",
  "purple"_key = "four",
};

cdr::value const cdr_colors1_wrong_type = cdr::table{
  "red"_key   = "one",
  "green"_key = 5,
  "blue"_key  = "three",
};

cdr::value const cdr_colors1_missing_field = cdr::table{
  "red"_key  = "one",
  "blue"_key = "three",
};

TEST_CASE( "[enum-map] cdr/strict" ) {
  using M = enum_map<e_color, string>;
  cdr::converter conv{ {
    .write_fields_with_default_value  = true,
    .allow_unrecognized_fields        = false,
    .default_construct_missing_fields = false,
  } };
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( native_colors1 ) == cdr_colors1 );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<M>( conv, cdr_colors1 ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( 5 ) ==
             conv.err( "expected type table, instead found type "
                       "integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_wrong_type ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_extra_field ) ==
             conv.err( "unrecognized key 'purple' in table." ) );
    REQUIRE( conv.from<M>( cdr_colors1_missing_field ) ==
             conv.err( "key 'green' not found in table." ) );
  }
}

TEST_CASE( "[enum-map] cdr/no-defaults" ) {
  using M = enum_map<e_color, string>;
  cdr::converter conv{ {
    .write_fields_with_default_value  = false,
    .allow_unrecognized_fields        = true,
    .default_construct_missing_fields = true,
  } };
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( native_colors1 ) ==
             cdr_colors1_missing_field );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<M>( conv, cdr_colors1 ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( 5 ) ==
             conv.err( "expected type table, instead found type "
                       "integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_wrong_type ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
    REQUIRE( conv.from<M>( cdr_colors1_extra_field ) ==
             native_colors1 );
    REQUIRE( conv.from<M>( cdr_colors1_missing_field ) ==
             native_colors1 );
  }
}

struct NonCopyable {
  [[maybe_unused]] NonCopyable() = default;
  NonCopyable( int m ) : n( m ) {}

  NonCopyable( NonCopyable const& )            = delete;
  NonCopyable& operator=( NonCopyable const& ) = delete;

  [[maybe_unused]] NonCopyable( NonCopyable&& ) = default;
  NonCopyable& operator=( NonCopyable&& )       = default;

  bool operator==( NonCopyable const& ) const = default;

  int n = 0;
};

TEST_CASE( "[enum-map] non-copyable" ) {
  enum_map<e_color, NonCopyable> em;
  REQUIRE( em[e_color::red] == NonCopyable( 0 ) );

  em[e_color::red] = NonCopyable{ 5 };
  REQUIRE( em[e_color::red] == NonCopyable( 5 ) );

  enum_map<e_color, NonCopyable> em2;
  REQUIRE( em2[e_color::red] == NonCopyable( 0 ) );

  em2 = std::move( em );
  REQUIRE( em2[e_color::red] == NonCopyable( 5 ) );
}

TEST_CASE( "[enum-map] iteration order" ) {
  enum_map<e_count, bool> const em;

  vector<pair<e_count, bool>> out;
  out.resize( em.size() );

  copy( em.begin(), em.end(), out.begin() );

  vector<pair<e_count, bool>> const expected{
    { e_count::one, false },      { e_count::two, false },
    { e_count::three, false },    { e_count::four, false },
    { e_count::five, false },     { e_count::six, false },
    { e_count::seven, false },    { e_count::eight, false },
    { e_count::nine, false },     { e_count::ten, false },
    { e_count::eleven, false },   { e_count::twelve, false },
    { e_count::thirteen, false }, { e_count::fourteen, false },
    { e_count::fifteen, false },
  };
  REQUIRE( out == expected );
}

TEST_CASE( "[enum-map] count_non_default_values" ) {
  enum_map<e_color, int> m;

  REQUIRE( m.count_non_default_values() == 0 );

  m[e_color::blue] = 1;
  REQUIRE( m.count_non_default_values() == 1 );

  m[e_color::blue] = 3;
  REQUIRE( m.count_non_default_values() == 1 );

  m[e_color::red] = 2;
  REQUIRE( m.count_non_default_values() == 2 );

  m[e_color::green] = -1;
  REQUIRE( m.count_non_default_values() == 3 );

  m[e_color::red] = 0;
  REQUIRE( m.count_non_default_values() == 2 );

  m[e_color::blue] = 0;
  REQUIRE( m.count_non_default_values() == 1 );

  m[e_color::green] = 0;
  REQUIRE( m.count_non_default_values() == 0 );
}

TEST_CASE( "[enum-map] traverse" ) {
  using T          = enum_map<e_color, string>;
  using K_expected = e_color;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<K, e_color> )
      v.push_back( "e_color" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o[e_color::red]   = "hello";
  o[e_color::green] = "world";
  o[e_color::blue]  = "again";
  f();

  REQUIRE( v == vector<string>{
                  "red",
                  "e_color",
                  "hello",
                  "green",
                  "e_color",
                  "world",
                  "blue",
                  "e_color",
                  "again",
                } );
}

LUA_TEST_CASE( "[enum-map] Lua API" ) {
  st.lib.open_all();

  SECTION( "int value" ) {
    using M = enum_map<e_color, int>;
    define_usertype_for( st, lua::tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.red == 5 )
      assert( m.green == 7 )
      assert( m.blue == 0 )
      assert( m.xyz == nil )
      m.red = 3
      m.green = 4
      m.blue = 99999
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m[e_color::red]   = 5;
    m[e_color::green] = 7;
    m[e_color::blue]  = 0;

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { e_color::red, 3 },
                     { e_color::green, 4 },
                     { e_color::blue, 99999 } } );
  }

  SECTION( "userdata value" ) {
    using M = enum_map<e_color, lua::EnumMapValueUserdata>;
    define_usertype_for( st,
                         lua::tag<lua::EnumMapValueUserdata>{} );
    define_usertype_for( st, lua::tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.red.n == 5 )
      assert( m.green.n == 2 )
      assert( m.blue.n == 4 )
      assert( m.xyz == nil )
      m.red.n = 3
      m.green.n = 4
      m.blue.n = 99999
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m[e_color::red]   = lua::EnumMapValueUserdata{};
    m[e_color::green] = lua::EnumMapValueUserdata{ .n = 2 };
    m[e_color::blue]  = lua::EnumMapValueUserdata{ .n = 4 };

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { e_color::red,
                       lua::EnumMapValueUserdata{ .n = 3 } },
                     { e_color::green,
                       lua::EnumMapValueUserdata{ .n = 4 } },
                     { e_color::blue, lua::EnumMapValueUserdata{
                                        .n = 99999 } } } );
  }
}

} // namespace
} // namespace refl
