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
#include "logger.hpp"
#include "macros.hpp"

// ss
#include "ss/root.hpp"
#include "ss/terrain.hpp"

// config
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
#include "base/io.hpp"
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

expect<fs::path> save_game( RootState const& root, int slot ) {
  auto p = path_for_slot( slot );
  p.replace_extension( ".sav.rcl" );
  // Serialize to rcl. Do this before b64 for timestamp reasons.
  HAS_VALUE_OR_RET(
      save_game_to_rcl_file( root, p, SaveGameOptions{} ) );
  // Serialize to b64.
  p.replace_extension( ".sav.b64" );
  // HAS_VALUE_OR_RET( blob.write( p ) );
  p.replace_extension();
  return p;
}

expect<fs::path> load_game( RootState& root, int slot ) {
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

  if( use_rcl ) {
    HAS_VALUE_OR_RET( load_game_from_rcl_file(
        root, rcl_path, SaveGameOptions{} ) );
    return rcl_path;
  } else {
    lg.info( "loading game from {}.", b64_path );
    NOT_IMPLEMENTED;
  }
}

void autosave( RootState const& root ) {
  expect<fs::path> res = save_game( root, 9 );
  if( !res ) lg.warn( "autosave failed: {}", res.error() );
}

bool should_autosave( int turns ) {
  return turns % config_savegame.autosave_frequency == 0;
}

} // namespace rn
