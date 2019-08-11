/****************************************************************
**power.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-11.
*
* Description: Handles power/battery management and detection.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"

namespace rn {

enum class e_( power_state,        //
               unknown,            //
               on_battery,         //
               plugged_no_battery, //
               plugged_charging,   //
               plugged_charged     //
);

struct MachinePowerInfo {
  e_power_state power_state;
  // If known, this will be populated; [0, 100].
  Opt<int> battery_percentage{};
};

// This will return as much info about the power situation as the
// system can give us.
MachinePowerInfo machine_power_info();

} // namespace rn
