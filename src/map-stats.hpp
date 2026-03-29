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
** Fwd. Decls.
*****************************************************************/
struct BiomeClustering;

/****************************************************************
** Public API.
*****************************************************************/
std::unique_ptr<IMapStatsCollector>
create_biome_density_stats_collector( std::string const& stem );

std::unique_ptr<IMapStatsCollector>
create_biome_adjacency_stats_collector(
    BiomeClustering const& clustering );

} // namespace rn
