/****************************************************************
**monitor.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Computations related to monitor DPI.
*
*****************************************************************/
#include "monitor.hpp"

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
ProcessedMonitorDpi post_process_monitor_dpi(
    MonitorDpi const& dpi ) {
  ProcessedMonitorDpi res;
  double hdpi = dpi.horizontal;
  double vdpi = dpi.vertical;
  double ddpi = dpi.diagonal;
  res.info_logs.push_back( fmt::format(
      "raw monitor DPI: horizontal={}, vertical={}, diagonal={}",
      hdpi, vdpi, ddpi ) );
  if( hdpi != vdpi )
    res.warning_logs.push_back(
        fmt::format( "horizontal DPI not equal to vertical DPI.",
                     hdpi, vdpi ) );

  // The above function may not provide all of the components,
  // and/or it may return inconsistent or invalid components.
  // Thus, we now do our best to reconstruct any missing or in-
  // valid ones if we have enough information.

  static auto good = []( double const d ) {
    // Any sensible DPI should surely be larger than 1 pixel
    // per inch. If not then it is probably invalid.
    return !isnan( d ) && d > 1.0;
  };
  static auto bad = []( double const d ) { return !good( d ); };

  // Geometrically, the diagonal DPI should always be >= to the
  // cardinal DPIs. If it is not then one of the components may
  // not be reliable. In those cases that were observed, it was
  // the case that the diagonal one was the accurate one, so
  // let's nuke the cardinal one and hope that we'll be able to
  // recompute it below.
  if( bad( ddpi ) ) {
    res.warning_logs.push_back( fmt::format(
        "diagonal DPI is not available from hardware." ) );
  } else if( good( hdpi ) && hdpi <= ddpi ) {
    res.warning_logs.push_back( fmt::format(
        "horizontal DPI is less than diagonal DPI, thus one "
        "of them is inaccurate and must be discarded.  Will "
        "assume that the diagonal one is accurate." ) );
    hdpi = 0.0;
  } else if( good( vdpi ) && vdpi <= ddpi ) {
    res.warning_logs.push_back( fmt::format(
        "vertical DPI is less than diagonal DPI, thus one of "
        "them is inaccurate and must be discarded.  Will "
        "assume that the diagonal one is accurate." ) );
    vdpi = 0.0;
  }

  if( good( hdpi ) && bad( vdpi ) ) vdpi = hdpi;
  if( bad( hdpi ) && good( vdpi ) ) hdpi = vdpi;
  // At this point hdpi/vdpi are either both good or both bad.
  CHECK_EQ( good( hdpi ), good( vdpi ) );

  bool const have_cardinal = good( hdpi );
  bool const have_diagonal = good( ddpi );

  if( !have_cardinal )
    res.warning_logs.push_back(
        fmt::format( "neither horizontal nor vertical DPIs are "
                     "available." ) );

  if( !have_cardinal && !have_diagonal ) {
    // Can't do anything here;
    res.warning_logs.push_back(
        fmt::format( "no DPI information can be obtained." ) );
    return res;
  }

  if( have_cardinal && have_diagonal ) {
    res.dpi = MonitorDpi{
      .horizontal = hdpi, .vertical = vdpi, .diagonal = ddpi };
    return res;
  }

  if( !have_cardinal && have_diagonal ) {
    res.warning_logs.push_back( fmt::format(
        "cardinal DPI must be inferred from diagonal DPI." ) );
    // Assume a square; won't be perfect, but good enough.
    res.dpi = MonitorDpi{ .horizontal = ddpi / 1.41421,
                          .vertical   = vdpi / 1.41421,
                          .diagonal   = ddpi };
    return res;
  }

  if( have_cardinal && !have_diagonal ) {
    res.warning_logs.push_back( fmt::format(
        "diagonal DPI must be inferred from cardinal DPI." ) );
    res.dpi = MonitorDpi{
      .horizontal = hdpi,
      .vertical   = vdpi,
      .diagonal   = sqrt( hdpi * hdpi + vdpi * vdpi ) };
    return res;
  }

  res.error_logs.push_back( fmt::format(
      "something went wrong while computing DPI." ) );
  return res;
}

Monitor monitor_properties( size const physical_screen,
                            maybe<MonitorDpi> const dpi ) {
  Monitor monitor;
  monitor.physical_screen = physical_screen;
  if( dpi.has_value() ) {
    monitor.dpi = dpi;
    monitor.diagonal_inches =
        sqrt( pow( physical_screen.w, 2.0 ) +
              pow( physical_screen.h, 2.0 ) ) /
        dpi->diagonal;
  }
  return monitor;
}

} // namespace gfx
