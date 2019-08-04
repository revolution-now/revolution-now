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
#include "coord.hpp"
#include "enum.hpp"
#include "mv-points.hpp"
#include "terrain.hpp"
#include "tiles.hpp"

namespace rn {

enum class ND e_( unit_type, free_colonist, soldier, caravel,
                  privateer, large_treasure, small_treasure );

enum class ND e_( unit_death, destroy, naval, capture, demote,
                  maybe_demote, demote_and_capture );

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
  int  attack_points{};
  int  defense_points{};
  bool can_attack() const { return attack_points > 0; }

  // FIXME: ideally these should be represented as an algebraic
  // data type in the config (and it should support loading
  // those). That way we would not have to do a runtime check
  // that `demoted` is set only if `on_death` has certain values.
  // When the unit loses a battle, what should happen?
  e_unit_death on_death{};
  // If the unit is to be demoted, what unit should it become?
  Opt<e_unit_type> demoted;

  // Cargo
  int      cargo_slots{};
  Opt<int> cargo_slots_occupies{}; // slots occupied by this unit

  void check_invariants() const;
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

/****************************************************************/
BEHAVIOR( land, foreign, unit, no_attack, attack, no_bombard,
          bombard );
// BEHAVIOR( land, foreign, colony, unused );
// BEHAVIOR( land, foreign, village, unused );
BEHAVIOR( land, neutral, empty, never, always, unload );
BEHAVIOR( land, friendly, unit, always, never, unload );
// BEHAVIOR( land, friendly, colony, always, move_into_dock );
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
#define IF_BEHAVIOR( crust_, relationship_, entity_ )        \
  if( auto bh =                                              \
          CALL_BEHAVIOR( crust_, relationship_, entity_ );   \
      crust == +e_crust::crust_ &&                           \
      relationship == +e_unit_relationship::relationship_ && \
      category == +e_entity_category::entity_ )

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
