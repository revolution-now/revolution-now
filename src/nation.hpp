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
#include "aliases.hpp"
#include "color.hpp"
#include "enum.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"

// Flatbuffers
#include "fb/nation_generated.h"

// C++ standard library
#include <string>

namespace rn {

enum class ND e_nation { dutch, french, english, spanish };

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

constexpr auto all_nations() {
  constexpr std::array<e_nation,
                       magic_enum::enum_count<e_nation>()>
      nations = [] {
        std::array<e_nation, magic_enum::enum_count<e_nation>()>
               res{};
        size_t idx = 0;
        for( auto nation : magic_enum::enum_values<e_nation>() )
          res[idx++] = nation;
        return res;
      }();
  static_assert( nations.size() ==
                 magic_enum::enum_count<e_nation>() );
  return nations;
}

} // namespace rn
