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
#include "aliases.hpp"
#include "cc-specific.hpp"
#include "config-files.hpp"
#include "fb.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "serial.hpp"

// base
#include "base/io.hpp"
#include "base/meta.hpp"

// Revolution Now (config)
#include "../config/ucl/savegame.inl"

// Flatbuffers
#include "fb/save-game_generated.h"

// base-util
#include "base-util/stopwatch.hpp"

// C++ standard library
#include <fstream>

using namespace std;

namespace rn {

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
expect<> savegame_deserializer( T const* src );
template<typename T>
expect<> savegame_post_validate( T const* );
template<typename T>
void default_construct_savegame_state( T const* );

namespace {

using fb_sg_types = mp::to_type_list_t<fb::SaveGame::FieldTypes>;
constexpr size_t num_savegame_modules =
    std::tuple_size_v<fb::SaveGame::FieldTypes>;

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
auto creation_tuple( FBBuilder& fbb, mp::type_list<Args...>* ) {
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

template<typename... Ts>
expect<> savegame_post_validate_impl( mp::type_list<Ts...>* ) {
  expect<> res = xp_success_t{};

  auto validate_one = [&]( auto* p ) {
    // If we've already failed on a past step, don't run anymore.
    if( !res ) return;
    lg.debug( "running post-deserialization validation on {}.",
              demangled_typename<
                  std::remove_pointer_t<decltype( p )>>() );
    res = savegame_post_validate( p );
  };

  ( validate_one( Ts{ nullptr } ), ... );
  return res;
}

expect<> load_from_blob( serial::BinaryBlob const& blob ) {
  auto*           root = blob.root<fb::SaveGame>();
  util::StopWatch watch;
  watch.start( "load" );
  auto     fields_pack = root->fields_pack();
  expect<> res         = xp_success_t{};
  mp::for_index_seq<num_savegame_modules>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        res = savegame_deserializer(
            std::get<Idx>( fields_pack ) );
        if( !res ) return true;
        return false;
      } );
  if( !res ) return res;
  watch.stop( "load" );

  // Post-deserialization validation.
  watch.start( "validate" );
  XP_OR_RETURN_(
      savegame_post_validate_impl( (fb_sg_types*)0 ) );
  watch.stop( "validate" );

  lg.info( "loading game took: {}, validation took: {}.",
           watch.human( "load" ), watch.human( "validate" ) );
  return xp_success_t{};
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<fs::path> save_game( int slot ) {
  // Increase this to get more accurate reading on save times.
  constexpr int   trials = 1;
  util::StopWatch watch;
  watch.start( "save" );
  auto serialize_to_blob = [&]() -> expect<serial::BinaryBlob> {
    for( int i = trials; i >= 1; --i ) {
      serial::BinaryBlob blob = save_game_to_blob();
      if( i == 1 ) return blob; // has to move
    }
    UNREACHABLE_LOCATION;
  };
  XP_OR_RETURN( blob, serialize_to_blob() );
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
    return UNEXPECTED( "failed to open {} for writing.", p );
  out << blob.to_json<fb::SaveGame>( /*quotes=*/false );

  // Serialize to binary.
  p.replace_extension( ".sav" );
  lg.info( "saving game to {}.", p );
  XP_OR_RETURN_( blob.write( p ) );

  p.replace_extension();
  return p;
}

expect<fs::path> load_game( int slot ) {
  auto json_path =
      path_for_slot( slot ).replace_extension( ".jsav" );
  auto blob_path =
      path_for_slot( slot ).replace_extension( ".sav" );

  bool json_exists = fs::exists( json_path );
  bool blob_exists = fs::exists( blob_path );

  if( !blob_exists && !json_exists )
    return UNEXPECTED( "save files not found for slot {}.",
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
      return UNEXPECTED( "failed to read json file" );
    XP_OR_RETURN( blob, serial::BinaryBlob::from_json(
                            /*schema_file_name=*/"save-game.fbs",
                            /*json=*/*maybe_json,
                            /*root_type=*/"SaveGame" ) );
    XP_OR_RETURN_( load_from_blob( blob ) );
    return json_path;
  } else {
    lg.info( "loading game from {}.", blob_path );
    XP_OR_RETURN( blob, serial::BinaryBlob::read( blob_path ) );
    XP_OR_RETURN_( load_from_blob( blob ) );
    return blob_path;
  }
}

expect<> reset_savegame_state() {
  FBBuilder fbb;
  auto      builder = fb::SaveGameBuilder( fbb );
  auto      sg      = builder.Finish();
  fbb.Finish( sg );
  auto blob =
      serial::BinaryBlob::from_builder( std::move( fbb ) );
  return load_from_blob( blob );
}

template<typename... fb_SG_types>
void default_construct_savegame_state_impl(
    mp::type_list<fb_SG_types...>* ) {
  ( default_construct_savegame_state( fb_SG_types{ nullptr } ),
    ... );
}

void default_construct_savegame_state() {
  default_construct_savegame_state_impl( (fb_sg_types*)0 );
}

/****************************************************************
** Testing
*****************************************************************/
void test_save_game() {}

} // namespace rn
