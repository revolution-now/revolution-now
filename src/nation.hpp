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

// C++ standard library
#include <string>

namespace rn {

enum class ND e_nation {
  dutch,
  french,
  english,
  spanish,
  /**/
  __count__ // must be last
};

inline constexpr int g_num_nations =
    static_cast<int>( e_nation::__count__ );

struct Nation {
  std::string name_lowercase;
  std::string country_name;
  Color       flag_color;

  std::string name_proper() const;
};

// disabled until there is an AI.
// ND e_nation player_nation();

Nation const& nation_obj( e_nation nation );

std::array<e_nation, g_num_nations> const& all_nations();

} // namespace rn
