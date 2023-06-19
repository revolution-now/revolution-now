/****************************************************************
**stacktrace.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-23.
*
* Description: Unit tests for the src/stacktrace.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/stacktrace.hpp"

// base
#include "base/build-properties.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[stacktrace] path filter" ) {
  fs::path             p;
  e_stack_trace_frames mode{};
  auto                 f = should_include_filepath_in_stacktrace;

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

  p = base::source_tree_root() / "src";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/a";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/ss/cargo.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/base/fs.hpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/base/../../CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "extern/rtmidi/CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "exe/main.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "exe/main.cppx";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "test/ss/cargo-test.cpp";
  REQUIRE( f( p.string(), mode ) );

  // RN+Extern Only
  mode = e_stack_trace_frames::rn_and_extern_only;
  REQUIRE_FALSE( f( "", mode ) );
  REQUIRE_FALSE( f( "/a/b/c", mode ) );
  REQUIRE_FALSE( f( "/a", mode ) );

  p = base::source_tree_root() / "src";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/a";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/ss/cargo.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/base/fs.hpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "src/base/../../CMakeLists.txt";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "extern/rtmidi/CMakeLists.txt";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "exe/main.cpp";
  REQUIRE( f( p.string(), mode ) );
  p = base::source_tree_root() / "exe/main.cppx";
  REQUIRE_FALSE( f( p.string(), mode ) );
  p = base::source_tree_root() / "test/ss/cargo-test.cpp";
  REQUIRE( f( p.string(), mode ) );
}

} // namespace
} // namespace rn
