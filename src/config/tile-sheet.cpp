/****************************************************************
**tile-sheet.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-15.
*
* Description: Config data for sprite locations.
*
*****************************************************************/
#include "config/tile-sheet.rds.hpp"

// config
#include "config/tile-enum.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <string>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

/****************************************************************
** Validation Methods
*****************************************************************/
valid_or<string> validate_font_sheets(
    vector<rr::AsciiFontSheetConfig> const& sheets ) {
  REFL_VALIDATE( !sheets.empty(),
                 "font sprite sheet expected to have at least "
                 "one element but has none." );

  bool has_simple =
      std::any_of( sheets.begin(), sheets.end(),
                   []( rr::AsciiFontSheetConfig const& sheet ) {
                     return sheet.font_name == "simple";
                   } );
  REFL_VALIDATE( has_simple,
                 "font sheets must contain one sheet with font "
                 "name 'simple'." );
  return valid;
}

valid_or<string> validate_sprite_sheets(
    vector<rr::SpriteSheetConfig> const& sheets ) {
  unordered_set<e_tile> tiles;
  tiles.reserve( refl::enum_count<e_tile> );
  int idx = 0;
  for( rr::SpriteSheetConfig const& sheet : sheets ) {
    REFL_VALIDATE( !sheet.img_path.empty(),
                   "sprite sheets must have non-empty image "
                   "paths, but the {}th one does not.",
                   idx );
    REFL_VALIDATE( sheet.sprite_size.area() > 0,
                   "sprite sizes must have area larger than "
                   "zero, but the {}th one does not.",
                   idx );
    for( auto const& [sprite_name, pos] : sheet.sprites ) {
      base::maybe<e_tile> tile =
          refl::enum_from_string<e_tile>( sprite_name );
      REFL_VALIDATE( tile.has_value(),
                     "found tile name '{}' in config file but "
                     "there is not such member of enum {}::{}.",
                     sprite_name, refl::traits<e_tile>::ns,
                     refl::traits<e_tile>::name );
      REFL_VALIDATE( !tiles.contains( *tile ),
                     "tile name '{}' appears more than once in "
                     "the sprite sheet config file.",
                     sprite_name );
      tiles.insert( *tile );
    }
    ++idx;
  }

  for( e_tile tile : refl::enum_values<e_tile> ) {
    REFL_VALIDATE(
        tiles.contains( tile ),
        "sprite sheet config file does not contain tile {}.",
        tile );
  }

  // At this point we should effectively have verified the fol-
  // lowing; if it is not satisfied then something is wrong with
  // our validation above.
  CHECK( tiles.size() == refl::enum_count<e_tile> );
  return valid;
}

} // namespace

/****************************************************************
** TileSheetsConfig
*****************************************************************/
valid_or<string> TileSheetsConfig::validate() const {
  HAS_VALUE_OR_RET( validate_font_sheets( font_sheets ) );
  HAS_VALUE_OR_RET( validate_sprite_sheets( sprite_sheets ) );
  return valid;
}

} // namespace rn
