/****************************************************************
**colview-land.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: Land view UI within the colony view.
*
*****************************************************************/
#include "colview-land.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "map-square.hpp"
#include "native-owned.hpp"
#include "production.rds.hpp"
#include "render-terrain.hpp"
#include "render.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"
#include "visibility.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/iter.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

e_tile tile_for_outdoor_job_20( e_outdoor_job const job ) {
  switch( job ) {
    case e_outdoor_job::food:
      return e_tile::commodity_food_20;
    case e_outdoor_job::fish:
      return e_tile::product_fish_20;
    case e_outdoor_job::sugar:
      return e_tile::commodity_sugar_20;
    case e_outdoor_job::tobacco:
      return e_tile::commodity_tobacco_20;
    case e_outdoor_job::cotton:
      return e_tile::commodity_cotton_20;
    case e_outdoor_job::furs:
      return e_tile::commodity_furs_20;
    case e_outdoor_job::lumber:
      return e_tile::commodity_lumber_20;
    case e_outdoor_job::ore:
      return e_tile::commodity_ore_20;
    case e_outdoor_job::silver:
      return e_tile::commodity_silver_20;
  }
}

void render_glow( rr::Renderer& renderer, Coord unit_coord,
                  e_unit_type type ) {
  UnitTypeAttributes const& desc = unit_attr( type );
  e_tile const tile              = desc.tile;
  render_sprite_silhouette(
      renderer, unit_coord + Delta{ .w = 1 }, tile,
      config_colony.colors.outdoor_unit_glow_color );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Delta ColonyLandView::size_needed( e_render_mode mode ) {
  int side_length_in_squares = 3;
  switch( mode ) {
    case e_render_mode::_3x3:
      side_length_in_squares = 3;
      break;
    case e_render_mode::_5x5:
      side_length_in_squares = 5;
      break;
    case e_render_mode::_6x6:
      side_length_in_squares = 6;
      break;
  }
  return Delta{ .w = 32, .h = 32 } *
         Delta{ .w = side_length_in_squares,
                .h = side_length_in_squares };
}

maybe<e_direction> ColonyLandView::direction_under_cursor(
    Coord coord ) const {
  switch( mode_ ) {
    case e_render_mode::_3x3:
      return Coord{ .x = 1, .y = 1 }.direction_to(
          coord / g_tile_delta );
    case e_render_mode::_5x5: {
      // TODO: this will probably have to be made more sophis-
      // ticated.
      Coord shifted = coord - g_tile_delta;
      if( shifted.x < 0 || shifted.y < 0 ) return nothing;
      return Coord{ .x = 1, .y = 1 }.direction_to(
          shifted / g_tile_delta );
    }
    case e_render_mode::_6x6:
      return Coord{ .x = 1, .y = 1 }.direction_to(
          coord / ( g_tile_delta * Delta{ .w = 2, .h = 2 } ) );
  }
}

Rect ColonyLandView::rect_for_unit( e_direction d ) const {
  switch( mode_ ) {
    case e_render_mode::_3x3:
      return Rect::from(
          Coord{ .x = 1, .y = 1 }.moved( d ) * g_tile_delta,
          g_tile_delta );
    case e_render_mode::_5x5: {
      NOT_IMPLEMENTED;
    }
    case e_render_mode::_6x6:
      return Rect::from(
          Coord{ .x = 1, .y = 1 }.moved( d ) * g_tile_delta *
                  Delta{ .w = 2, .h = 2 } +
              ( g_tile_delta / Delta{ .w = 2, .h = 2 } ),
          g_tile_delta );
  }
}

maybe<UnitId> ColonyLandView::unit_for_direction(
    e_direction d ) const {
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  return colony.outdoor_jobs[d].member( &OutdoorUnit::unit_id );
}

maybe<e_outdoor_job> ColonyLandView::job_for_direction(
    e_direction d ) const {
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  return colony.outdoor_jobs[d].member( &OutdoorUnit::job );
}

maybe<UnitId> ColonyLandView::unit_under_cursor(
    Coord where ) const {
  UNWRAP_RETURN( d, direction_under_cursor( where ) );
  return unit_for_direction( d );
}

Delta ColonyLandView::delta() const {
  return size_needed( mode_ );
}

maybe<int> ColonyLandView::entity() const {
  return static_cast<int>( e_colview_entity::land );
}

ui::View& ColonyLandView::view() noexcept { return *this; }

ui::View const& ColonyLandView::view() const noexcept {
  return *this;
}

wait<> ColonyLandView::perform_click(
    input::mouse_button_event_t const& event ) {
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitId> unit_id = unit_under_cursor( event.pos );
  if( !unit_id.has_value() ) co_return;

  EnumChoiceConfig const config{ .msg = "Select Occupation" };
  maybe<e_outdoor_job> const new_job =
      co_await ts_.gui.optional_enum_choice(
          config, config_colony.outdoors.job_names );
  if( !new_job.has_value() ) co_return;
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  change_unit_outdoor_job( colony, *unit_id, *new_job );
  update_production( ss_, colony_ );
}

maybe<CanReceiveDraggable<ColViewObject>>
ColonyLandView::can_receive( ColViewObject const& o, int,
                             Coord const& where ) const {
  // Verify that the dragged object is a unit.
  maybe<UnitId> unit_id = o.get_if<ColViewObject::unit>().member(
      &ColViewObject::unit::id );
  if( !unit_id.has_value() ) return nothing;
  // Check if the unit is a colonist.
  Unit const& unit = ss_.units.unit_for( *unit_id );
  if( !unit.is_colonist() ) return nothing;
  // Check if there is a land square under the cursor that is
  // not the center.
  maybe<e_direction> d = direction_under_cursor( where );
  if( !d.has_value() ) return nothing;
  Coord const world_square = colony_.location.moved( *d );
  // Check if this is the same unit currently being dragged, if
  // so we'll allow it.
  if( dragging_.has_value() && d == dragging_->d )
    return CanReceiveDraggable<ColViewObject>::yes{ .draggable =
                                                        o };
  // Check if there is already a unit on the square.
  if( unit_under_cursor( where ).has_value() ) return nothing;
  // Check if there is a colonist from another colony (friendly
  // or foreign) that is already working on this square.
  if( occupied_red_box_[*d] ) return nothing;
  // Check if there is a native dwelling on the tile.
  if( ss_.natives.maybe_dwelling_from_coord( world_square )
          .has_value() )
    return nothing;
  // Note that we don't check for water/docks here; that is
  // done in the check function.

  // We're good to go.
  return CanReceiveDraggable<ColViewObject>::yes{ .draggable =
                                                      o };
}

wait<base::valid_or<DragRejection>> ColonyLandView::sink_check(
    ColViewObject const&, int, Coord const where ) {
  Colony const& colony = ss_.colonies.colony_for( colony_.id );
  UNWRAP_CHECK( d, direction_under_cursor( where ) );
  Coord const tile_under_cursor = colony.location.moved( d );
  if( !ss_.terrain.square_exists( tile_under_cursor ) )
    co_return DragRejection{
      .reason =
          "This tile is beyond the bounds of the world map." };
  MapSquare const& square =
      ss_.terrain.square_at( tile_under_cursor );

  if( native_owned_land_[d] ) {
    bool const has_taken =
        co_await prompt_player_for_taking_native_land(
            ss_, ts_, player_, tile_under_cursor,
            e_native_land_grab_type::in_colony );
    if( !has_taken ) co_return DragRejection{};
    native_owned_land_[d] = nothing;
  }

  if( is_water( square ) &&
      !colony_has_building_level( colony,
                                  e_colony_building::docks ) ) {
    co_return DragRejection{
      .reason =
          "We must build [docks] in this colony in "
          "order to work on sea squares." };
  }

  if( square.lost_city_rumor ) {
    co_return DragRejection{
      .reason =
          "We must explore this Lost City Rumor before we "
          "can work this square." };
  }

  co_return base::valid;
}

ColonyJob ColonyLandView::make_job_for_square(
    e_direction d ) const {
  // The original game seems to always choose food.
  return ColonyJob::outdoor{ .direction = d,
                             .job       = e_outdoor_job::food };
}

wait<> ColonyLandView::drop( ColViewObject const& o,
                             Coord const& where ) {
  UNWRAP_CHECK( unit_id, o.get_if<ColViewObject::unit>().member(
                             &ColViewObject::unit::id ) );
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  UNWRAP_CHECK( d, direction_under_cursor( where ) );
  ColonyJob job = make_job_for_square( d );
  if( dragging_.has_value() ) {
    // The unit being dragged is coming from another square on
    // the land view, so keep its job the same.
    job = ColonyJob::outdoor{ .direction = d,
                              .job       = dragging_->job };
  }
  UnitOwnershipChanger( ss_, unit_id )
      .change_to_colony( ts_, colony, job );
  CHECK_HAS_VALUE( colony.validate() );
  co_return;
}

maybe<DraggableObjectWithBounds<ColViewObject>>
ColonyLandView::object_here( Coord const& where ) const {
  UNWRAP_RETURN( unit_id, unit_under_cursor( where ) );
  UNWRAP_RETURN( d, direction_under_cursor( where ) );
  return DraggableObjectWithBounds<ColViewObject>{
    .obj    = ColViewObject::unit{ .id = unit_id },
    .bounds = rect_for_unit( d ) };
}

bool ColonyLandView::try_drag( ColViewObject const&,
                               Coord const& where ) {
  UNWRAP_CHECK( d, direction_under_cursor( where ) );
  UNWRAP_CHECK( job, job_for_direction( d ) );
  dragging_ = Draggable{ .d = d, .job = job };
  return true;
}

void ColonyLandView::cancel_drag() { dragging_ = nothing; }

wait<> ColonyLandView::disown_dragged_object() {
  UNWRAP_CHECK( draggable, dragging_ );
  UNWRAP_CHECK( unit_id, unit_for_direction( draggable.d ) );
  UnitOwnershipChanger( ss_, unit_id ).change_to_free();
  co_return;
}

void ColonyLandView::draw_land_3x3( rr::Renderer& renderer,
                                    Coord const coord ) const {
  point const mouse_pos =
      input::current_mouse_position().to_gfx();
  bool const hover = mouse_pos.is_inside( bounds( coord ) );

  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      gfx::size( coord.distance_from_origin() ).to_double() );

  // This alpha is to fade the land tiles behind the units so
  // as to make the units more visible. Not sure yet if we want
  // to do that.
  double const alpha = hover ? 0.0 : 0.3;

  // FIXME: Should not be duplicating land-view rendering code
  // here.
  rr::Painter painter       = renderer.painter();
  Coord const colony_square = colony_.location;
  VisibilityForNation const viz( ss_, player_.nation );
  // Render terrain.
  for( Rect const local_rect : gfx::subrects(
           Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) ) {
    Coord const local_coord = local_rect.upper_left();
    Coord world_square      = colony_square +
                         local_coord.distance_from_origin() -
                         Delta{ .w = 1, .h = 1 };
    render_terrain_square_merged(
        renderer, local_coord * g_tile_delta, world_square, viz,
        TerrainRenderOptions{} );
    static Coord const local_colony_loc =
        Coord{ .x = 1, .y = 1 };
    if( local_coord == local_colony_loc ) continue;
    UNWRAP_CHECK( d,
                  local_colony_loc.direction_to( local_coord ) );
    if( occupied_red_box_[d] )
      // This square is occupied by a colonist from another
      // colony (either friendly or foreign).
      painter.draw_empty_rect(
          Rect::from( local_coord * g_tile_delta, g_tile_delta ),
          rr::Painter::e_border_mode::inside, pixel::red() );
  }

  // Render colonies.
  for( Rect const local_rect : gfx::subrects(
           Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) ) {
    Coord const local_coord = local_rect.upper_left();
    auto world_square       = colony_square +
                        local_coord.distance_from_origin() -
                        Delta{ .w = 1, .h = 1 };
    auto maybe_col_id =
        ss_.colonies.maybe_from_coord( world_square );
    if( !maybe_col_id ) continue;
    render_colony(
        renderer,
        local_coord * g_tile_delta - Delta{ .w = 6, .h = 6 },
        ss_, ss_.colonies.colony_for( *maybe_col_id ),
        ColonyRenderOptions{ .render_name       = false,
                             .render_population = false,
                             .render_flag       = true } );
  }

  // Render native dwellings.
  for( Rect const local_rect : gfx::subrects(
           Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) ) {
    Coord const local_coord = local_rect.upper_left();
    auto world_square       = colony_square +
                        local_coord.distance_from_origin() -
                        Delta{ .w = 1, .h = 1 };
    // We always render the real thing when directly visible by a
    // unit or colony, which it is in this case since it is next
    // to one of our colonies.
    auto maybe_dwelling_id =
        ss_.natives.maybe_dwelling_from_coord( world_square );
    if( !maybe_dwelling_id ) continue;
    render_dwelling(
        renderer,
        local_coord * g_tile_delta - Delta{ .w = 6, .h = 6 },
        ss_, ss_.natives.dwelling_for( *maybe_dwelling_id ) );
  }

  // Render native-owned land markers (totem poles).
  for( Rect const local_rect : gfx::subrects(
           Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) ) {
    Coord const local_coord = local_rect.upper_left();
    static Coord const local_colony_loc =
        Coord{ .x = 1, .y = 1 };
    auto world_square = colony_square +
                        local_coord.distance_from_origin() -
                        Delta{ .w = 1, .h = 1 };
    // We never show the totem pole over the colony square even
    // if it is owned land (which it might be, because the game
    // does allow the player to found colonies on native-owned
    // land without paying and without removing the native owner-
    // ship).
    if( local_coord == local_colony_loc ) continue;
    // Don't draw a totem pole over a native dwelling.
    if( ss_.natives.maybe_dwelling_from_coord( world_square )
            .has_value() )
      continue;
    UNWRAP_CHECK( d,
                  local_colony_loc.direction_to( local_coord ) );
    if( !native_owned_land_[d].has_value() ) continue;
    render_sprite( renderer, local_coord * g_tile_delta,
                   e_tile::totem_pole );
  }

  // This must be drawn after the terrain square instead of the
  // other way around because otherwise the tiles come out uneven
  // because some tiles render more layers than others, which
  // when alphas are accumulated, creates bad looking visuals, in
  // particular between tiles that have some land stenciled onto
  // them vs tiles that only have water in their cardinal direc-
  // tions (the former will render first a land tile then sten-
  // cil'd water, whereas the latter will render only water).
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
    renderer.painter().draw_solid_rect(
        rect{ .size = { .w = 32 * 3, .h = 32 * 3 } },
        pixel{ .r = 0, .g = 0, .b = 0, .a = 255 } );
  }
}

void ColonyLandView::draw_spread(
    rr::Renderer& renderer, rect const box, e_tile const tile,
    int const quantity, bool const is_colony_tile ) const {
  rect const inner_box = box.with_edges_removed( 8 );
  TileSpreadConfig const spread_config{
    .tile    = { .tile = tile, .count = quantity },
    .options = {
      .bounds = inner_box.size.w,
      .label_policy =
          ss_.settings.colony_options.numbers
              ? SpreadLabels{ SpreadLabels::always{} }
              : SpreadLabels{ SpreadLabels::auto_decide{} },
      .label_opts = SpreadLabelOptions{
        .color_bg = pixel::black(),
        .placement =
            SpreadLabelPlacement::left_middle_adjusted{},
      } } };
  if( quantity == 0 ) {
    auto const draw = [&]( e_tile const tile ) {
      size const sz = sprite_size( tile );
      point const origin =
          is_colony_tile
              ? gfx::centered_in( sz, inner_box )
              : gfx::centered_at_left( sz, inner_box );
      render_sprite( renderer, origin, tile );
    };
    draw( tile );
    draw( e_tile::red_prohibition_20 );
    return;
  }
  auto const plan =
      build_tile_spread( engine_.textometer(), spread_config );
  auto const spread_origin =
      gfx::centered_in( plan.bounds.size, inner_box );
  draw_rendered_icon_spread( renderer, spread_origin, plan );
}

void ColonyLandView::draw_land_6x6( rr::Renderer& renderer,
                                    Coord const coord ) const {
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, 2.0 );
    draw_land_3x3( renderer, coord );
  }
  // Further drawing should not be scaled.

  // Render units.
  Colony const& colony = ss_.colonies.colony_for( colony_.id );
  Coord const center   = Coord{ .x = 1, .y = 1 };

  for( auto const& [direction, outdoor_unit] :
       colony.outdoor_jobs ) {
    if( !outdoor_unit.has_value() ) continue;
    if( dragging_.has_value() && dragging_->d == direction )
      continue;
    point const square_coord =
        coord + ( center.moved( direction ) * g_tile_delta *
                  Delta{ .w = 2, .h = 2 } )
                    .distance_from_origin();
    e_outdoor_job const job   = outdoor_unit->job;
    e_tile const product_tile = tile_for_outdoor_job_20( job );
    SquareProduction const& production =
        colview_production().land_production[direction];
    int const quantity = production.quantity;
    rect const spread_box =
        rect{ .origin = square_coord + size{ .h = 11 },
              .size   = { .w = 64, .h = 20 } };
    draw_spread( renderer, spread_box, product_tile, quantity,
                 /*is_colony_tile=*/false );
    point const unit_coord =
        square_coord + size{ .w = 16, .h = 20 };
    UnitId const unit_id = outdoor_unit->unit_id;
    Unit const& unit     = ss_.units.unit_for( unit_id );
    UnitTypeAttributes const& desc = unit_attr( unit.type() );
    render_glow( renderer, unit_coord, unit.type() );
    render_unit_type(
        renderer, unit_coord, desc.type,
        UnitRenderOptions{ .shadow = UnitShadow{} } );
  }

  // Center square.
  point const square_coord =
      coord + ( center * g_tile_delta * Delta{ .w = 2, .h = 2 } )
                  .distance_from_origin();
  ColonyProduction const& production = colview_production();

  // food.
  {
    e_tile const product_tile =
        tile_for_outdoor_job_20( e_outdoor_job::food );
    int const quantity = production.center_food_production;
    rect const spread_box =
        rect{ .origin = square_coord + size{ .h = 11 },
              .size   = { .w = 64, .h = 20 } };
    draw_spread( renderer, spread_box, product_tile, quantity,
                 /*is_colony_tile=*/true );
  }

  // secondary.
  if( production.center_extra_production.has_value() ) {
    e_outdoor_job const job =
        production.center_extra_production->what;
    e_tile const product_tile = tile_for_outdoor_job_20( job );
    int const quantity =
        production.center_extra_production->quantity;
    rect const spread_box =
        rect{ .origin = square_coord + size{ .h = 32 + 2 },
              .size   = { .w = 64, .h = 20 } };
    draw_spread( renderer, spread_box, product_tile, quantity,
                 /*is_colony_tile=*/true );
  }
}

void ColonyLandView::draw( rr::Renderer& renderer,
                           Coord coord ) const {
  rr::Painter painter = renderer.painter();
  switch( mode_ ) {
    case e_render_mode::_3x3:
      draw_land_3x3( renderer, coord );
      break;
    case e_render_mode::_5x5:
      painter.draw_solid_rect( bounds( coord ), pixel::wood() );
      draw_land_3x3( renderer, coord + g_tile_delta );
      break;
    case e_render_mode::_6x6:
      draw_land_6x6( renderer, coord );
      break;
  }
}

unique_ptr<ColonyLandView> ColonyLandView::create(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    Colony& colony, e_render_mode mode ) {
  return make_unique<ColonyLandView>( engine, ss, ts, player,
                                      colony, mode );
}

ColonyLandView::ColonyLandView( IEngine& engine, SS& ss, TS& ts,
                                Player& player, Colony& colony,
                                e_render_mode mode )
  : ColonySubView( engine, ss, ts, player, colony ),
    mode_( mode ),
    occupied_red_box_(
        find_occupied_surrounding_colony_squares( ss, colony ) ),
    native_owned_land_( native_owned_land_around_square(
        ss, player, colony.location ) ) {}

} // namespace rn
