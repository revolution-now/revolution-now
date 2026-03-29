/****************************************************************
**imap-stats.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-29.
*
* Description: Interface for collecting stats about the map.
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct MapMatrix;

/****************************************************************
** IMapStatsCollector
*****************************************************************/
// Map statistics are used for a number of reasons:
//   1. Calibrating the (fixed) config parameters in the config
//      files to reproduce OG behavior.
//   2. Testing map generation.
//   3. They are also used during in-game map generation as part
//      of iterative algorithms to determine when various proper-
//      ties of the map have reach target values.
struct IMapStatsCollector {
  virtual ~IMapStatsCollector() = default;

  virtual void collect( MapMatrix const& m ) = 0;
  virtual void summarize()                   = 0;
  virtual void write() const                 = 0;

  void collect_and_summarize( MapMatrix const& m ) {
    collect( m );
    summarize();
  }
};

} // namespace rn
