/****************************************************************
**tile-sheet.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-19.
*
* Description: All things related to tile sheet configuration.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "tile-enum.rds.hpp"

// Rds
#include "tile-sheet.rds.hpp"

// refl
#include "refl/enum-map.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace rn {

/****************************************************************
** TileSheetsConfig
*****************************************************************/
struct TileSheetsConfig {
  TileSheetsConfig();
  bool operator==( TileSheetsConfig const& ) const = default;

  // Implement refl::WrapsReflected.
  TileSheetsConfig( wrapped::TileSheetsConfig&& o );
  wrapped::TileSheetsConfig const&  refl() const { return o_; }
  static constexpr std::string_view refl_ns = "rn";
  static constexpr std::string_view refl_name =
      "TileSheetsConfig";

  valid_or<std::string> validate() const;
  void                  validate_or_die() const;

  gfx::size sprite_size( e_tile tile ) const;

 private:
  // ----- Serializable state.
  wrapped::TileSheetsConfig o_;

  // ----- Non-serializable (transient) state.
  refl::enum_map<e_tile, gfx::size> sizes_;
};

} // namespace rn
