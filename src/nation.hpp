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
#include "coord.hpp"
#include "lua-enum.hpp"
#include "maybe.hpp"

// gfx
#include "gfx/pixel.hpp"

// Rds
#include "nation.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <string>

namespace rn {

struct NationDesc {
  std::string name_lowercase;
  std::string country_name;
  std::string adjective;
  std::string article;
  gfx::pixel  flag_color;

  std::string name_proper() const;
};
NOTHROW_MOVE( NationDesc );

NationDesc const& nation_obj( e_nation nation );

maybe<e_nation> nation_from_coord( Coord coord );

constexpr auto all_nations() {
  constexpr std::array<e_nation, refl::enum_count<e_nation>>
      nations = [] {
        std::array<e_nation, refl::enum_count<e_nation>> res{};
        size_t                                           idx = 0;
        for( auto nation : refl::enum_values<e_nation> )
          res[idx++] = nation;
        return res;
      }();
  static_assert( nations.size() == refl::enum_count<e_nation> );
  return nations;
}

/****************************************************************
** Lua
*****************************************************************/
LUA_ENUM_DECL( nation );

} // namespace rn
