/****************************************************************
**player.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#include "player.hpp"

// ss
#include "ss/fathers.hpp"
#include "ss/nation.hpp"
#include "ss/revolution.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

using ::base::valid;
using ::base::valid_or;

void linker_dont_discard_module_player();
void linker_dont_discard_module_player() {}

/****************************************************************
** Player
*****************************************************************/
valid_or<string> Player::validate() const {
  REFL_VALIDATE( nation_for( type ) == nation,
                 "Player {} has inconsistent type (={}) and "
                 "nation (={}).",
                 type, type, nation );

  return valid;
}

} // namespace rn
