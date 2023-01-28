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

struct Colony;
struct Dwelling;
class SmoothViewport;

/****************************************************************
** LandViewAnimator
*****************************************************************/
struct LandViewAnimator {
  using UnitAnimStatesMap =
      std::unordered_map<GenericUnitId,
                         std::stack<UnitAnimationState_t>>;
  using DwellingAnimStatesMap =
      std::unordered_map<DwellingId,
                         std::stack<DwellingAnimationState_t>>;
  using ColonyAnimStatesMap =
      std::unordered_map<ColonyId,
                         std::stack<ColonyAnimationState_t>>;

  LandViewAnimator( SSConst const& ss, SmoothViewport& viewport )
    : ss_( ss ), viewport_( viewport ) {}

  // Getters.

  maybe<UnitAnimationState_t const&> unit_animation(
      UnitId id ) const;

  maybe<ColonyAnimationState_t const&> colony_animation(
      ColonyId id ) const;

  maybe<DwellingAnimationState_t const&> dwelling_animation(
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

  wait<> animate_move( UnitId id, e_direction direction );

  wait<> animate_attack(
      GenericUnitId attacker, GenericUnitId defender,
      std::vector<PixelationAnimation_t> const& animations,
      bool                                      attacker_wins );

  wait<> animate_unit_pixelation(
      PixelationAnimation_t const& what );

  wait<> animate_colony_destruction( Colony const& colony );

  wait<> animate_colony_capture(
      UnitId attacker_id, UnitId defender_id,
      std::vector<PixelationAnimation_t> const& animations,
      ColonyId                                  colony_id );

  // Animator primitives.

  wait<> animate_unit_depixelation( GenericUnitId id,
                                    maybe<e_tile> target_tile );

  wait<> animate_unit_enpixelation( GenericUnitId id,
                                    e_tile        target_tile );

  wait<> animate_colony_depixelation( Colony const& colony );

  wait<> animate_dwelling_depixelation(
      Dwelling const& dwelling );

  wait<> animate_blink( UnitId id, bool visible_initially );

  wait<> animate_slide( GenericUnitId id, e_direction d );

  // Smooth map scrolling.

  wait<> ensure_visible( Coord const& coord );

  wait<> ensure_visible_unit( GenericUnitId id );

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

 public:
  // Animation adders.

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
  wait<> start_pixelation_animation(
      PixelationAnimation_t anim );

  std::vector<wait<>> start_pixelation_animations(
      std::vector<PixelationAnimation_t> const& anims );

 private:
  // Note: SSConst should be held by value.
  SSConst const         ss_;
  SmoothViewport&       viewport_;
  UnitAnimStatesMap     unit_animations_;
  ColonyAnimStatesMap   colony_animations_;
  DwellingAnimStatesMap dwelling_animations_;
};

} // namespace rn
