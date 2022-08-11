/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Injectable interface for random number generation.
*
*****************************************************************/
#include "irand.hpp"

// luapp
#include "luapp/register.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IRand
*****************************************************************/
void to_str( IRand const& o, string& out, base::ADL_t ) {
  out += "IRand@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

/****************************************************************
** Public API
*****************************************************************/
void linker_dont_discard_module_irand();
void linker_dont_discard_module_irand() {}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = IRand;
  auto u  = st.usertype.create<U>();

  u["bernoulli"] = &U::bernoulli;
};

} // namespace
} // namespace rn
