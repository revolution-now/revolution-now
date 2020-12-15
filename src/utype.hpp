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
#include "coord.hpp"
#include "fb.hpp"
#include "land-square.hpp"
#include "mv-points.hpp"
#include "tiles.hpp"

// Flatbuffers
#include "fb/utype_generated.h"

namespace rn {

enum class ND e_unit_type {
  free_colonist,
  soldier,
  merchantman,
  privateer,
  large_treasure,
  small_treasure
};

enum class ND e_unit_death {
  destroy,
  naval,
  capture,
  demote,
  maybe_demote,
  demote_and_capture
};

// We need this for some weird reason -- if we dont' have it and
// we put the MOVABLE_ONLY in the UnitDescriptor class then we
// lose the aggregate constructor that we need in the cpp file.
// This allows us to be movable only but retain the aggregate
// constructor.
struct UnitDescriptorBase {
  UnitDescriptorBase() = default;
  MOVABLE_ONLY( UnitDescriptorBase );
};

// Static information describing classes of units. There will be
// one of these for each type of unit.
struct ND UnitDescriptor : public UnitDescriptorBase {
  std::string name{};
  e_unit_type type{};

  // Rendering
  e_tile      tile{};
  bool        nat_icon_front{};
  e_direction nat_icon_position{};

  // Movement
  bool     ship{};
  int      visibility{};
  MvPoints movement_points{};

  // Combat
  int  attack_points{};
  int  defense_points{};
  bool can_attack() const { return attack_points > 0; }
  bool is_military_unit() const { return can_attack(); }

  // FIXME: ideally these should be represented as an algebraic
  // data type in the config (and it should support loading
  // those). That way we would not have to do a runtime check
  // that `demoted` is set only if `on_death` has certain values.
  // When the unit loses a battle, what should happen?
  e_unit_death on_death{};
  // If the unit is to be demoted, what unit should it become?
  maybe<e_unit_type> demoted;

  // Cargo
  int      cargo_slots{};
  maybe<int> cargo_slots_occupies{}; // slots occupied by this unit

  void check_invariants() const;
};
NOTHROW_MOVE( UnitDescriptor );

UnitDescriptor const& unit_desc( e_unit_type type );

/****************************************************************
**Unit Movement Behaviors / Capabilities
*****************************************************************/
#define TEMPLATE_BEHAVIOR                                    \
  template<e_crust target, e_unit_relationship relationship, \
           e_entity_category entity>

#define BEHAVIOR_VALUES( crust, relationship, entity ) \
  e_crust::crust, e_unit_relationship::relationship,   \
      e_entity_category::entity

#define BEHAVIOR_NS( crust, relationship, entity ) \
  unit_behavior::crust::relationship::entity

#define BEHAVIOR( c, r, e, ... )                    \
  namespace BEHAVIOR_NS( c, r, e ) {                \
    enum class e_vals { __VA_ARGS__ };              \
  }                                                 \
  template<>                                        \
  struct to_behaviors<BEHAVIOR_VALUES( c, r, e )> { \
    using type = BEHAVIOR_NS( c, r, e )::e_vals;    \
  };                                                \
  template<>                                        \
  to_behaviors_t<BEHAVIOR_VALUES( c, r, e )>        \
  behavior<BEHAVIOR_VALUES( c, r, e )>(             \
      UnitDescriptor const& desc )

enum class e_unit_relationship { neutral, friendly, foreign };
enum class e_entity_category { empty, unit, colony, village };

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

/****************************************************************/
BEHAVIOR( land, foreign, unit, no_attack, attack, no_bombard,
          bombard );
BEHAVIOR( land, foreign, colony, never, attack, trade );
// BEHAVIOR( land, foreign, village, unused );
BEHAVIOR( land, neutral, empty, never, always, unload );
BEHAVIOR( land, friendly, unit, always, never, unload );
BEHAVIOR( land, friendly, colony, always );
BEHAVIOR( water, foreign, unit, no_attack, attack, no_bombard,
          bombard );
BEHAVIOR( water, neutral, empty, never, always );
BEHAVIOR( water, friendly, unit, always, never, move_onto_ship );
/****************************************************************/

// The macros below are for users of the above functions.

#define CALL_BEHAVIOR( crust_, relationship_, entity_ )         \
  behavior<e_crust::crust_, e_unit_relationship::relationship_, \
           e_entity_category::entity_>( unit.desc() )

// This is a bit of an abuse of the init-statement if the if
// statement. Here we are using it just to automate the calling
// of the behavior function, not to test the result of the
// behavior function.
#define IF_BEHAVIOR( crust_, relationship_, entity_ )       \
  if( auto bh =                                             \
          CALL_BEHAVIOR( crust_, relationship_, entity_ );  \
      crust == e_crust::crust_ &&                           \
      relationship == e_unit_relationship::relationship_ && \
      category == e_entity_category::entity_ )

// This one is used to assert that there is no specialization for
// the given combination of parameters. These can be used for all
// combinations of parameters that are not specialized so that,
// eventually, when they are specialized, the compiler can notify
// us of where we need to insert logic to handle that situation.
#define STATIC_ASSERT_NO_BEHAVIOR( crust, relationship, \
                                   entity )             \
  static_assert(                                        \
      is_same_v<void, decltype( CALL_BEHAVIOR(          \
                          crust, relationship, entity ) )> )

} // namespace rn
