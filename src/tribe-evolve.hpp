/****************************************************************
**tribe-evolve.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-24.
*
* Description: Evolves a tribe's dwelling by one turn.
*
*****************************************************************/
#pragma once

namespace rn {

struct SS;
struct TS;

enum class e_tribe;

void evolve_dwellings_for_tribe( SS& ss, TS& ts,
                                 e_tribe tribe_type );

// Evolve those aspects/properties of the tribe that are common
// to the entire tribe, i.e. not dwellingor unit-specific.
void evolve_tribe_common( SS& ss, TS& ts, e_tribe tribe_type );

} // namespace rn
