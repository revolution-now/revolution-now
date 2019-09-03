/****************************************************************
**fsm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-25.
*
* Description: Finite State Machine.
*
*****************************************************************/
#include "fsm.hpp"

// Revolution Now
#include "fmt-helper.hpp"

// C++ standard library
#include <string>
#include <variant>

using namespace std;

namespace rn {

namespace {} // namespace

void test_fsm() {
  using TestVariant  = std::variant<int, double>;
  using TestVariant2 = std::variant<long, string>;

  auto visitor = []( auto const& o1, auto const& o2 ) {
    fmt::print( "{}-{}\n", o1, o2 );
  };

  visit( visitor, TestVariant{}, TestVariant2{} );
}

} // namespace rn
