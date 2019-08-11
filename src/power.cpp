/****************************************************************
**power.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-11.
*
* Description: Handles power/battery management and detection.
*
*****************************************************************/
#include "power.hpp"

// SDL
#include "SDL.h"

namespace rn {

namespace {} // namespace

MachinePowerInfo machine_power_info() {
  MachinePowerInfo info;

  int  seconds_left, battery_percent;
  auto status =
      ::SDL_GetPowerInfo( &seconds_left, &battery_percent );
  switch( status ) {
    // Not plugged in, running on the battery.
    case SDL_POWERSTATE_ON_BATTERY:
      info.power_state = e_power_state::on_battery;
      break;
    // Plugged in, no battery available.
    case SDL_POWERSTATE_NO_BATTERY:
      info.power_state = e_power_state::plugged_no_battery;
      break;
    // Plugged in, charging battery.
    case SDL_POWERSTATE_CHARGING:
      info.power_state = e_power_state::plugged_charging;
      break;
    // Plugged in, battery charged.
    case SDL_POWERSTATE_CHARGED:
      info.power_state = e_power_state::plugged_charged;
      break;
    // Cannot determine power status.
    case SDL_POWERSTATE_UNKNOWN:
    default: //
      info.power_state = e_power_state::unknown;
      break;
  }
  if( battery_percent != -1 )
    info.battery_percentage = battery_percent;
  return info;
}

} // namespace rn
