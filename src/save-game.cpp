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
#include "config-files.hpp"
#include "logger.hpp"
#include "macros.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/io.hpp"
#include "base/meta.hpp"
#include "base/to-str-ext-std.hpp"

// Revolution Now (config)
#include "../config/rcl/savegame.inl"

// base-util
#include "base-util/stopwatch.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

namespace {

#if 0
fs::path path_for_slot( int slot ) {
  CHECK( slot >= 0 );
  return config_savegame.folder /
         fmt::format( "slot{:02}", slot );
}
#endif

#if 0
valid_or<string> load_from_blob(
    /*serial::BinaryBlob const& blob*/ ) {
  NOT_IMPLEMENTED;
  auto*           root = blob.root<fb::SaveGame>();
  util::StopWatch watch;
  watch.start( "load" );
  valid_deserial_t res = valid;
  FOR_CONSTEXPR_IDX( Idx, kNumSavegameModules ) {
    res = savegame_deserializer( root->get_field<Idx>() );
    if( !res ) return true;
    return false;
  };
  if( !res ) return res;
  watch.stop( "load" );

  // Post-deserialization validation.
  watch.start( "validate" );
  HAS_VALUE_OR_RET( savegame_post_validate_impl(
      std::make_index_sequence<kNumSavegameModules>() ) );
  watch.stop( "validate" );

  lg.info( "loading game took: {}, validation took: {}.",
           watch.human( "load" ), watch.human( "validate" ) );
  return valid;
}
#endif

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<fs::path, generic_err> save_game( int slot ) {
  (void)slot;
  NOT_IMPLEMENTED;
#if 0
  // Increase this to get more accurate reading on save times.
  constexpr int   trials = 1;
  util::StopWatch watch;
  watch.start( "save" );
  auto serialize_to_blob = [&]() -> serial::BinaryBlob {
    for( int i = trials; i >= 1; --i ) {
      serial::BinaryBlob blob = save_game_to_blob();
      if( i == 1 ) return blob; // has to move
    }
    UNREACHABLE_LOCATION;
  };
  auto blob = serialize_to_blob();
  watch.stop( "save" );
  lg.info( "saving game ({} trials) took: {}", trials,
           watch.human( "save" ) );
  auto p = path_for_slot( slot );

  // Serialize to JSON. Must do this before binary for timestamp
  // reasons.
  p.replace_extension( ".jsav" );
  lg.info( "saving game to {}.", p );
  ofstream out( p );
  if( !out.good() )
    return GENERIC_ERROR( "failed to open {} for writing.", p );
  out << blob.to_json<fb::SaveGame>( /*quotes=*/false );

  // Serialize to binary.
  p.replace_extension( ".sav" );
  lg.info( "saving game to {}.", p );
  HAS_VALUE_OR_RET( blob.write( p ) );

  p.replace_extension();
  return p;
#endif
}

expect<fs::path, generic_err> load_game( int slot ) {
  (void)slot;
  NOT_IMPLEMENTED;
#if 0
  auto json_path =
      path_for_slot( slot ).replace_extension( ".jsav" );
  auto blob_path =
      path_for_slot( slot ).replace_extension( ".sav" );

  bool json_exists = fs::exists( json_path );
  bool blob_exists = fs::exists( blob_path );

  if( !blob_exists && !json_exists )
    return GENERIC_ERROR( "save files not found for slot {}.",
                          slot );

  // Determine whether to use JSON file or binary file.
  bool use_json = false;
  if( json_exists && !blob_exists ) {
    lg.warn(
        "loading game from JSON file {} since binary file does "
        "not exist.",
        json_path );
    use_json = true;
  } else if( !json_exists && blob_exists ) {
    use_json = false;
  } else {
    // Both exist, so choose based on timestamps. Detect if JSON
    // file has been edited; if so then we should probably prefer
    // that one.
    use_json = fs::last_write_time( json_path ) >
               fs::last_write_time( blob_path );
    if( use_json )
      lg.warn(
          "loading game from JSON file {} since it is newer.",
          json_path );
  }

  if( use_json ) {
    auto maybe_json =
        base::read_text_file_as_string( json_path );
    if( !maybe_json )
      return GENERIC_ERROR( "failed to read json file" );
    UNWRAP_RETURN( blob,
                   serial::BinaryBlob::from_json(
                       /*schema_file_name=*/"save-game.fbs",
                       /*json=*/*maybe_json,
                       /*root_type=*/"SaveGame" ) );
    HAS_VALUE_OR_RET( load_from_blob( blob ) );
    return json_path;
  } else {
    lg.info( "loading game from {}.", blob_path );
    UNWRAP_RETURN( blob, serial::BinaryBlob::read( blob_path ) );
    HAS_VALUE_OR_RET( load_from_blob( blob ) );
    return blob_path;
  }
#endif
}

} // namespace rn
