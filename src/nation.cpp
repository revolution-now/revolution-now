/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#include "nation.hpp"

// Revolution Now
#include "config-files.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <cctype>

#define MAKE_NATION( __name )                               \
  m[e_nation::__name] = Nation {                            \
    TO_STRING( __name ), config_nation.__name.country_name, \
        *config_nation.__name.flag_color                    \
  }

using namespace std;

namespace rn {

namespace {} // namespace

Nation const& nation_obj( e_nation nation ) {
  static absl::flat_hash_map<e_nation, Nation> nations = [] {
    absl::flat_hash_map<e_nation, Nation> m;
    MAKE_NATION( dutch );
    MAKE_NATION( french );
    MAKE_NATION( english );
    MAKE_NATION( spanish );
    CHECK( m.size() == g_num_nations );
    return m;
  }();
  return nations[nation];
}

// disabled until there is an AI.
// e_nation player_nation() { return config_rn.player_nation; }

string Nation::name_proper() const {
  string res = name_lowercase;
  if( res.empty() ) return res;
  res[0] = std::toupper( res[0] );
  return res;
}

array<e_nation, g_num_nations> const& all_nations() {
  static constexpr array<e_nation, g_num_nations> nations{
      e_nation::dutch, e_nation::french, e_nation::english,
      e_nation::spanish};
  static_assert( nations.size() == g_num_nations );
  return nations;
}

} // namespace rn
