/****************************************************************
**human-euro-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IEuroMind for human players.
*
*****************************************************************/
#include "human-euro-mind.hpp"

using namespace std;

namespace rn {

/****************************************************************
** HumanEuroMind
*****************************************************************/
HumanEuroMind::HumanEuroMind( e_nation nation, SS& ss,
                              IGui& gui )
  : nation_( nation ), ss_( ss ), gui_( gui ) {
  // TODO: to suppress unused variable warnings.
  (void)ss_;
  (void)gui_;
}

// Implement IEuroMind.
e_nation HumanEuroMind::nation() const { return nation_; }

} // namespace rn
