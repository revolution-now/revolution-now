/****************************************************************
* Units Config File
*****************************************************************/
#ifndef UNITS_INL
#define UNITS_INL

#include "../../src/utype.hpp"

namespace rn {

CFG( units,
  // This includes a bunch of different fields, but they are
  // grouped here because they need to be validated together.
  FLD( UnitCompositionConfig, composition )
)

}

#endif
