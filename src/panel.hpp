/****************************************************************
**panel.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-11-15.
*
* Description: Implementation for the panel.
*
*****************************************************************/
#pragma once

// rds
#include "panel.rds.hpp"

// ss
#include "ss/unit-id.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
PanelEntities entities_shown_on_panel( SSConst const& ss );

} // namespace rn
