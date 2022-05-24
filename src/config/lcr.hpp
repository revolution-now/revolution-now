/****************************************************************
**lcr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-23.
*
* Description: Types used by the LCR config file.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "lcr-impl.rds.hpp"

namespace rn {

template<typename T>
using LcrProperty = EnumMap<e_lcr_explorer_bucket, T>;

} // namespace rn
