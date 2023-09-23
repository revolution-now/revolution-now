/****************************************************************
**land-view-anim.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-09.
*
* Description: Implements entity animations in the land view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "land-view-anim.rds.hpp"

// Revolution Now
#include "wait.hpp"

// config
#include "config/tile-enum-fwd.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/dwelling-id.hpp"
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <queue>
#include <stack>

namespace rn {

struct AnimationAction;
struct AnimationSequence;
struct Colony;
struct Dwelling;
struct Visibility;

class SmoothViewport;

/****************************************************************
** LandViewAnimator
*****************************************************************/
struct LandViewAnimator {
  using UnitAnimStatesMap =
      std::unordered_map<GenericUnitId,
                         std::stack<UnitAnimationState>>;
  using DwellingAnimStatesMap =
      std::unordered_map<DwellingId,
                         std::stack<DwellingAnimationState>>;
  using ColonyAnimStatesMap =
      std::unordered_map<ColonyId,
                         std::stack<ColonyAnimationState>>;

  LandViewAnimator( SSConst const& ss, SmoothViewport& viewport,
                    Visibility const& viz )
    : ss_( ss ), viewport_( viewport ), viz_( viz ) {}

  // Getters.

  maybe<UnitAnimationState const&> unit_animation(
      UnitId id ) const;

  maybe<ColonyAnimationState const&> colony_animation(
      ColonyId id ) const;

  maybe<DwellingAnimationState const&> dwelling_animation(
      DwellingId id ) const;

  auto const& unit_animations() const {
    return unit_animations_;
  }

  auto const& colony_animations() const {
    return colony_animations_;
  }

  auto const& dwelling_animations() const {
    return dwelling_animations_;
  }

  // Animation sequences.

  wait<> animate_sequence( AnimationSequence const& seq );

  wait<> animate_blink( UnitId id, bool visible_initially );

  // Smooth map scrolling.

  wait<> ensure_visible( Coord const& coord );

  wait<> ensure_visible_unit( GenericUnitId id );

 private:
  // Animation primitives.

  wait<> animate_action_primitive(
      AnimationAction const& action );

  wait<> unit_depixelation_throttler(
      GenericUnitId id, maybe<e_tile> target_tile );

  wait<> unit_enpixelation_throttler( GenericUnitId id );

  wait<> colony_depixelation_throttler( Colony const& colony );

  wait<> dwelling_depixelation_throttler(
      Dwelling const& dwelling );

  wait<> slide_throttler( GenericUnitId id, e_direction d );

 private:
  template<typename Anim, typename Map>
  struct [[nodiscard]] Popper {
    using Id = typename Map::key_type;

    Popper( Map& m, Id id )
      : m_( m ),
        id_( id ),
        // Creates a new Anim{} and pushes it to the top of the
        // stack.
        anim_( m_[id_].emplace().template emplace<Anim>() ) {}

    ~Popper() noexcept {
      CHECK( m_.contains( id_ ) );
      CHECK( !m_[id_].empty() );
      CHECK( m_[id_].top().template holds<Anim>() );
      m_[id_].pop();
      if( m_[id_].empty() ) m_.erase( id_ );
    }

    Popper( Popper const& )            = delete;
    Popper& operator=( Popper const& ) = delete;
    Popper( Popper&& )                 = delete;
    Popper& operator=( Popper&& )      = delete;

    Anim& get() const { return anim_; }

   private:
    Map&  m_;
    Id    id_;
    Anim& anim_;
  };

  template<typename Anim, typename Map>
  auto make_popper( Map& m, typename Map::key_type id ) {
    return Popper<Anim, Map>( m, id );
  }

  template<typename Anim>
  auto add_unit_animation( GenericUnitId id ) {
    return make_popper<Anim>( unit_animations_, id );
  }

  template<typename Anim>
  auto add_colony_animation( ColonyId id ) {
    return make_popper<Anim>( colony_animations_, id );
  }

  template<typename Anim>
  auto add_dwelling_animation( DwellingId id ) {
    return make_popper<Anim>( dwelling_animations_, id );
  }

 private:
  // Note: SSConst should be held by value.
  SSConst const         ss_;
  SmoothViewport&       viewport_;
  UnitAnimStatesMap     unit_animations_;
  ColonyAnimStatesMap   colony_animations_;
  DwellingAnimStatesMap dwelling_animations_;
  Visibility const&     viz_;
};

} // namespace rn
