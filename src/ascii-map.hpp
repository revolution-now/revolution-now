/****************************************************************
**ascii-map.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Draws maps to the terminal for tools/testing.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <iosfwd>

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct RealTerrain;

/****************************************************************
** Public API.
*****************************************************************/
void print_ascii_map( RealTerrain const& terrain,
                      std::ostream& out );

} // namespace rn
