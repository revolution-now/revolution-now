/****************************************************************
**ext.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Declarations needed by types that want to use the
*              luapp extension points.
*
*****************************************************************/
#include "ext.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

namespace internal {

int ext_stack_size( cthread L ) {
  return c_api( L ).stack_size();
}

} // namespace internal

} // namespace lua
