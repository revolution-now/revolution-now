/****************************************************************
**build-properties.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-23.
*
* Description: Utilities for getting info about the build.
*
*****************************************************************/
#include "build-properties.hpp"

// base-util
#include "base-util/macros.hpp"

using namespace std;

namespace base {

fs::path const& source_tree_root() {
  static const fs::path p = [] {
    return TO_STRING( RN_SOURCE_TREE_ROOT );
  }();
  return p;
}

fs::path const& build_output_root() {
  static const fs::path p = [] {
    return TO_STRING( RN_BUILD_OUTPUT_ROOT_DIR );
  }();
  return p;
}

} // namespace base
