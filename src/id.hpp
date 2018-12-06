/****************************************************************
**id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles IDs.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "typed-int.hpp"

TYPED_ID( UnitId ) // NOLINTNEXTLINE(hicpp-explicit-conversions)

namespace rn {

ND UnitId next_unit_id();

} // namespace rn

namespace std {

DEFINE_HASH_FOR_TYPED_INT( ::rn::UnitId )

} // namespace std
