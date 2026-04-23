/****************************************************************
**map-stats.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-22.
*
* Description: Some map statistics collectors.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "imap-stats.hpp"

// C++ standard library
#include <memory>

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
std::unique_ptr<IMapStatsCollector>
create_biome_density_stats_collector( std::string const& stem );

std::unique_ptr<IMapStatsCollector>
create_biome_wetness_stats_collector();

} // namespace rn
