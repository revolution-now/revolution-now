/****************************************************************
**land-view-render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-09.
*
* Description: Handles rendering for the land view.
*
*****************************************************************/
#include "land-view-render.hpp"

// Revolution Now
#include "land-view-anim.hpp"
#include "render.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"
#include "visibility.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/fog-square.rds.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/unit-id.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/iter.hpp"

// base
#include "base/keyval.hpp"

// C++ standard library
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

bool last_unit_input_is_in_stack_indirect(
    SSConst const& ss, UnitId last_unit_input,
    vector<GenericUnitId> const& units ) {
  maybe<UnitId> const holder =
      is_unit_onboard( ss.units, last_unit_input );
  for( GenericUnitId id : units ) {
    // Is this the unit we're looking for?
    if( id == last_unit_input ) return true;
    // Is this unit holding the unit we're looking for?
    if( id == holder ) return true;
  }
  return false;
}

} // namespace

// Given a tile, compute the screen rect where it should be ren-
// dered.
Rect LandViewRenderer::render_rect_for_tile( Coord tile ) const {
  Delta delta_in_tiles  = tile - covered_.upper_left();
  Delta delta_in_pixels = delta_in_tiles * g_tile_delta;
  return Rect::from( Coord{} + delta_in_pixels, g_tile_delta );
}

vector<GenericUnitId> land_view_unit_stack(
    SSConst const& ss, Coord tile,
    maybe<UnitId> last_unit_input ) {
  unordered_set<GenericUnitId> const& units =
      ss.units.from_coord( tile );
  vector<GenericUnitId> res;
  if( units.empty() ) return res;
  res = vector<GenericUnitId>( units.begin(), units.end() );
  sort_unit_stack( ss, res );
  bool const has_last_unit_input =
      last_unit_input.has_value() &&
      last_unit_input_is_in_stack_indirect( ss, *last_unit_input,
                                            res );
  // This is optional, but always put the most recent unit to ask
  // for orders at the top of the stack if they are in this tile.
  // This makes for a better UX since e.g. if a unit is on a
  // square with other units and it attempts to make an invalid
  // move, it will remain on top while the message box pops up
  // with the explanation.
  if( has_last_unit_input ) {
    erase( res, *last_unit_input );
    res.insert( res.begin(), *last_unit_input );
  }
  return res;
}

// `multiple_units` is for the case when there are multiple units
// on the square and we want to indicate that visually.
void LandViewRenderer::render_single_unit(
    Coord where, GenericUnitId id,
    e_flag_count flag_count ) const {
  switch( ss_.units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      UnitId const unit_id{ to_underlying( id ) };
      render_unit( renderer_, where,
                   ss_.units.unit_for( unit_id ),
                   UnitRenderOptions{ .flag   = flag_count,
                                      .shadow = UnitShadow{} } );
      break;
    }
    case e_unit_kind::native: {
      NativeUnitId const unit_id{ to_underlying( id ) };
      render_native_unit(
          renderer_, where, ss_, ss_.units.unit_for( unit_id ),
          UnitRenderOptions{ .flag   = flag_count,
                             .shadow = UnitShadow{} } );
      break;
    }
  }
}

void LandViewRenderer::render_units_on_square(
    Coord tile, bool flags ) const {
  if( viz_.visible( tile ) !=
      e_tile_visibility::visible_and_clear )
    return;
  // This will be sorted in decreasing order of defense, then by
  // decreasing id.
  vector<GenericUnitId> sorted =
      land_view_unit_stack( ss_, tile, last_unit_input_ );
  // We shouldn't draw units that are being animated because they
  // will be drawn on top separately. If we did draw them here
  // then it would cause problems for e.g. a unit that is doing a
  // blinking animation; when the blink is in the "off" state the
  // unit would still be visible.
  erase_if( sorted, [this]( GenericUnitId id ) {
    return lv_animator_.unit_animations().contains( id );
  } );
  if( sorted.empty() ) return;
  GenericUnitId const max_defense = sorted[0];

  Coord const where = render_rect_for_tile( tile ).upper_left();
  bool const  multiple_units    = ( sorted.size() > 1 );
  e_flag_count const flag_count = !flags ? e_flag_count::none
                                  : !multiple_units
                                      ? e_flag_count::single
                                      : e_flag_count::multiple;
  render_single_unit( where, max_defense, flag_count );
}

vector<pair<Coord, GenericUnitId>>
LandViewRenderer::units_to_render() const {
  // This is for efficiency. When we are sufficiently zoomed out
  // then it is more efficient to iterate over units then covered
  // tiles, whereas the reverse is true when zoomed in.
  unordered_map<GenericUnitId, UnitState> const& all =
      ss_.units.all();
  int const                          num_units = all.size();
  int const                          num_tiles = covered_.area();
  vector<pair<Coord, GenericUnitId>> res;
  res.reserve( num_units );
  if( num_tiles > num_units ) {
    // Iterate over units.
    for( auto const& [id, state] : all )
      if( maybe<Coord> coord =
              coord_for_unit_indirect( ss_.units, id );
          coord.has_value() && coord->is_inside( covered_ ) )
        res.emplace_back( *coord, id );
  } else {
    // Iterate over covered tiles.
    for( Rect tile : gfx::subrects( covered_ ) )
      for( GenericUnitId generic_id :
           ss_.units.from_coord( tile.upper_left() ) )
        res.emplace_back( tile.upper_left(), generic_id );
  }
  return res;
}

void LandViewRenderer::render_units_default() const {
  unordered_set<Coord> hit;
  for( auto [tile, id] : units_to_render() ) {
    if( ss_.colonies.maybe_from_coord( tile ).has_value() )
      continue;
    if( hit.contains( tile ) ) continue;
    render_units_on_square( tile, /*flags=*/true );
    hit.insert( tile );
  }
}

// `multiple_units` is for the case when there are multiple units
// on the square and we want to indicate that visually.
void LandViewRenderer::render_single_unit_depixelate_to(
    Coord where, GenericUnitId id, bool multiple_units,
    double stage, e_tile target_tile ) const {
  e_flag_count const flag_style = multiple_units
                                      ? e_flag_count::multiple
                                      : e_flag_count::single;
  switch( ss_.units.unit_kind( id ) ) {
    case e_unit_kind::euro:
      render_unit_depixelate_to(
          renderer_, where, ss_, ss_.units.euro_unit_for( id ),
          target_tile, stage,
          UnitRenderOptions{ .flag   = flag_style,
                             .shadow = UnitShadow{} } );
      break;
    case e_unit_kind::native:
      render_native_unit_depixelate_to(
          renderer_, where, ss_, ss_.units.native_unit_for( id ),
          target_tile, stage,
          UnitRenderOptions{ .flag   = flag_style,
                             .shadow = UnitShadow{} } );
      break;
  }
}

void LandViewRenderer::render_units_impl() const {
  if( lv_animator_.unit_animations().empty() )
    return render_units_default();
  // We have some units being animated, so now things get compli-
  // cated. This will be the case most of the time, since there
  // is usually at least one unit blinking. The exception would
  // be the end-of-turn when there should be no animations.

  unordered_map<GenericUnitId, UnitAnimationState::front const*>
      front;
  unordered_map<GenericUnitId, UnitAnimationState::blink const*>
      blink;
  unordered_map<GenericUnitId, UnitAnimationState::slide const*>
      slide;
  unordered_map<GenericUnitId,
                UnitAnimationState::depixelate_unit const*>
      depixelate_unit;
  unordered_map<GenericUnitId,
                UnitAnimationState::enpixelate_unit const*>
      enpixelate_unit;
  // These are the tiles to skip when rendering units that are
  // not animated. An example would be that if a unit is blinking
  // then we don't want to render any other units on that tile.
  unordered_set<Coord> tiles_to_skip;
  // These are tiles where we want to draw units but faded and
  // with no flags so that the unit in front will be more dis-
  // cernible but the player will still see that there are units
  // behind.
  unordered_set<Coord> tiles_to_fade;
  for( auto const& [id, anim_stack] :
       lv_animator_.unit_animations() ) {
    CHECK( !anim_stack.empty() );
    UnitAnimationState const& anim = anim_stack.top();
    Coord const               tile =
        coord_for_unit_multi_ownership_or_die( ss_, id );
    switch( anim.to_enum() ) {
      case UnitAnimationState::e::front:
        front[id] = &anim.get<UnitAnimationState::front>();
        tiles_to_skip.insert( tile );
        break;
      case UnitAnimationState::e::hide:
        // Nothing to do here. These units with the "hide" anima-
        // tion will never be rendered because 1) they are con-
        // sidered to be "animated" and so will not be rendered
        // in the unit stack, and 2) we are not explicitly
        // drawing them in this function.
        break;
      case UnitAnimationState::e::blink:
        blink[id] = &anim.get<UnitAnimationState::blink>();
        tiles_to_skip.insert( tile );
        break;
      case UnitAnimationState::e::slide:
        slide[id] = &anim.get<UnitAnimationState::slide>();
        tiles_to_fade.insert( tile );
        break;
      case UnitAnimationState::e::depixelate_unit:
        depixelate_unit[id] =
            &anim.get<UnitAnimationState::depixelate_unit>();
        tiles_to_fade.insert( tile );
        break;
      case UnitAnimationState::e::enpixelate_unit:
        enpixelate_unit[id] =
            &anim.get<UnitAnimationState::enpixelate_unit>();
        tiles_to_fade.insert( tile );
        break;
    }
  }

  // Render all non-animated units except for those on tiles that
  // we want to skip and those explicitly hidden (actually "hid-
  // den" is a form of animation anyway).
  unordered_set<Coord> hit;
  for( auto [tile, id] : units_to_render() ) {
    if( tiles_to_skip.contains( tile ) ) continue;
    if( ss_.colonies.maybe_from_coord( tile ).has_value() )
      continue;
    if( lv_animator_.unit_animations().contains( id ) ) continue;
    if( hit.contains( tile ) ) continue;
    hit.insert( tile );
    if( tiles_to_fade.contains( tile ) ) {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .25 );
      render_units_on_square( tile, /*flags=*/false );
    } else {
      render_units_on_square( tile, /*flags=*/true );
    }
  }

  auto render_impl = [&]( GenericUnitId id, auto const& f ) {
    Coord const tile =
        coord_for_unit_multi_ownership_or_die( ss_, id );
    // This will suppress the slide animation if the unit is
    // coming from a fully hidden square, so that the unit will
    // just appear on the destination square, as opposed to first
    // appearing on top of a non-visible square and sliding off
    // of it.
    if( viz_.visible( tile ) == e_tile_visibility::hidden )
      return;
    Coord const where =
        render_rect_for_tile( tile ).upper_left();
    bool const multiple_units =
        ss_.units.from_coord( tile ).size() > 1;
    e_flag_count const flag_count = multiple_units
                                        ? e_flag_count::multiple
                                        : e_flag_count::single;
    f( where, flag_count );
  };

  // 1. Render units that are supposed to hover above a colony.
  for( auto const& [id, anim] : front ) {
    render_impl( id,
                 [&]( Coord where, e_flag_count flag_count ) {
                   render_single_unit( where, id, flag_count );
                 } );
  }

  // 2. Render units that are blinking.
  for( auto const& [id, anim] : blink ) {
    if( !anim->visible ) continue;
    render_impl( id, [&]( Coord where, e_flag_count ) {
      render_single_unit( where, id, e_flag_count::single );
    } );
  }

  // 3. Render units that are sliding.
  for( auto const& [id, anim] : slide ) {
    Coord const mover_coord =
        coord_for_unit_indirect_or_die( ss_.units, id );
    // Now render the sliding unit.
    Delta const pixel_delta =
        ( ( mover_coord.moved( anim->direction ) -
            mover_coord ) *
          g_tile_delta )
            .multiply_and_round( anim->percent );
    render_impl( id, [&]( Coord where, e_flag_count ) {
      render_single_unit( where + pixel_delta, id,
                          e_flag_count::single );
    } );
  }

  // 4. Render units that are depixelating.
  for( auto const& [id, anim] : depixelate_unit ) {
    // Check if we are depixelating to another unit.
    if( !anim->target.has_value() ) {
      Coord const tile =
          coord_for_unit_multi_ownership_or_die( ss_, id );
      Coord const loc =
          render_rect_for_tile( tile ).upper_left();
      // Render and depixelate both the unit and the flag.
      SCOPED_RENDERER_MOD_SET(
          painter_mods.depixelate.hash_anchor, loc );
      SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                               anim->stage );
      render_impl( id, [&]( Coord where, e_flag_count ) {
        render_single_unit( where, id, e_flag_count::single );
      } );
    } else {
      CHECK( anim->target.has_value() );
      // Render and the unit and the flag but only depixelate the
      // unit to the target unit. This function will set the hash
      // anchor and stage ultimately.
      render_impl( id, [&]( Coord where, e_flag_count ) {
        render_single_unit_depixelate_to(
            where, id, /*multiple_units=*/false, anim->stage,
            *anim->target );
      } );
    }
  }

  // 5. Render units that are enpixelating.
  for( auto const& [id, anim] : enpixelate_unit ) {
    render_impl( id, [&]( Coord where, e_flag_count ) {
      SCOPED_RENDERER_MOD_SET(
          painter_mods.depixelate.hash_anchor, where );
      SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                               anim->stage );
      render_single_unit( where, id, e_flag_count::none );
    } );
  }
}

void LandViewRenderer::render_native_dwelling(
    Dwelling const& dwelling ) const {
  Coord const location = ss_.natives.coord_for( dwelling.id );
  if( viz_.visible( location ) == e_tile_visibility::hidden )
    return;
  Coord const tile_coord =
      render_rect_for_tile( location ).upper_left() -
      Delta{ .w = 6, .h = 6 };

  if( viz_.visible( location ) ==
      e_tile_visibility::visible_and_clear ) {
    render_real_dwelling( renderer_, tile_coord, ss_, dwelling );
    return;
  }

  // The tile is explored but fogged.
  UNWRAP_CHECK( fog_square, viz_.fog_square_at( location ) );
  if( fog_square.dwelling.has_value() ) {
    render_fog_dwelling( renderer_, tile_coord,
                         *fog_square.dwelling );
    return;
  }

  // The player has apparently visited this tile but is not aware
  // of the dwelling on it. This should never happen in an (un-
  // modded) game because dwellings can't spawn or respawn after
  // map generation under any circumstances. TODO: consider re-
  // moving this check-fail in the future to allow for mods that
  // might cause dwellings to spawn.
  SHOULD_NOT_BE_HERE;
}

void LandViewRenderer::render_native_dwelling_depixelate(
    Dwelling const& dwelling ) const {
  Coord const location = ss_.natives.coord_for( dwelling.id );
  UNWRAP_CHECK(
      animation,
      lv_animator_.dwelling_animation( dwelling.id )
          .get_if<DwellingAnimationState::depixelate>() );
  // As usual, the hash anchor coord is arbitrary so long as
  // its position is fixed relative to the sprite.
  Coord const hash_anchor =
      render_rect_for_tile( location ).upper_left();
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           animation.stage );
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                           hash_anchor );
  render_native_dwelling( dwelling );
}

// When the player is moving a unit and it runs out of movement
// points there is a chance that the player will accidentally
// issue a couple of extra input commands to the unit beyond the
// end of its turn. If that happens then the very next unit to
// ask for orders would get those commands and move in a way that
// the player likely had not intended. So we have a mechanism
// (logic elsewhere) of preventing that, and the visual indicator
// is an hourglass temporarily drawn on the new unit let the
// player know that a few input commands were thrown out in order
// to avoid inadvertantly giving them to the new unit.
//
// TODO: rendering this is technically optional. Decide whether
// to keep it. If this ends up being removed then the config op-
// tions referenced can also be removed.
void LandViewRenderer::render_input_overrun_indicator() const {
  if( !input_overrun_indicator_.has_value() ) return;
  InputOverrunIndicator const& indicator =
      *input_overrun_indicator_;
  maybe<Coord> const unit_coord =
      coord_for_unit_indirect( ss_.units, indicator.unit_id );
  if( !unit_coord.has_value() ) return;
  if( !unit_coord->is_inside( covered_.with_border_added() ) )
    return;
  Rect const indicator_render_rect =
      render_rect_for_tile( *unit_coord );
  auto const kHoldTime = config_land_view.input_overrun_detection
                             .hourglass_hold_time;
  auto const kFadeTime = config_land_view.input_overrun_detection
                             .hourglass_fade_time;
  double     alpha = 1.0;
  auto const delta = Clock_t::now() - indicator.start_time;
  if( delta > kHoldTime ) {
    auto fade_time = clamp(
        duration_cast<chrono::milliseconds>( delta - kHoldTime ),
        0ms, kFadeTime );
    alpha = double( fade_time.count() ) / kFadeTime.count();
    alpha = 1.0 - alpha;
  }
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
  rr::Painter painter = renderer.painter();
  render_sprite( painter, indicator_render_rect.upper_left(),
                 e_tile::lift_key );
}

void LandViewRenderer::render_colony(
    Colony const& colony ) const {
  if( viz_.visible( colony.location ) ==
      e_tile_visibility::hidden )
    return;
  Coord const tile_coord =
      render_rect_for_tile( colony.location ).upper_left();
  Coord const colony_sprite_upper_left =
      tile_coord - Delta{ .w = 6, .h = 6 };
  ColonyRenderOptions const options{ .render_name       = true,
                                     .render_population = true,
                                     .render_flag       = true };
  if( viz_.visible( colony.location ) ==
      e_tile_visibility::visible_and_clear ) {
    render_real_colony( renderer_, colony_sprite_upper_left, ss_,
                        colony, options );
    return;
  }

  // The tile is explored but fogged.
  UNWRAP_CHECK( fog_square,
                viz_.fog_square_at( colony.location ) );
  if( fog_square.colony.has_value() ) {
    render_fog_colony( renderer_, colony_sprite_upper_left,
                       *fog_square.colony, options );
    return;
  }

  // Do nothing since the player is not aware of the colony here,
  // probably because they explored this tile before it was
  // founded.
}

void LandViewRenderer::render_colony_depixelate(
    Colony const& colony ) const {
  UNWRAP_CHECK(
      animation,
      lv_animator_.colony_animation( colony.id )
          .get_if<ColonyAnimationState::depixelate>() );
  // As usual, the hash anchor coord is arbitrary so long as
  // its position is fixed relative to the sprite.
  Coord const hash_anchor =
      render_rect_for_tile( colony.location ).upper_left();
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           animation.stage );
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                           hash_anchor );
  this->render_colony( colony );
}

// This will render the background around the zoomed-out map.
// This background consists of some giant stretched ocean tiles.
// It goes like this:
//
//   1. The tiles are scaled up, but only as large as possible so
//      that they can remain as squares; so the tile size will be
//      equal to the shorter side length of the viewport.
//   2. Those tiles are then tiled to cover all of the area.
//   3. Steps 1+2 are repeated two more times with partial alpha
//      (i.e., layered on top of the previous), but each time
//      being scaled up slightly more. The scaling is done about
//      the center of the composite image in order create a
//      "zooming" effect. To achieve this, the composite (total,
//      tiled) image is rendered around the origin and the GPU
//      then scales it and then translates it.
//
// As mentioned, all of the layers are done with partial alpha so
// that they all end up visible and thus create a "zooming" ef-
// fect.
void LandViewRenderer::render_backdrop() const {
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, 0.4 );
  auto const [shortest_side, longest_side] = [&] {
    Delta const delta         = viewport_rect_pixels_.delta();
    int         shortest_side = std::min( delta.w, delta.h );
    int         longest_side  = std::max( delta.w, delta.h );
    return pair{ shortest_side, longest_side };
  }();
  int const num_squares_needed =
      longest_side / shortest_side + 1;
  Delta const tile_size =
      Delta{ .w = W{ shortest_side }, .h = H{ shortest_side } };
  Rect const tiled_rect =
      Rect::from( Coord{},
                  tile_size* Delta{ .w = num_squares_needed,
                                    .h = num_squares_needed } )
          .centered_on( Coord{} );
  Delta const shift = viewport_rect_pixels_.center() -
                      viewport_rect_pixels_.upper_left();
  double       scale      = 1.00;
  double const kScaleInc  = .014;
  int const    kNumLayers = 4;
  for( int i = 0; i < kNumLayers; ++i ) {
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, scale );
    SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                             gfx::size( shift ).to_double() );
    rr::Painter painter = renderer_.painter();
    for( Rect rect : gfx::subrects( tiled_rect, tile_size ) )
      render_sprite( painter,
                     Rect::from( rect.upper_left(), tile_size ),
                     e_tile::terrain_ocean );
    scale += kScaleInc;
  }
}

void LandViewRenderer::render_units() const {
  render_units_impl();
  // Do any post rendering steps that must be done after units
  // rendering.
  render_input_overrun_indicator();
}

void LandViewRenderer::render_native_dwellings() const {
  unordered_map<DwellingId, DwellingState> const& all =
      ss_.natives.dwellings_all();
  // FIXME: this needs to just iterate over those in the covered
  // rect for efficiency.
  for( auto const& [id, state] : all ) {
    if( !state.ownership.location.is_inside( covered_ ) )
      continue;
    maybe<DwellingAnimationState const&> anim =
        lv_animator_.dwelling_animation( id );
    if( !anim.has_value() )
      render_native_dwelling( state.dwelling );
    else
      render_native_dwelling_depixelate( state.dwelling );
  }
}

// Render units under colonies or dwellings (used when abandoning
// a colony leaving a colonist or destroying a dwelling leaving a
// missionary.
void LandViewRenderer::render_units_underneath() const {
  // Under colonies.
  for( auto const& [colony_id, anim_stack] :
       lv_animator_.colony_animations() ) {
    CHECK( !anim_stack.empty() );
    ColonyAnimationState const& anim = anim_stack.top();
    switch( anim.to_enum() ) {
      case ColonyAnimationState::e::depixelate: {
        Coord const location =
            ss_.colonies.colony_for( colony_id ).location;
        if( !location.is_inside( covered_ ) ) return;
        render_units_on_square( location, /*flags=*/false );
        break;
      }
    }
  }

  // Under dwellings.
  for( auto const& [dwelling_id, anim_stack] :
       lv_animator_.dwelling_animations() ) {
    CHECK( !anim_stack.empty() );
    DwellingAnimationState const& anim = anim_stack.top();
    switch( anim.to_enum() ) {
      case DwellingAnimationState::e::depixelate: {
        Coord const location =
            ss_.natives.coord_for( dwelling_id );
        if( !location.is_inside( covered_ ) ) return;
        render_units_on_square( location, /*flags=*/false );
        break;
      }
    }
  }
}

void LandViewRenderer::render_colonies() const {
  // FIXME: since colony icons spill over the usual 32x32 tile
  // we need to render colonies that are beyond the `covered`
  // rect.
  unordered_map<ColonyId, Colony> const& all =
      ss_.colonies.all();
  for( auto const& [id, colony] : all ) {
    if( !colony.location.is_inside( covered_ ) ) continue;
    maybe<ColonyAnimationState const&> anim =
        lv_animator_.colony_animation( id );
    if( !anim.has_value() )
      this->render_colony( colony );
    else {
      switch( anim->to_enum() ) {
        case ColonyAnimationState::e::depixelate:
          render_colony_depixelate( colony );
          break;
      }
    }
  }
}

/****************************************************************
** Public API
*****************************************************************/
LandViewRenderer::LandViewRenderer(
    SSConst const& ss, rr::Renderer& renderer_arg,
    LandViewAnimator const& lv_animator, Visibility const& viz,
    maybe<UnitId> last_unit_input, Rect viewport_rect_pixels,
    maybe<InputOverrunIndicator> input_overrun_indicator,
    SmoothViewport const&        viewport )
  : ss_( ss ),
    renderer_( renderer_arg ),
    renderer( renderer_arg ),
    lv_animator_( lv_animator ),
    covered_( viewport.covered_tiles() ),
    viz_( viz ),
    last_unit_input_( last_unit_input ),
    viewport_rect_pixels_( viewport_rect_pixels ),
    input_overrun_indicator_( input_overrun_indicator ),
    viewport_( viewport ) {
  // Some of the actions in this class would crash if the last
  // unit input corresponds to a unit ID that no longer exists.
  // Since this class does not care about the last unit id in
  // that situation anyway, we will nullify it in that case.
  if( last_unit_input_.has_value() &&
      !ss_.units.exists( *last_unit_input_ ) )
    last_unit_input_ = nothing;
}

void LandViewRenderer::render_entities() const {
  SCOPED_RENDERER_MOD_SET( buffer_mods.buffer,
                           rr::e_render_buffer::entities );
  // Move the rendering start slightly off screen (in the
  // upper-left direction) by an amount that is within the span
  // of one tile to partially show that tile row/column.
  gfx::dpoint const corner =
      viewport_.rendering_dest_rect().origin -
      viewport_.covered_pixels().origin.fmod( 32.0 ) *
          viewport_.get_zoom();

  // The below render_* functions will always render at normal
  // scale and starting at 0,0 on the screen, and then the ren-
  // derer mods that we've install above will automatically do
  // the shifting and scaling.
  SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale,
                           viewport_.get_zoom() );
  SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                           corner.distance_from_origin() );
  render_units_underneath();
  render_native_dwellings();
  render_colonies();
  render_units();
}

void LandViewRenderer::render_non_entities() const {
  // If the map is zoomed out enough such that some of the outter
  // space is visible, paint a background so that it won't just
  // have empty black surroundings.
  if( viewport_.are_surroundings_visible() ) {
    SCOPED_RENDERER_MOD_SET( buffer_mods.buffer,
                             rr::e_render_buffer::backdrop );
    render_backdrop();

    {
      // This is the shadow behind the land rectangle.
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, 0.5 );
      double const zoom          = viewport_.get_zoom();
      int          shadow_offset = 6;
      gfx::dpoint  corner =
          viewport_.landscape_buffer_render_upper_left();
      corner.x += shadow_offset;
      corner.y += shadow_offset;
      Rect const shadow_rect{
          .x = int( corner.x ),
          .y = int( corner.y ),
          .w = int( viewport_.world_size_pixels().w * zoom ),
          .h = int( viewport_.world_size_pixels().h * zoom ),
      };
      rr::Painter painter = renderer.painter();
      painter.draw_solid_rect(
          shadow_rect, gfx::pixel::black().with_alpha( 100 ) );
    }
  }

  // Set camera.
  double const      zoom = viewport_.get_zoom();
  gfx::dpoint const translation =
      viewport_.landscape_buffer_render_upper_left();
  renderer.set_camera( translation.distance_from_origin(),
                       zoom );
}

} // namespace rn
