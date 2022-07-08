/****************************************************************
**testing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-05.
*
* Description: Common definitions for unit tests.
*
*****************************************************************/
#include "testing.hpp"

// base
#include "base/fs.hpp"

using namespace std;

namespace testing {

fs::path const& data_dir() {
  static fs::path data{ "test/data" };
  return data;
}

} // namespace testing
