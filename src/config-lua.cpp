/****************************************************************
**config-lua.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Loads the config objects into a Lua state.
*
*****************************************************************/
#include "config-lua.hpp"

// config modules.
#include "config/cheat.rds.hpp"
#include "config/colony.rds.hpp"
#include "config/combat.rds.hpp"
#include "config/command.rds.hpp"
#include "config/commodity.rds.hpp"
#include "config/debug.rds.hpp"
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

// other config headers.
#include "config/tile-enum.rds.hpp"

// luapp
#include "luapp/cdr.hpp"
#include "luapp/pretty.hpp"
#include "luapp/state.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

void inject_config( lua::state& st, string const& name,
                    cdr::converter::options const& cdr_opts,
                    auto const& conf ) {
  static_assert(
      cdr::ToCanonical<std::remove_cvref_t<decltype( conf )>> );
  cdr::value const cdr_val =
      cdr::run_conversion_to_canonical( conf, cdr_opts );
  cdr::converter conv( cdr_opts );
  UNWRAP_CHECK( tbl, conv.ensure_type<cdr::table>( cdr_val ) );
  lua::LuaToCdrConverter converter( st );
  st["config"][name] = converter.convert( tbl );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void inject_configs( lua::state& st ) {
  base::ScopedTimer const timer( "converting configs to lua" );
  cdr::converter::options const cdr_opts{
    .write_fields_with_default_value = true };
  CHECK(
      st["config"] == lua::nil,
      "Lua state already has a top-level key called 'config'." );
  st["config"] = st.table.create();

#define INJECT( name ) \
  inject_config( st, #name, cdr_opts, config_##name )

  INJECT( cheat );
  INJECT( colony );
  INJECT( combat );
  INJECT( command );
  INJECT( commodity );
  INJECT( debug );
  INJECT( fathers );
  INJECT( gfx );
  INJECT( harbor );
  INJECT( immigration );
  INJECT( input );
  INJECT( land_view );
  INJECT( lcr );
  INJECT( market );
  INJECT( menu );
  INJECT( missionary );
  INJECT( music );
  INJECT( nation );
  INJECT( natives );
  INJECT( old_world );
  INJECT( production );
  INJECT( revolution );
  INJECT( rn );
  INJECT( savegame );
  INJECT( sound );
  INJECT( terrain );
  INJECT( text );
  INJECT( tile_sheet );
  INJECT( turn );
  INJECT( ui );
  INJECT( unit_type );

  // Dump full lua configs, may come in handy for debugging.
  if( config_debug.dump.dump_lua_config_to.has_value() ) {
    // Do it as the initializer of a static variable so that it
    // only gets done once per process.
    [[maybe_unused]] static auto const _ = [&] {
      static string const fname =
          *config_debug.dump.dump_lua_config_to;
      lg.warn( "dumping lua config to {}.", fname );
      string const configs_all = pretty_print( st["config"] );
      ofstream out( fname );
      // Make valid lua syntax.
      out << "_G.config = " << configs_all;
      return false;
    }();
  }
}

} // namespace rn
