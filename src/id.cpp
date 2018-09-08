/****************************************************************
* id.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles IDs.
*
*****************************************************************/
#include "id.hpp"

namespace rn {

namespace {

int g_next_unit_id{ 0 };
  
} // namespace

UnitId next_unit_id() {
  return UnitId( g_next_unit_id++ );
}

} // namespace rn
