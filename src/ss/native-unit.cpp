/****************************************************************
**native-unit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-21.
*
* Description: Represents native units (various kinds of braves).
*
*****************************************************************/
#include "native-unit.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_ss_native_unit();
void linker_dont_discard_module_ss_native_unit() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // NativeUnit.
  [&] {
    using U = ::rn::NativeUnit;

    auto u = st.usertype.create<U>();

    u["id"]   = &U::id;
    u["type"] = &U::type;
  }();
};

} // namespace

} // namespace rn
