/****************************************************************
**custom-house.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-22.
*
* Description: All things related to the custom house.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "wait.hpp"

namespace rn {

struct Colony;
struct Planes;
struct TS;

// TODO: for custom house:
//
//  +1.  Requires Stuyvesant; once obtained, any colony can build
//       the building at any time.
//  +2.  Can click on the building and select which goods should
//       be sold.
//   3.  When a good reaches a quantity > 100 in a colony then
//       all but 50 of it is sold.
//   4.  It sells goods before spoilage is assessed.
//   5.  It can sell goods even if they are boycotted.
//   6.  Prior to the war of independence, custom house sales are
//       taxed according to the current tax rate, and there is no
//       further charge.
//   7.  If there is an enemy war ship within a couple of tiles
//       then the custom house is considered to be blockaded and
//       does not function.
//   8.  Even inland colonies can have custom houses, and those
//       cannot be blockaded. Though even colonies bordering
//       water without ocean access are considered inland (this
//       should be checked).
//   9.  Custom house sales still cause market prices to move,
//       but they can still only move one unit per turn.
//   10. The custom house can trade with europe even after the
//       war of independence. There is supposed to be a 50%
//       charge for smuggling in that case, though it does not
//       appear to be applied in the OG, and the strategy guide
//       says that "it does not work in the first released ver-
//       sion of the game." We will apply it in this game.

// Allows the user to adjust which commodities are being sold by
// the custom house.
wait<> open_custom_house_menu( Planes& planes, TS& ts,
                               Colony& colony );

// When the custom house is first built in a colony this should
// be called to set the default sellable commodities.
void set_default_custom_house_state( Colony& colony );

} // namespace rn
