/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#include "land-view.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "coord.hpp"
#include "fb.hpp"
#include "fsm.hpp"
#include "id.hpp"
#include "matrix.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "tx.hpp"
#include "utype.hpp"
#include "viewport.hpp"

// Flatbuffers
#include "fb/sg-land-view_generated.h"

using namespace std;

namespace rn {

namespace {
struct Transient {
  // slide_unit

  // Note that mag_acceleration is not relevant here.
  DissipativeVelocity percent_vel{
      /*min_velocity=*/0,
      /*max_velocity=*/.07,
      /*initial_velocity=*/.1,
      /*mag_acceleration=*/1,
      /*mag_drag_acceleration=*/.002 };

  // depixelate_unit

  std::vector<Coord> all_pixels{};
  Texture            tx_from;
  Opt<Matrix<Color>> demote_pixels{};
};

/****************************************************************
** FSMs
*****************************************************************/
// The viewport rendering states are not really states of the
// world, they are mainly just animation or rendering states.
// Each state is represented by a struct which may contain data
// members.  The data members of the struct's will be mutated in
// in order to change/advance the state of animation, although
// the rendering functions themselves will never mutate them.
adt_s_rn_(
    LandViewState,    //
    ( end_of_turn ),  //
    ( blink_unit,     //
      ( UnitId, id ), //
      // Orders given to `id` by player.
      ( Opt<orders_t>, orders ), //
      // Units that the player has asked to prioritize (i.e.,
      // bring them forward in the queue of units waiting for or-
      // ders).
      ( Vec<UnitId>, prioritize ), //
      // Units that the player has asked to add to the orders
      // queue but at the end. This is useful if a unit that is
      // sentry'd has already been removed from the queue
      // (without asking for orders) and later in the same turn
      // had its orders cleared by the player (but not priori-
      // tized), this will allow it to ask for orders this turn.
      ( Vec<UnitId>, add_to_back ) ),         //
    ( slide_unit,                             //
      ( UnitId, id ),                         //
      ( Coord, target ),                      //
      ( double, percent ),                    //
      ( DissipativeVelocity, percent_vel ) ), //
    ( depixelate_unit,                        //
      ( UnitId, id ),                         //
      ( bool, finished ) )                    //
);

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( LandView ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( LandView,
  ( bool, b ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( LandView );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).
    return xp_success_t{};
  }
};
SAVEGAME_IMPL( LandView );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
// ...

/****************************************************************
** Testing
*****************************************************************/
void test_land_view() {
  //
}

} // namespace rn
