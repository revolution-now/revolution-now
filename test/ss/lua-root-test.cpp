/****************************************************************
**lua-root-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-18.
*
* Description: Unit tests for the ss/lua-root module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
// #include "src/ss/lua-root.hpp"

// Testing
#include "test/luapp/common.hpp"

// Revolution Now
#include "src/lua.hpp"

// ss
#include "src/ss/root.hpp"

// luapp
#include "src/luapp/ext-refl.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::testing::data_dir;

/****************************************************************
** Test Cases
*****************************************************************/
LUA_TEST_CASE(
    "[ss/lua-root] root data structure lua bindings" ) {
  st.lib.open_all();
  run_lua_startup_routines( st );

  RootState root;
  st["root"] = root;

  auto const file = data_dir() / "lua" / "lua-root-test.lua";
  REQUIRE( st.script.run_file_safe( file.string() ) == valid );
}

} // namespace
} // namespace rn
