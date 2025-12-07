/****************************************************************
**rcl-game-storage.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: IGameStorage* implementations for the rcl format.
*
*****************************************************************/
#include "rcl-game-storage.hpp"

// Revolution Now
#include "roles.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/savegame.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.rds.hpp"

// rcl
#include "rcl/emit.hpp"
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// gfx
#include "gfx/cdr-matrix.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// refl
#include "refl/cdr.hpp"

// base
#include "base/io.hpp"
#include "base/logger.hpp"
#include "base/string.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

string construct_rcl_title( SSConst const& ss ) {
  string const difficulty =
      base::capitalize_initials( refl::enum_value_name(
          ss.root.settings.game_setup_options.difficulty ) );
  maybe<e_player> const player_type = [&] -> maybe<e_player> {
    auto const human =
        player_for_role( ss, e_player_role::primary_human );
    if( human.has_value() ) return *human;
    auto const active =
        player_for_role( ss, e_player_role::active );
    if( active.has_value() ) return *active;
    return nothing;
  }();
  string const name = [&] {
    if( player_type.has_value() ) {
      UNWRAP_CHECK_T( Player const& player,
                      ss.players.players[*player_type] );
      return player.name;
    }
    return "Player"s;
  }();
  string const player_name =
      player_type.has_value()
          ? config_nation.players[*player_type]
                .possessive_pre_declaration
          : "(AI only)";
  TurnState const& turn_state = ss.root.turn;
  string const time_point     = fmt::format(
      "{} {}",
      base::capitalize_initials( refl::enum_value_name(
          turn_state.time_point.season ) ),
      turn_state.time_point.year );
  Delta const map_size = ss.root.zzz_terrain.world_size_tiles();
  return fmt::format( "{} {} of the {}, {}, {}x{}", difficulty,
                      name, player_name, time_point, map_size.w,
                      map_size.h );
}

bool should_write_fields_with_default_values(
    RclGameStorageSave::options const& options ) {
  e_savegame_verbosity const e = options.verbosity.value_or(
      config_savegame.default_savegame_verbosity );
  switch( e ) {
    case rn::e_savegame_verbosity::full:
      return true;
    case rn::e_savegame_verbosity::compact:
      return false;
  }
}

string save_game_to_rcl(
    RootState const& root,
    RclGameStorageSave::options const& opts ) {
  cdr::converter::options const cdr_opts{
    .write_fields_with_default_value =
        should_write_fields_with_default_values( opts ) };
  base::ScopedTimer timer( "save-game" );
  timer.checkpoint( "to_canonical" );
  cdr::value cdr_val =
      cdr::run_conversion_to_canonical( root, cdr_opts );
  cdr::converter conv( cdr_opts );
  UNWRAP_CHECK( tbl, conv.ensure_type<cdr::table>( cdr_val ) );
  rcl::ProcessingOptions proc_opts{ .run_key_parse  = false,
                                    .unflatten_keys = false };
  timer.checkpoint( "create doc" );
  UNWRAP_CHECK(
      rcl_doc, rcl::doc::create( std::move( tbl ), proc_opts ) );
  rcl::EmitOptions emit_opts{
    .flatten_keys = true,
  };
  timer.checkpoint( "emit rcl" );
  return rcl::emit( rcl_doc, emit_opts );
}

// The filename is only used for error reporting.
valid_or<string> load_game_from_rcl( RootState& out_root,
                                     string_view filename,
                                     string const& in ) {
  cdr::converter::options const cdr_opts{
    .allow_unrecognized_fields        = false,
    .default_construct_missing_fields = true,
  };
  base::ScopedTimer timer( "load-game" );
  timer.checkpoint( "rcl parse" );
  rcl::ProcessingOptions proc_opts{ .run_key_parse  = true,
                                    .unflatten_keys = true };
  UNWRAP_RETURN( rcl_doc,
                 rcl::parse( filename, in, proc_opts ) );
  timer.checkpoint( "from_canonical" );
  UNWRAP_RETURN( root, run_conversion_from_canonical<RootState>(
                           rcl_doc.top_val(), cdr_opts ) );
  out_root = std::move( root );
  return valid;
}

maybe<string> rcl_file_title( fs::path const& rcl_path ) {
  ifstream in( rcl_path );
  if( !in.good() ) return nothing;
  string line;
  getline( in, line );
  if( line.empty() ) return nothing;
  if( !line.starts_with( "#" ) ) return nothing;
  string const title( line.begin() + 1, line.end() );
  string const trimmed = base::trim( title );
  if( trimmed.empty() ) return nothing;
  return trimmed;
}

} // namespace

/****************************************************************
** RclGameStorageQuery
*****************************************************************/
string_view RclGameStorageQueryImpl::extension_impl() const {
  return "rcl";
}

string RclGameStorageQueryImpl::description_impl(
    fs::path const& p ) const {
  CHECK( fs::exists( p ) );
  maybe<string> const descriptor = rcl_file_title( p );
  return descriptor.has_value()
             ? *descriptor
             : ( fs::path{ p }.replace_extension().string() +
                 " (no title)" );
}

/****************************************************************
** RclGameStorageSave
*****************************************************************/
valid_or<string> RclGameStorageSave::store(
    fs::path const& p ) const {
  lg.info( "saving game to {}.", p );
  // Increase this to get more accurate reading on save times.
  string const rcl_output = base::timer(
      "saving game to rcl",
      [&] { return save_game_to_rcl( ss_.root, opts_ ); } );
  ofstream out( p );
  if( !out.good() )
    return fmt::format( "failed to open {} for writing.", p );
  out << "# " << construct_rcl_title( ss_ ) << "\n";
  out << rcl_output;
  return valid;
}

/****************************************************************
** RclGameStorageLoad
*****************************************************************/
valid_or<string> RclGameStorageLoad::load(
    fs::path const& p ) const {
  auto maybe_rcl = base::read_text_file_as_string( p );
  if( !maybe_rcl )
    return fmt::format( "failed to read Rcl file" );
  {
    base::ScopedTimer timer( "loading game from rcl" );
    GOOD_OR_RETURN(
        load_game_from_rcl( ss_.root, p.string(), *maybe_rcl ) );
  }
  return valid;
}

} // namespace rn
