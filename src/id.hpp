/****************************************************************
**id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles IDs.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "sg-macros.hpp"
#include "typed-int.hpp"

TYPED_ID( UnitId )
UD_LITERAL( UnitId, id )

TYPED_ID( ColonyId )

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Id );

ND UnitId   next_unit_id();
ND ColonyId next_colony_id();

UnitId   last_unit_id();
ColonyId last_colony_id();

/****************************************************************
** Testing
*****************************************************************/
namespace testing_only {
void reset_all_ids();
}

} // namespace rn

namespace std {

DEFINE_HASH_FOR_TYPED_INT( ::rn::UnitId )
DEFINE_HASH_FOR_TYPED_INT( ::rn::ColonyId )

} // namespace std
