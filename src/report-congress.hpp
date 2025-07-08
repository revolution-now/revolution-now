/****************************************************************
**report-congress.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: The Continental Congress report screen.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IEngine;
struct IGui;
struct Planes;
struct SSConst;
struct Player;

wait<> show_continental_congress_report( IEngine& engine,
                                         SSConst const& ss,
                                         IGui& gui,
                                         Player const& player,
                                         Planes& planes );

} // namespace rn
