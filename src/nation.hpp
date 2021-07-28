/****************************************************************
**nation.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "coord.hpp"
#include "lua-enum.hpp"
#include "maybe.hpp"

// Rds
#include "rds/nation.hpp"

// C++ standard library
#include <string>

namespace rn {

struct NationDesc {
  std::string name_lowercase;
  std::string country_name;
  std::string adjective;
  std::string article;
  Color       flag_color;

  std::string name_proper() const;
};
NOTHROW_MOVE( NationDesc );

NationDesc const& nation_obj( e_nation nation );

maybe<e_nation> nation_from_coord( Coord coord );

constexpr auto all_nations() {
  constexpr std::array<e_nation, enum_traits<e_nation>::count>
      nations = [] {
        std::array<e_nation, enum_traits<e_nation>::count> res{};
        size_t idx = 0;
        for( auto nation : enum_traits<e_nation>::values )
          res[idx++] = nation;
        return res;
      }();
  static_assert( nations.size() ==
                 enum_traits<e_nation>::count );
  return nations;
}

/****************************************************************
** Lua
*****************************************************************/
LUA_ENUM_DECL( nation );

} // namespace rn
