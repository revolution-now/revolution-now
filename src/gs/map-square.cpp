/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-26.
*
* Description: Serializable state representing a map square.
*
*****************************************************************/
#include "map-square.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_gs_map_square() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::MapSquare;

  auto u = st.usertype.create<rn::MapSquare>();

  u["surface"]         = &U::surface;
  u["ground"]          = &U::ground;
  u["overlay"]         = &U::overlay;
  u["river"]           = &U::river;
  u["ground_resource"] = &U::ground_resource;
  u["forest_resource"] = &U::forest_resource;
  u["irrigation"]      = &U::irrigation;
  u["road"]            = &U::road;
  u["sea_lane"]        = &U::sea_lane;
  u["lost_city_rumor"] = &U::lost_city_rumor;
};

} // namespace

} // namespace rn
