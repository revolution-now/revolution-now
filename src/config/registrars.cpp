/****************************************************************
**registrars.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-10.
*
* Description: Not a config module, just provides a single TU
*              for doing the registration and associated template
*              instantiations of the rds conversion method for
*              config data structures, which imroves compile
*              times in that they don't have to be redundantly
*              instantiated in any TU that wants to consume
*              config data.
*
*****************************************************************/
// config modules.
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
#include "config/range-helpers.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/savegame.rds.hpp"
#include "config/sound.rds.hpp"
#include "config/terrain.rds.hpp"
#include "config/tile-sheet.rds.hpp"
#include "config/turn.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// Other config files.
#include "config/tile-enum.rds.hpp"

#include "../../test/rds/testing.rds.hpp"

// cdr
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

using namespace std;

#define INSTANTIATE_CONFIG( ns, name )       \
  detail::empty_registrar register_config(   \
      ::ns::config_##name##_t* o ) {         \
    return register_config_impl( #name, o ); \
  }

#define INSTANTIATE_RN_CONFIG( name ) \
  INSTANTIATE_CONFIG( rn, name )

namespace rds {

static cdr::converter::options const& converter_options() {
  static cdr::converter::options opts{
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = false,
  };
  return opts;
}

template<cdr::FromCanonical S>
detail::empty_registrar register_config_impl(
    std::string const& name, S* global ) {
  detail::register_config_erased(
      name,
      [global]( cdr::value const& o ) -> PopulatorErrorType {
        UNWRAP_RETURN( res,
                       cdr::run_conversion_from_canonical<S>(
                           o, converter_options() ) );
        *global = std::move( res );
        return base::valid;
      } );
  return {};
}

/****************************************************************
** For each new config, add an entry here.
*****************************************************************/
static_assert(
    cdr::FromCanonical<::rn::config::menu::MenuConfig> );
static_assert(
    cdr::ToCanonical<::rn::config::menu::MenuConfig> );
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
INSTANTIATE_RN_CONFIG( rn );
INSTANTIATE_RN_CONFIG( savegame );
INSTANTIATE_RN_CONFIG( sound );
INSTANTIATE_RN_CONFIG( terrain );
INSTANTIATE_RN_CONFIG( tile_sheet );
INSTANTIATE_RN_CONFIG( turn );
INSTANTIATE_RN_CONFIG( ui );
INSTANTIATE_RN_CONFIG( unit_type );

INSTANTIATE_CONFIG( rn::test, testing );

} // namespace rds
