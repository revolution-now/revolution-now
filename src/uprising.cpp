/****************************************************************
**uprising.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-15.
*
* Description: Implements the "Tory Uprising" mechanic.
*
*****************************************************************/
#include "uprising.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API.
*****************************************************************/
UprisingColonies find_uprising_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player colonial_player_type ) {
  UprisingColonies res;
  (void)ss;
  (void)connectivity;
  (void)colonial_player_type;
  return res;
}

UprisingColony const& select_uprising_colony(
    IRand& rand, UprisingColonies const& uprising_colonies ) {
  CHECK( !uprising_colonies.colonies.empty() );
  (void)rand;
  (void)uprising_colonies;
  return uprising_colonies.colonies[0];
}

vector<e_unit_type> generate_uprising_units( IRand& rand,
                                             int const count ) {
  vector<e_unit_type> res;
  res.reserve( count );
  (void)rand;
  (void)count;
  return res;
}

void deploy_uprising_units(
    SS& ss, UprisingColony const& uprising_colony,
    vector<e_unit_type> const& unit_types ) {
  (void)ss;
  (void)uprising_colony;
  (void)unit_types;
}

wait<> show_uprising_msg(
    SSConst const& ss, IGui& gui,
    UprisingColony const& uprising_colony ) {
  (void)ss;
  (void)gui;
  (void)uprising_colony;
  co_return;
}

} // namespace rn
