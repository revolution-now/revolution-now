/****************************************************************
**converter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Unit tests for the src/cdr/converter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/converter.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

TEST_CASE( "[cdr/converter] unordered_map" ) {
  converter conv( "test" );

  using M   = unordered_map<string, int>;
  value v   = list{ table{ { "fst", "one" }, { "snd", 1 } },
                  table{ { "fst", "two" }, { "snd", "2" } } };
  auto  res = conv.from<M>( v );
  REQUIRE( res.has_error() );
  REQUIRE( base::to_str( res.error() ) ==
           "Message: failed to convert cdr value of type string "
           "to int.\n"
           "Frame Trace (most recent frame last):\n"
           "------------------------------------\n"
           "test\n"
           " \\-std::unordered_map\n"
           "    \\-std::pair\n"
           "       \\-int\n"
           "------------------------------------\n" );
}

} // namespace
} // namespace cdr
