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

// Rds
#include "land-view-anim.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/attributes.hpp"

// C++ standard library
#include <stack>

namespace rn {

struct AnimationAction;
struct AnimationSequence;
struct Colony;
struct Dwelling;
struct IVisibility;

class SmoothViewport;

namespace co {
struct latch;
}

/****************************************************************
** LandViewAnimator
*****************************************************************/
struct LandViewAnimator {
  using UnitAnimStatesMap =
      std::unordered_map<GenericUnitId,
                         std::stack<UnitAnimationState>>;
  using DwellingAnimStatesMap =
      std::unordered_map<Coord,
                         std::stack<DwellingAnimationState>>;
  using ColonyAnimStatesMap =
      std::unordered_map<Coord,
                         std::stack<ColonyAnimationState>>;

  LandViewAnimator(
      SSConst const& ss, SmoothViewport& viewport,
      std::unique_ptr<IVisibility const> const& viz );

  // Getters.

  maybe<UnitAnimationState const&> unit_animation(
      UnitId id ) const;

  maybe<ColonyAnimationState const&> colony_animation(
      Coord tile ) const;

  maybe<DwellingAnimationState const&> dwelling_animation(
      Coord tile ) const;

  auto const& unit_animations() const {
    return unit_animations_;
  }

  auto const& colony_animations() const {
    return colony_animations_;
  }

  auto const& dwelling_animations() const {
    return dwelling_animations_;
  }

  maybe<LandscapeAnimReplacementState> const&
  landview_anim_replacement_state() const {
    return landview_anim_replacement_state_;
  }

  maybe<LandscapeAnimEnpixelationState> const&
  landview_anim_enpixelation_state() const {
    return landview_anim_enpixelation_state_;
  }

  // Animation sequences.

  // Will animate the sequence then co_return. It is not uncommon
  // to being run in the background, thus for safety we include
  // the lifetime attribute.
  wait<> animate_sequence(
      AnimationSequence const& seq ATTR_LIFETIMEBOUND );

  // Will animate the sequence but will hold the end state of the
  // final subsequence. This will never co_return, thus it must
  // eventually be cancelled by the caller. It is not uncommon to
  // being run in the background, thus for safety we include the
  // lifetime attribute.
  wait<> animate_sequence_and_hold(
      AnimationSequence const& seq ATTR_LIFETIMEBOUND );

  wait<> animate_blink( UnitId id, bool visible_initially );

  // Smooth map scrolling.

  wait<> ensure_visible( Coord const& coord );

  wait<> ensure_visible_unit( GenericUnitId id );

 private:
  // Animation Sequences.
  wait<> animate_sequence_impl( AnimationSequence const& seq,
                                bool hold_last );

  // Animation primitives.

  wait<> animate_action_primitive( AnimationAction const& action,
                                   co::latch&             hold );

  wait<> unit_depixelation_throttler(
      co::latch& hold, UnitId id,
      maybe<e_unit_type> target_type );

  wait<> native_unit_depixelation_throttler(
      co::latch& hold, NativeUnitId id,
      maybe<e_native_unit_type> target_type );

  wait<> unit_enpixelation_throttler( co::latch&    hold,
                                      GenericUnitId id );

  wait<> colony_depixelation_throttler( co::latch&    hold,
                                        Colony const& colony );

  wait<> dwelling_depixelation_throttler( co::latch& hold,
                                          Coord      tile );

  std::vector<Coord> redrawn_squares_for_overrides(
      VisibilityOverrides const& overrides );

  wait<> landscape_anim_enpixelation_throttler(
      co::latch& hold, VisibilityOverrides const& overrides );

  wait<> landscape_anim_replacer(
      co::latch& hold, VisibilityOverrides const& modded );

  wait<> slide_throttler( co::latch& hold, GenericUnitId id,
                          e_direction d );

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
  auto add_colony_animation( Coord tile ) {
    return make_popper<Anim>( colony_animations_, tile );
  }

  template<typename Anim>
  auto add_dwelling_animation( Coord tile ) {
    return make_popper<Anim>( dwelling_animations_, tile );
  }

 private:
  SSConst const&        ss_;
  SmoothViewport&       viewport_;
  UnitAnimStatesMap     unit_animations_;
  ColonyAnimStatesMap   colony_animations_;
  DwellingAnimStatesMap dwelling_animations_;
  maybe<LandscapeAnimReplacementState>
      landview_anim_replacement_state_;
  maybe<LandscapeAnimEnpixelationState>
      landview_anim_enpixelation_state_;
  // We hold the unique_ptr reference because we need to know
  // when the source version was changed.
  std::unique_ptr<IVisibility const> const& viz_;
};

} // namespace rn
