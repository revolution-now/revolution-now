/****************************************************************
* Nation Config File
*****************************************************************/
#ifndef NATION_INL
#define NATION_INL

// Revolution Now
#include "enum-map.hpp"

// Rds
#include "nation.rds.hpp"

// C++ standard library
#include <string>

namespace rn {

using NationalityMap = EnumMap<e_nation, Nationality>;

CFG( nation,
  FLD( NationalityMap, nations )
)

}

#endif
