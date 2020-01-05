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

enum class ND e_( nation, //
                  /*values*/
                  dutch, french, english, spanish );
SERIALIZABLE_ENUM( e_nation );

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
