/****************************************************************
**utype.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-11.
*
* Description: Unit type descriptors.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"
#include "geo-types.hpp"
#include "mv-points.hpp"
#include "tiles.hpp"
#include "world.hpp"

namespace rn {

enum class ND e_unit_type { free_colonist, soldier, caravel };

// Static information describing classes of units. There will be
// one of these for each type of unit.
struct ND UnitDescriptor {
  std::string name{};
  e_unit_type type{};

  // Rendering
  g_tile      tile{};
  bool        nat_icon_front{};
  e_direction nat_icon_position{};

  // Movement
  bool     boat{};
  int      visibility{};
  MvPoints movement_points{};

  // Combat
  bool can_attack{};
  int  attack_points{};
  int  defense_points{};

  // Cargo
  int      cargo_slots{};
  Opt<int> cargo_slots_occupies{}; // slots occupied by this unit
};

UnitDescriptor const& unit_desc( e_unit_type type );

/****************************************************************
**Unit Movement Behaviors / Capabilities
*****************************************************************/
#define TEMPLATE_BEHAVIOR                                 \
  template<e_crust::_enumerated             target,       \
           e_unit_relationship::_enumerated relationship, \
           e_entity_category::_enumerated   entity>

#define BEHAVIOR_VALUES( crust, relationship, entity ) \
  e_crust::crust, e_unit_relationship::relationship,   \
      e_entity_category::entity

#define BEHAVIOR_NS( crust, relationship, entity ) \
  unit_behavior::crust::relationship::entity

#define BEHAVIOR( c, r, e, ... )                    \
  namespace BEHAVIOR_NS( c, r, e ) {                \
    enum class e_( vals, __VA_ARGS__ );             \
  }                                                 \
  template<>                                        \
  struct to_behaviors<BEHAVIOR_VALUES( c, r, e )> { \
    using type = BEHAVIOR_NS( c, r, e )::e_vals;    \
  };                                                \
  template<>                                        \
  to_behaviors_t<BEHAVIOR_VALUES( c, r, e )>        \
  behavior<BEHAVIOR_VALUES( c, r, e )>(             \
      UnitDescriptor const& desc )

enum class e_( unit_relationship, neutral, friendly, foreign );
enum class e_( entity_category, empty, unit, colony, village );

TEMPLATE_BEHAVIOR
struct to_behaviors {
  // This must be void, it is used as a fallback in code that
  // wants to verify that a combination of parameters is NOT
  // implemented.
  using type = void;
};

TEMPLATE_BEHAVIOR
using to_behaviors_t =
    typename to_behaviors<target, relationship, entity>::type;

TEMPLATE_BEHAVIOR
to_behaviors_t<target, relationship, entity> behavior(
    UnitDescriptor const& desc );

// BEHAVIOR( land, foreign, unit, nothing, attack, bombard );
// BEHAVIOR( land, foreign, colony, unused );
// BEHAVIOR( land, foreign, village, unused );
BEHAVIOR( land, neutral, empty, never, always, unload );
BEHAVIOR( land, friendly, unit, always, never, unload );
// BEHAVIOR( land, friendly, colony, always, move_into_dock );
// BEHAVIOR( water, foreign, unit, nothing, attack, bombard );
BEHAVIOR( water, neutral, empty, never, always );
BEHAVIOR( water, friendly, unit, always, never, move_onto_ship );

} // namespace rn
