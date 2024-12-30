/****************************************************************
**monitor.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Computations related to monitor DPI.
*
*****************************************************************/
#pragma once

// rds
#include "monitor.rds.hpp"

namespace gfx {

Monitor monitor_properties( size physical_screen,
                            base::maybe<MonitorDpi> dpi );

ProcessedMonitorDpi post_process_monitor_dpi(
    MonitorDpi const& dpi );

} // namespace gfx
