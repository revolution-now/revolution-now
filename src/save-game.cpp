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
#include "serial.hpp"

// Revolution Now (save-state modules)
#include "id.hpp"
#include "ustate.hpp"

// Revolution Now (config)
#include "../config/ucl/savegame.inl"

// Flatbuffers
#include "fb/save-game_generated.h"

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

#define SERIALIZE_TO_OFFSET( name ) \
  serialize_to_offset<fb::SG_##name>( fbb )

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<fs::path> save_game( int slot ) {
  FBBuilder fbb;
  // clang-format off
  auto sg = fb::CreateSaveGame( fbb,
    // ==========================================================
    SERIALIZE_TO_OFFSET( Id             ),
    SERIALIZE_TO_OFFSET( Unit           )
    // ==========================================================
  );
  // clang-format on
  fbb.Finish( sg );
  auto blob =
      serial::BinaryBlob::from_builder( std::move( fbb ) );
  auto p = path_for_slot( slot );
  p.replace_extension( ".sav" );
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
  return p;
}

/****************************************************************
** Testing
*****************************************************************/
void test_save_game() {}

} // namespace rn
