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

// C++ standard library
#include <vector>

namespace sav {

// This one populates the connectivity sections the way they were
// likely intended to be populated, namely without what appears
// to be a bug in the OG's algorithm. See below for info on the
// bug.
void populate_connectivity( std::vector<TILE> const& tiles,
                            std::vector<PATH> const& path,
                            CONNECTIVITY& connectivity );

// Do not use, since it has never worked totally correctly.
//
// This one attempts to replicate what appears to be a bug in the
// OG's implementation, but doesn't do so successfully in some
// cases.
//
// The bug causes the neast and swest fields to be populated in-
// correctly between two diagonally connected quads that are
// straddled by an unconnected quad to their north and west).
// E.g.:
//
//                   +-+-+-+-+-+-+-+-+-+-+-+-+
//                   |       |       |       |
//                   |       |       |       |
//                   |       |       |       |
//                   +-+-+-+-+-+-+-+-+-+-+-+-+
//                   |       |       |       |
//                   |       |   x   |   b   |
//                   |       |       |       |
//                   +-+-+-+-+-+-+-+-+-+-+-+-+
//                   |       |       |       |
//                   |       |   a   |       |
//                   |       |       |       |
//                   +-+-+-+-+-+-+-+-+-+-+-+-+
//
// In this case, if quad x is not connected to any surrounding
// quads then the neast/swest connectivity between a and b (if it
// exists) is sometimes incorrectly suppressed in a way that is
// difficult to predict in all cases. This algorithm does not
// replicate that.
//
// What is described above applies to the sea lane connectivity;
// for land connectivity the bug is similar except that it is
// sometimes observed to happen even when x has connectivity.
//
// Again, do not use this because there is no point in repli-
// cating the bug and it doesn't even do so correctly in all
// cases.
[[deprecated]] void populate_sea_lane_connectivity_with_bug(
    std::vector<TILE> const& tiles,
    std::vector<PATH> const& path, CONNECTIVITY& connectivity );

} // namespace sav
