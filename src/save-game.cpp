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
#include "config-files.hpp"
#include "fb.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "serial.hpp"

// Revolution Now (save-state modules)
#include "id.hpp"
#include "player.hpp"
#include "terrain.hpp"
#include "turn.hpp"
#include "ustate.hpp"

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

namespace {

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
auto creation_tuple( FBBuilder& fbb, tuple<Args...>* ) {
  return tuple{
      serialize_to_offset<serial::remove_fb_offset_t<Args>>(
          fbb )... };
}

// FIXME: needs to wait until we can access the getters via types
// only.
// template<typename... Args>
// expect<> deserialize_all( serial::BinaryBlob const& blob,
//                          tuple<Args...>* ) {
//  expect<> res;

//  auto single = [&]( auto* root ) {
//    // If we already have an error than return.
//    if( !res ) return;
//    res = savegame_deserializer( root );
//  };

//  ( single(
//        blob.template root<serial::remove_fb_offset_t<Args>>()
//        ),
//    ... );
//  return res;
//}

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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<fs::path> save_game( int slot ) {
  constexpr int   trials = 10;
  util::StopWatch watch;
  watch.start( "save" );
  auto blob = [&] {
    for( int i = trials; i >= 1; --i ) {
      auto blob = save_game_to_blob();
      if( i == 1 ) return blob;
    }
    UNREACHABLE_LOCATION;
  }();
  watch.stop( "save" );
  lg.info( "saving game ({} trials) took: {}", trials,
           watch.human( "save" ) );
  auto p = path_for_slot( slot );
  p.replace_extension( ".sav" );
  lg.info( "saving game to {}.", p );
  XP_OR_RETURN_( blob.write( p ) );
  auto json = blob.to_json<fb::SaveGame>( /*quotes=*/false );
  p.replace_extension( ".jsav" );
  ofstream out( p );
  if( !out.good() )
    return UNEXPECTED( "failed to open {} for writing.", p );
  out << json;
  p.replace_extension();
  return p;
}

expect<fs::path> load_game( int slot ) {
  auto p = path_for_slot( slot );
  p.replace_extension( ".sav" );
  lg.info( "loading game from {}.", p );
  XP_OR_RETURN( blob, serial::BinaryBlob::read( p ) );
  // FIXME: needs to wait until we can access the getters via
  // types only.
  // using creation_types =
  //    serial::fb_creation_tuple_t<fb::SaveGame>;
  // XP_OR_RETURN_( deserialize_all(
  //    blob, static_cast<creation_types*>( nullptr ) ) );
  auto* root = blob.root<fb::SaveGame>();
  XP_OR_RETURN_( savegame_deserializer( root->id_state() ) );
  XP_OR_RETURN_( savegame_deserializer( root->unit_state() ) );
  XP_OR_RETURN_( savegame_deserializer( root->player_state() ) );
  XP_OR_RETURN_(
      savegame_deserializer( root->terrain_state() ) );
  XP_OR_RETURN_( savegame_deserializer( root->turn_state() ) );
  return p;
}

/****************************************************************
** Testing
*****************************************************************/
void test_save_game() {}

} // namespace rn
