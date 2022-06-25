/****************************************************************
**register.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-24.
*
* Description: Machinery for registering userdata initialization.
*
*****************************************************************/
#include "register.hpp"

using namespace std;

namespace lua {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
namespace detail {

void register_lua_fn( detail::LuaRegistrationFnSig* const* fn ) {
  registration_functions().push_back( fn );
}

} // namespace detail

// Note: although these are registered only once per process,
// they are not run when registered. Moreover, the functions ac-
// cept a Lua state to do the registration. That means that they
// can be anew each time a new Lua state is created.
std::vector<detail::LuaRegistrationFnSig* const*>&
registration_functions() {
  static vector<detail::LuaRegistrationFnSig* const*> fns;
  return fns;
}

} // namespace lua
