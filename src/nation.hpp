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
#include "fmt-helper.hpp"

// C++ standard library
#include <string>

namespace rn {

enum class ND e_( nation, //
                  /*values*/
                  dutch, french, english, spanish );

struct NationDesc {
  std::string name_lowercase;
  std::string country_name;
  Color       flag_color;

  std::string name_proper() const;
};
NOTHROW_MOVE( NationDesc );

NationDesc const& nation_obj( e_nation nation );

constexpr std::array<e_nation, e_nation::_size()> all_nations() {
  constexpr std::array<e_nation, e_nation::_size()> nations =
      [] {
        std::array<e_nation, e_nation::_size()> res{};
        size_t                                  idx = 0;
        for( auto nation : values<e_nation> )
          res[idx++] = nation;
        return res;
      }();
  static_assert( nations.size() == e_nation::_size() );
  return nations;
}

} // namespace rn
