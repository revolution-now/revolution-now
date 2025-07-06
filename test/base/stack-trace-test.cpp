/****************************************************************
**stack-trace-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-05.
*
* Description: Unit tests for the base/stack-trace module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/stack-trace.hpp"

// base
#include "src/base/build-properties.hpp"
#include "src/base/fs.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[stack-trace] path filter" ) {
  fs::path p;
  e_stack_trace_frames mode{};
  auto f = should_include_filepath_in_stacktrace;

  // All
  mode = e_stack_trace_frames::all;
  REQUIRE( f( "", mode ) );
  REQUIRE( f( "/a/b/c", mode ) );
  REQUIRE( f( "/a", mode ) );

  // RN Only
  mode = e_stack_trace_frames::rn_only;
  REQUIRE_FALSE( f( "", mode ) );
  REQUIRE_FALSE( f( "/a/b/c", mode ) );
  REQUIRE_FALSE( f( "/a", mode ) );

  p = source_tree_root() / "src";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "src/a";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "src/ss/cargo.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "src/base/fs.hpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "src/base/../../CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "extern/rtmidi/CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "exe/main.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "exe/main.cppx";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "test/ss/cargo-test.cpp";
  REQUIRE( f( p.string(), mode ) );

  // RN+Extern Only
  mode = e_stack_trace_frames::rn_and_extern_only;
  REQUIRE_FALSE( f( "", mode ) );
  REQUIRE_FALSE( f( "/a/b/c", mode ) );
  REQUIRE_FALSE( f( "/a", mode ) );

  p = source_tree_root() / "src";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "src/a";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "src/ss/cargo.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "src/base/fs.hpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "src/base/../../CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "extern/rtmidi/CMakeLists.txt";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "exe/main.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = source_tree_root() / "exe/main.cppx";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = source_tree_root() / "test/ss/cargo-test.cpp";
  REQUIRE( f( p.string(), mode ) );
}

} // namespace
} // namespace base
