/****************************************************************
**odr-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-06.
*
* Description: Unit tests for the base/odr module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/odr.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** ODR-use tester.
*****************************************************************/
struct OdrUsedTester {
  inline static bool used = false;

  inline static int registration = [] {
    used = true;
    return 0;
  }();
};

template<typename T>
struct TemplatedNotOdrUsedTester {
  inline static bool used = false;

  inline static int registration = [] {
    used = true;
    return 0;
  }();
};

template<typename T>
struct TemplatedOdrUsedTester {
  inline static bool used = false;

  inline static int registration = [] {
    used = true;
    return 0;
  }();

  // This is the key.
  ODR_USE_MEMBER( registration );
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/odr] method is ODR-used" ) {
  // Non-templated class is always expected to initialize static
  // data members. This is kind of a sanity check.
  REQUIRE( OdrUsedTester::used );

  // This one doesn't really need to be tested, but is true in
  // practice. If it ever starts to fail then we should just be
  // able to comment it out, since there shouldn't be any harm in
  // having the linker include something that is not ODR-used.
  // Though it would be an interesting change in behavior for the
  // compilers.
  REQUIRE( !TemplatedNotOdrUsedTester<int>::used );

  // This one we need to be true for out ODR-use macro to work.
  REQUIRE( TemplatedOdrUsedTester<int>::used );
}

} // namespace
} // namespace base
