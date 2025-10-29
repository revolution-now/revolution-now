/****************************************************************
**type-ext-base.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Type-traversing std types.
*
*****************************************************************/
#pragma once

// traverse
#include "type-ext.hpp"

// base
#include "base/maybe.hpp"

namespace trv {

TRV_TYPE_TRAVERSE( ::base::maybe, T );

} // namespace trv
