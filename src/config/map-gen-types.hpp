/****************************************************************
**map-gen-types.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-17.
*
* Description: Config data for the map-gen-types module.
*
*****************************************************************/
#pragma once

#include "map-gen-types.rds.hpp"

namespace rn {

// This is the magnitude (+/-) that corresponds to the three op-
// tions presented by the OG when customizing the game (e.g.
// arid, normal, wet would be -100, 0, 100).
int constexpr kWeatherValueCustomizationMagnitude = 100;
// In both the OG and NG a weather value can actually be extended
// beyond the values representing the hard-coded customization
// points (e.g., a temperature value of 150 is "warmer than war-
// m").
int constexpr kWeatherValueMaxMagnitude = 200;

double constexpr kBiomeSelfAffinityMin = -3.0;
double constexpr kBiomeSelfAffinityMax = 3.0;

} // namespace rn
