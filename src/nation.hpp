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
#include "enum.hpp"

// C++ standard library
#include <string>

namespace rn {

enum class ND e_( nation, //
                  /*values*/
                  dutch, french, english, spanish );

struct Nation {
  std::string name_lowercase;
  std::string country_name;
  Color       flag_color;

  std::string name_proper() const;
};

// disabled until there is an AI.
// ND e_nation player_nation();

Nation const& nation_obj( e_nation nation );

std::array<e_nation, e_nation::_size()> const& all_nations();

} // namespace rn
