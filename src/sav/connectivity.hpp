/****************************************************************
**connectivity.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-12-20.
*
* Description: Algorithm to populate the map connectivity
*              sections of the SAV file in a way that is intended
*              to reproduce the OG's intended behavior.
*
*****************************************************************/
#pragma once

// sav
#include "sav-struct.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <vector>

namespace sav {

// This function populates the region IDs in the PATH section
// given a tile map. It will attempt to use the same region ID
// convention as the OG. Note that the other field
// (visitor_nation) must be populated separately.
void populate_region_ids( std::vector<TILE> const& tiles,
                          std::vector<PATH>& path );

// This function populates the connectivity sections the way they
// were likely intended to be populated, namely without what ap-
// pears to be a bug in the OG's algorithm.
//
// See documentation accompanying SAV schema for a description of
// the algorithm, which is a bit complicated.
//
// OG Bug details: The bug causes the neast and swest fields to
// be populated incorrectly between two diagonally connected
// quads. E.g.
//
//                     +-+-+-+-+-+-+-+-+
//                     |       |       |
//                     |       |   b   |
//                     |       |       |
//                     +-+-+-+-+-+-+-+-+
//                     |       |       |
//                     |   a   |       |
//                     |       |       |
//                     +-+-+-+-+-+-+-+-+
//
// If quads a and b are deemed to be connected according to the
// correct version of the algorithm, then it is usually recorded
// as such, as one would expect, by giving quad a a neast bit and
// quad b a swest bit. However, in a few cases this does not
// happen and both bits are suppressed, and it is not clear how
// to predict when this happens.
//
// This algorithm does not replicate that behavior.
//
// Note that this relies on the TILE map and PATH map (only the
// region IDs) being present and populated correctly.
void populate_connectivity( std::vector<TILE> const& tiles,
                            std::vector<PATH> const& path,
                            gfx::size map_size,
                            CONNECTIVITY& connectivity );

} // namespace sav
