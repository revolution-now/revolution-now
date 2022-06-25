/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#include "fathers.hpp"

// config
#include "config/fathers.rds.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** e_founding_father
*****************************************************************/
string_view founding_father_name( e_founding_father father ) {
  return config_fathers.fathers[father].name;
}

/****************************************************************
** e_founding_father_type
*****************************************************************/
e_founding_father_type founding_father_type(
    e_founding_father father ) {
  return config_fathers.fathers[father].type;
}

vector<e_founding_father> founding_fathers_for_type(
    e_founding_father_type type ) {
  vector<e_founding_father> res;
  for( e_founding_father father :
       refl::enum_values<e_founding_father> )
    if( config_fathers.fathers[father].type == type )
      res.push_back( father );
  return res;
}

string_view founding_father_type_name(
    e_founding_father_type type ) {
  return config_fathers.types[type].name;
}

void linker_dont_discard_module_fathers() {}

} // namespace rn
