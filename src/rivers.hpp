/****************************************************************
**rivers.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-14.
*
* Description: Handles river generation.
*
*****************************************************************/
#pragma once

// rds
#include "rivers.rds.hpp"

// config
#include "config/range-helpers.rds.hpp"

// base
#include "base/function-ref.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct MapMatrix;
struct RiverParameters;
struct WeatherValue;

/****************************************************************
** Public API.
*****************************************************************/
RiverParameters derive_river_parameters( double scale,
                                         WeatherValue climate );

void add_rivers( MapMatrix& m, IRand& rand,
                 RiverParameters const& params );

[[nodiscard]] int count_rivers( MapMatrix const& m );

} // namespace rn
