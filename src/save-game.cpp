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
#include "fb.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "serial.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/io.hpp"
#include "base/meta.hpp"

// Revolution Now (config)
#include "../config/rcl/savegame.inl"

// Flatbuffers
#include "fb/save-game_generated.h"

// base-util
#include "base-util/stopwatch.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

using ::rn::serial::FBBuilder;
using ::rn::serial::FBOffset;

/****************************************************************
** Save-game module hooks.
*****************************************************************/
// These function templates are being forward declared here;
// there must be one of these defined for each module that is
// saved. However, we don't include any headers that declare
// their specializations in order to reduce header dependencies.
// We just rely on the linker to find each of them for us. If one
// is missing, we'll get a compiler error, so there should be no
// possibility for error. When these functions are called in code
// further below, the compiler can deduce their only template ar-
// guments, and so then there is no need for the function bodies
// to be available in headers, and the compiler is happy to just
// wait for the linker to find them. This is convenient for us
// because it allows us to not include headers from all of the
// save game modules in this translation unit.
template<typename T>
void savegame_serializer( FBBuilder&   builder,
                          FBOffset<T>* out_offset );
template<typename T>
valid_deserial_t savegame_deserializer( T const* src );
template<typename T>
valid_deserial_t savegame_post_validate( T const* );
template<typename T>
void default_construct_savegame_state( T const* );

namespace {

constexpr size_t kNumSavegameModules =
    fb::SaveGame::Traits::fields_number;

fs::path path_for_slot( int slot ) {
  CHECK( slot >= 0 );
  return config_savegame.folder /
         fmt::format( "slot{:02}", slot );
}

template<typename T>
FBOffset<T> serialize_to_offset( FBBuilder& fbb ) {
  FBOffset<T> state;
  savegame_serializer( fbb, &state );
  return state;
}

template<typename... Args>
auto creation_tuple( FBBuilder& fbb, mp::list<Args...>* ) {
  return tuple{
      serialize_to_offset<serial::remove_fb_offset_t<Args>>(
          fbb )... };
}

serial::BinaryBlob save_game_to_blob() {
  FBBuilder fbb;
  // This gets a tuple whose element types are the types
  // needed to be passed to the table creation method of
  // fb::SaveGame.
  using creation_types =
      serial::fb_creation_tuple_t<fb::SaveGame>;
  auto tpl = creation_tuple(
      fbb, static_cast<creation_types*>( nullptr ) );
  auto to_apply = [&]( auto const&... args ) {
    return fb::CreateSaveGame( fbb, args... );
  };
  auto sg = std::apply( to_apply, tpl );
  fbb.Finish( sg );
  return serial::BinaryBlob::from_builder( std::move( fbb ) );
}

template<size_t... Idxs>
valid_deserial_t savegame_post_validate_impl(
    std::index_sequence<Idxs...> ) {
  valid_deserial_t res = valid;

  auto validate_one = [&]<typename T>( T* p ) {
    // If we've already failed on a past step, don't run anymore.
    if( !res ) return;
    lg.debug( "running post-deserialization validation on {}.",
              base::demangled_typename<T>() );
    res = savegame_post_validate( p );
  };

  ( validate_one(
        fb::SaveGame::Traits::FieldType<Idxs>{ nullptr } ),
    ... );
  return res;
}

valid_deserial_t load_from_blob(
    serial::BinaryBlob const& blob ) {
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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<fs::path, generic_err> save_game( int slot ) {
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
}

expect<fs::path, generic_err> load_game( int slot ) {
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
}

valid_deserial_t reset_savegame_state() {
  FBBuilder fbb;
  auto      builder = fb::SaveGameBuilder( fbb );
  auto      sg      = builder.Finish();
  fbb.Finish( sg );
  auto blob =
      serial::BinaryBlob::from_builder( std::move( fbb ) );
  return load_from_blob( blob );
}

template<size_t... Idxs>
void default_construct_savegame_state_impl(
    std::index_sequence<Idxs...> ) {
  ( default_construct_savegame_state(
        fb::SaveGame::Traits::FieldType<Idxs>{ nullptr } ),
    ... );
}

void default_construct_savegame_state() {
  default_construct_savegame_state_impl(
      std::make_index_sequence<kNumSavegameModules>() );
}

/****************************************************************
** Testing
*****************************************************************/
void test_save_game() {}

} // namespace rn
