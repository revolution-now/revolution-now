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

// Rds
#include "nation.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <array>

namespace rn {

struct UnitsState;
struct ColoniesState;

Nationality const& nation_obj( e_nation nation );

maybe<e_nation> nation_from_coord(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Coord coord );

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
