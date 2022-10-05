/****************************************************************
**tax.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Handles things related to tax increases and
*              boycotts.
*
*****************************************************************/
#include "tax.hpp"

// Revolution Now
#include "co-wait.hpp"

// ss
#include "ss/ref.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<TaxationUpdate> compute_tax_change( SSConst const&,
                                          Player const& ) {
  // TODO
  return nothing;
}

wait<e_tax_proposal_answer> prompt_player_for_tax_change(
    SSConst const&, TS&, Player const&, TaxationUpdate const& ) {
  // TODO
  co_return {};
}

void apply_tax_change( SS&, TS&, Player&,
                       TaxationUpdate const& ) {
  // TODO
}

wait<> start_of_turn_tax_check( SS& ss, TS& ts,
                                Player& player ) {
  maybe<TaxationUpdate> const update =
      compute_tax_change( ss, player );
  if( !update.has_value() ) co_return;
  e_tax_proposal_answer const answer =
      co_await prompt_player_for_tax_change( ss, ts, player,
                                             *update );
  if( answer == e_tax_proposal_answer::reject ) co_return;
  apply_tax_change( ss, ts, player, *update );
}

} // namespace rn
