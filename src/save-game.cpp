/****************************************************************
**save-game.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-18.
*
* Description: Interface for saving and loading a game.
*
*****************************************************************/
#include "save-game.hpp"

// Revolution Now
#include "igui.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "time.hpp"
#include "ts.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.hpp"
#include "ss/terrain.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/savegame.rds.hpp"

// refl
#include "refl/cdr.hpp"

// rcl
#include "rcl/emit.hpp"
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "base/conv.hpp"
#include "base/io.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/stopwatch.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

fs::path path_for_slot( int slot ) {
  CHECK( slot >= 0 );
  return config_savegame.folder /
         fmt::format( "slot{:02}", slot );
}

int number_of_normal_slots() {
  return config_savegame.num_save_slots;
}

int number_of_total_slots() {
  int const kNumAutosaveSlots = 1;
  return number_of_normal_slots() + kNumAutosaveSlots;
}

int autosave_slot() { return number_of_total_slots() - 1; }

bool is_autosave_slot( int slot ) {
  int const last_normal_slot = number_of_normal_slots() - 1;
  return slot > last_normal_slot;
}

fs::path rcl_file_path( string const& slot_path ) {
  return slot_path + ".sav.rcl";
}

bool rcl_file_exists( string const& slot_path ) {
  return fs::exists( rcl_file_path( slot_path ) );
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

void print_time( util::StopWatch const& watch,
                 string_view            name ) {
  (void)watch;
  (void)name;
  // fmt::print( "{}: {}\n", name, watch.human( name ) );
}

string save_game_to_rcl( RootState const&       root,
                         SaveGameOptions const& opts ) {
  cdr::converter::options const cdr_opts{
      .write_fields_with_default_value =
          opts.verbosity == e_savegame_verbosity::full,
  };
  util::StopWatch watch;
  watch.start( "[save] total" );
  watch.start( "  [save] to_canonical" );
  cdr::value cdr_val =
      cdr::run_conversion_to_canonical( root, cdr_opts );
  watch.stop( "  [save] to_canonical" );
  cdr::converter conv( cdr_opts );
  UNWRAP_CHECK( tbl, conv.ensure_type<cdr::table>( cdr_val ) );
  rcl::ProcessingOptions proc_opts{ .run_key_parse  = false,
                                    .unflatten_keys = false };
  UNWRAP_CHECK(
      rcl_doc, rcl::doc::create( std::move( tbl ), proc_opts ) );
  rcl::EmitOptions emit_opts{
      .flatten_keys = true,
  };
  watch.start( "  [save] emit rcl" );
  string res = rcl::emit( rcl_doc, emit_opts );
  watch.stop( "  [save] emit rcl" );
  watch.stop( "[save] total" );
  print_time( watch, "[save] total" );
  print_time( watch, "  [save] to_canonical" );
  print_time( watch, "  [save] emit rcl" );
  return res;
}

// The filename is only used for error reporting.
valid_or<string> load_game_from_rcl( RootState&    out_root,
                                     string_view   filename,
                                     string const& in,
                                     SaveGameOptions const& ) {
  cdr::converter::options const cdr_opts{
      .allow_unrecognized_fields        = false,
      .default_construct_missing_fields = true,
  };
  util::StopWatch watch;
  watch.start( "[load] total" );
  watch.start( "  [load] rcl parse" );
  rcl::ProcessingOptions proc_opts{ .run_key_parse  = true,
                                    .unflatten_keys = true };
  UNWRAP_RETURN( rcl_doc,
                 rcl::parse( filename, in, proc_opts ) );
  watch.stop( "  [load] rcl parse" );
  watch.start( "  [load] from_canonical" );
  UNWRAP_RETURN( root, run_conversion_from_canonical<RootState>(
                           rcl_doc.top_val(), cdr_opts ) );
  watch.stop( "  [load] from_canonical" );
  watch.stop( "[load] total" );
  print_time( watch, "[load] total" );
  print_time( watch, "  [load] rcl parse" );
  print_time( watch, "  [load] from_canonical" );
  out_root = std::move( root );
  return valid;
}

unordered_map<int, string> description_for_slots() {
  unordered_map<int, string> res;
  for( int i = 0; i < number_of_total_slots(); ++i ) {
    if( rcl_file_exists( path_for_slot( i ) ) ) {
      maybe<string> const descriptor =
          rcl_file_title( rcl_file_path( path_for_slot( i ) ) );
      res[i] =
          descriptor.has_value()
              ? *descriptor
              : ( path_for_slot( i ).string() + " (no title)" );
    }
  }
  return res;
}

wait<maybe<int>> select_slot( TS& ts, bool include_autosaves,
                              bool allow_empty ) {
  unordered_map<int, string> slots = description_for_slots();
  ChoiceConfig               config{
                    .msg  = "Select a slot:",
                    .sort = false,
  };
  int const    num_slots  = include_autosaves
                                ? number_of_total_slots()
                                : number_of_normal_slots();
  string const kEmptyName = "(none)";
  for( int i = 0; i < num_slots; ++i ) {
    string summary = kEmptyName;
    if( slots.contains( i ) ) summary = slots[i];
    if( i >= number_of_normal_slots() )
      summary = "Autosave: " + summary;
    config.options.push_back( { .key = fmt::to_string( i ),
                                .display_name = summary } );
  }
  while( true ) {
    maybe<string> selection =
        co_await ts.gui.optional_choice( config );
    if( !selection.has_value() ) co_return nothing;
    UNWRAP_CHECK( slot, base::from_chars<int>( *selection ) );
    if( allow_empty ) co_return slot;
    // We're not allowing to select empty slots.
    if( slots.contains( slot ) ) co_return slot;
    co_await ts.gui.message_box(
        "There is not game saved in this slot." );
  }
}

string construct_rcl_title( RootState const& root ) {
  string const difficulty = base::capitalize_initials(
      refl::enum_value_name( root.settings.difficulty ) );
  string const    name = "David"; // FIXME: temporary
  maybe<e_nation> human;
  for( e_nation nation : refl::enum_values<e_nation> ) {
    maybe<Player const&> player = root.players.players[nation];
    if( !player.has_value() ) continue;
    if( !player->human ) continue;
    human = nation;
    break;
  }
  string const nation_name =
      human.has_value() ? config_nation.nations[*human].adjective
                        : "(AI only)";
  TurnState const& turn_state = root.turn;
  string const     time_point = fmt::format(
      "{} {}",
      base::capitalize_initials( refl::enum_value_name(
          turn_state.time_point.season ) ),
      turn_state.time_point.year );
  return fmt::format( "{} {} of the {}, {}", difficulty, name,
                      nation_name, time_point );
}

// We must record the serialized state of the game each time it
// is loaded or saved so that we can check when it is dirty.
void record_saved_state( SSConst const& ss, TS& ts ) {
  // If we're trying to copy to ourselves then something is
  // wrong.
  CHECK( &ts.saved != &ss.root );
  time_it( "copying root baseline", [&] { //
    ts.saved = ss.root;
  } );
}
// Checks if the serializable game state has been modified in any
// way since the last time it was saved or loaded.
bool is_game_saved( SSConst const& ss, TS& ts ) {
  bool const equal = time_it( "saved state comparison", [&] {
    return ( ss.root == ts.saved );
  } );
  return equal;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
valid_or<std::string> save_game_to_rcl_file(
    RootState const& root, fs::path const& p,
    SaveGameOptions const& opts ) {
  lg.info( "saving game to {}.", p );
  // Increase this to get more accurate reading on save times.
  constexpr int   trials = 1;
  util::StopWatch watch;
  static string   label = "game save (rcl)";
  watch.start( label );
  string rcl_output;
  for( int i = trials; i >= 1; --i )
    rcl_output = save_game_to_rcl( root, opts );
  watch.stop( label );
  lg.info( "saving game to rcl ({} trials) took: {}", trials,
           watch.human( label ) );
  ofstream out( p );
  if( !out.good() )
    return fmt::format( "failed to open {} for writing.", p );
  out << "# " << construct_rcl_title( root ) << "\n";
  out << rcl_output;
  return valid;
}

valid_or<std::string> load_game_from_rcl_file(
    RootState& root, fs::path const& p,
    SaveGameOptions const& opts ) {
  auto maybe_rcl = base::read_text_file_as_string( p );
  if( !maybe_rcl )
    return fmt::format( "failed to read Rcl file" );
  util::StopWatch watch;
  watch.start( "loading from rcl" );
  constexpr int trials = 1;
  for( int i = trials; i >= 1; --i ) {
    HAS_VALUE_OR_RET( load_game_from_rcl( root, p.string(),
                                          *maybe_rcl, opts ) );
  }
  watch.stop( "loading from rcl" );
  lg.info( "loading game ({} trials) took: {}", trials,
           watch.human( "loading from rcl" ) );
  return valid;
}

expect<fs::path> save_game( SSConst const& ss, TS& ts,
                            int slot ) {
  auto p = path_for_slot( slot );
  p.replace_extension( ".sav.rcl" );
  // Serialize to rcl. Do this before b64 for timestamp reasons.
  HAS_VALUE_OR_RET(
      save_game_to_rcl_file( ss.root, p, SaveGameOptions{} ) );
  // Serialize to b64.
  p.replace_extension( ".sav.b64" );
  // HAS_VALUE_OR_RET( blob.write( p ) );
  p.replace_extension();
  // Note that we don't update the ts.saved state here, since we
  // don't want auto-saves to count as saves in that regard,
  // since otherwise the player would not be prompted to save the
  // game on exit if it had just been auto-saved.
  if( !is_autosave_slot( slot ) ) record_saved_state( ss, ts );
  return p;
}

expect<fs::path> load_game( SS& ss, TS& ts, int slot ) {
  auto rcl_path =
      path_for_slot( slot ).replace_extension( ".sav.rcl" );
  auto b64_path =
      path_for_slot( slot ).replace_extension( ".sav.b64" );

  bool rcl_exists = fs::exists( rcl_path );
  bool b64_exists = fs::exists( b64_path );

  if( !rcl_exists && !b64_exists )
    return fmt::format( "save files not found for slot {}.",
                        slot );

  // Determine whether to use the rcl file or the binary file.
  bool use_rcl = false;
  if( rcl_exists && !b64_exists ) {
    lg.warn(
        "loading game from Rcl file {} since binary file does "
        "not exist.",
        rcl_path );
    use_rcl = true;
  } else if( !rcl_exists && b64_exists ) {
    use_rcl = false;
  } else {
    // Both exist, so choose based on timestamps. Detect if Rcl
    // file has been edited; if so then we should probably prefer
    // that one.
    use_rcl = fs::last_write_time( rcl_path ) >
              fs::last_write_time( b64_path );
    if( use_rcl )
      lg.warn(
          "loading game from Rcl file {} since it is newer.",
          rcl_path );
  }

  CHECK( use_rcl, "other formats not implemented." );
  HAS_VALUE_OR_RET( load_game_from_rcl_file(
      ss.root, rcl_path, SaveGameOptions{} ) );

  record_saved_state( ss, ts );
  return rcl_path;
}

void autosave( SSConst const& ss, TS& ts ) {
  expect<fs::path> res = save_game( ss, ts, autosave_slot() );
  if( !res ) lg.warn( "autosave failed: {}", res.error() );
}

bool should_autosave( int turns ) {
  return turns % config_savegame.autosave_frequency == 0;
}

wait<bool> save_game_menu( SSConst const& ss, TS& ts ) {
  maybe<int> const slot = co_await select_slot(
      ts, /*include_autosaves=*/false, /*allow_empty=*/true );
  if( !slot.has_value() ) co_return false;
  expect<fs::path> result = save_game( ss, ts, *slot );
  if( !result.has_value() ) {
    co_await ts.gui.message_box( "Error: failed to save game." );
    lg.error( "failed to save game: {}.", result.error() );
    co_return false;
  }
  co_await ts.gui.message_box(
      fmt::format( "Successfully saved game to {}.",
                   path_for_slot( *slot ) ) );
  lg.info( "saved game to {}.", path_for_slot( *slot ) );
  co_return true;
}

wait<base::NoDiscard<bool>> load_game_menu( SS& ss, TS& ts ) {
  maybe<int> const slot = co_await select_slot(
      ts, /*include_autosaves=*/true, /*allow_empty=*/false );
  if( !slot.has_value() ) co_return false;
  expect<fs::path> result = load_game( ss, ts, *slot );
  if( !result.has_value() ) {
    co_await ts.gui.message_box( "Error: failed to load game." );
    lg.error( "failed to load game: {}.", result.error() );
    co_return false;
  }
  co_await ts.gui.message_box(
      fmt::format( "Successfully loaded game from {}.",
                   path_for_slot( *slot ) ) );
  lg.info( "loaded game from {}.", path_for_slot( *slot ) );
  co_return true;
}

// Will return whether a save happened or not.
wait<> check_ask_save( SSConst const& ss, TS& ts ) {
  if( is_game_saved( ss, ts ) ) co_return;
  YesNoConfig const config{ .msg =
                                "This game has unsaved changes. "
                                "Would you like to save?",
                            .yes_label      = "Yes",
                            .no_label       = "No",
                            .no_comes_first = false };

  maybe<ui::e_confirm> const answer =
      co_await ts.gui.optional_yes_no( config );
  if( !answer.has_value() ) co_return;
  co_await save_game_menu( ss, ts );
}

} // namespace rn
