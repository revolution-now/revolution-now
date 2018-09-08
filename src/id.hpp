/****************************************************************
* id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles IDs.
*
*****************************************************************/
#pragma once

#include "typed-int.hpp"

namespace rn {

TYPED_ID( UnitId )

UnitId next_unit_id();

} // namespace rn

namespace std {

DEFINE_HASH_FOR_TYPED_INT( ::rn::UnitId )

} // namespace std
