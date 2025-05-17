/****************************************************************
**registrars.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-10.
*
* Description: Not a config module, just provides a single TU for
*              doing the registration and associated template in-
*              stantiations of the rds conversion method for
*              config data structures, which improves compile
*              times in that they don't have to be redundantly
*              instantiated in any TU that wants to consume
*              config data.
*
*****************************************************************/
// config modules.
#include "config/cheat.rds.hpp"
#include "config/colony.rds.hpp"
#include "config/combat.rds.hpp"
#include "config/command.rds.hpp"
#include "config/commodity.rds.hpp"
#include "config/fathers.rds.hpp"
#include "config/gfx.rds.hpp"
#include "config/harbor.rds.hpp"
#include "config/immigration.rds.hpp"
#include "config/input.rds.hpp"
#include "config/land-view.rds.hpp"
#include "config/lcr.rds.hpp"
#include "config/market.rds.hpp"
#include "config/menu.rds.hpp"
#include "config/missionary.rds.hpp"
#include "config/music.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/old-world.rds.hpp"
#include "config/production.rds.hpp"
#include "config/revolution.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/savegame.rds.hpp"
#include "config/sound.rds.hpp"
#include "config/terrain.rds.hpp"
#include "config/text.rds.hpp"
#include "config/tile-sheet.rds.hpp"
#include "config/turn.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// Other config files.
#include "config/registrar-helper.hpp"
#include "config/tile-enum.rds.hpp"

// cdr
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

#define INSTANTIATE_RN_CONFIG( name ) \
  INSTANTIATE_CONFIG( rn, name )

namespace rds {

// NOTE: Don't forget to add to the config-lua module.
INSTANTIATE_RN_CONFIG( cheat );
INSTANTIATE_RN_CONFIG( colony );
INSTANTIATE_RN_CONFIG( combat );
INSTANTIATE_RN_CONFIG( commodity );
INSTANTIATE_RN_CONFIG( fathers );
INSTANTIATE_RN_CONFIG( gfx );
INSTANTIATE_RN_CONFIG( harbor );
INSTANTIATE_RN_CONFIG( immigration );
INSTANTIATE_RN_CONFIG( input );
INSTANTIATE_RN_CONFIG( land_view );
INSTANTIATE_RN_CONFIG( lcr );
INSTANTIATE_RN_CONFIG( market );
INSTANTIATE_RN_CONFIG( menu );
INSTANTIATE_RN_CONFIG( missionary );
INSTANTIATE_RN_CONFIG( music );
INSTANTIATE_RN_CONFIG( nation );
INSTANTIATE_RN_CONFIG( natives );
INSTANTIATE_RN_CONFIG( old_world );
INSTANTIATE_RN_CONFIG( command );
INSTANTIATE_RN_CONFIG( production );
INSTANTIATE_RN_CONFIG( revolution );
INSTANTIATE_RN_CONFIG( rn );
INSTANTIATE_RN_CONFIG( savegame );
INSTANTIATE_RN_CONFIG( sound );
INSTANTIATE_RN_CONFIG( terrain );
INSTANTIATE_RN_CONFIG( text );
INSTANTIATE_RN_CONFIG( tile_sheet );
INSTANTIATE_RN_CONFIG( turn );
INSTANTIATE_RN_CONFIG( ui );
INSTANTIATE_RN_CONFIG( unit_type );
// NOTE: Don't forget to add to the config-lua module.

} // namespace rds
