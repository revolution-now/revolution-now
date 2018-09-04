/****************************************************************
* nation.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#pragma once

namespace rn {

enum class e_nation {
  dutch,
  french,
  english,
  spanish
};

e_nation player_nationality();

} // namespace rn
