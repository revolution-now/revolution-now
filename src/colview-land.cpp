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
#include "gui.hpp"
#include "map-square.hpp"
#include "production.rds.hpp"
#include "render-terrain.hpp"
#include "render.hpp"
#include "text.hpp"
#include "tile-enum.rds.hpp"
#include "tiles.hpp"
#include "ts.hpp"
#include "visibility.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

e_tile tile_for_outdoor_job( e_outdoor_job job ) {
  switch( job ) {
    case e_outdoor_job::food: return e_tile::commodity_food;
    case e_outdoor_job::fish: return e_tile::product_fish;
    case e_outdoor_job::sugar: return e_tile::commodity_sugar;
    case e_outdoor_job::tobacco:
      return e_tile::commodity_tobacco;
    case e_outdoor_job::cotton: return e_tile::commodity_cotton;
    case e_outdoor_job::fur: return e_tile::commodity_fur;
    case e_outdoor_job::lumber: return e_tile::commodity_lumber;
    case e_outdoor_job::ore: return e_tile::commodity_ore;
    case e_outdoor_job::silver: return e_tile::commodity_silver;
  }
}

void render_glow( rr::Renderer& renderer, Coord unit_coord,
                  e_unit_type type ) {
  UnitTypeAttributes const& desc    = unit_attr( type );
  e_tile const              tile    = desc.tile;
  rr::Painter               painter = renderer.painter();
  render_sprite_silhouette(
      painter, unit_coord + Delta{ .w = 1 }, tile,
      config_colony.colors.outdoor_unit_glow_color );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Delta ColonyLandView::size_needed( e_render_mode mode ) {
  int side_length_in_squares = 3;
  switch( mode ) {
    case e_render_mode::_3x3: side_length_in_squares = 3; break;
    case e_render_mode::_5x5: side_length_in_squares = 5; break;
    case e_render_mode::_6x6: side_length_in_squares = 6; break;
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
  CHECK( event.pos.is_inside( rect( {} ) ) );
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

maybe<ColViewObject_t> ColonyLandView::can_receive(
    ColViewObject_t const& o, int, Coord const& where ) const {
  // Verify that the dragged object is a unit.
  maybe<UnitId> unit_id = o.get_if<ColViewObject::unit>().member(
      &ColViewObject::unit::id );
  if( !unit_id.has_value() ) return nothing;
  // Check if the unit is a human.
  Unit const& unit = ss_.units.unit_for( *unit_id );
  if( !unit.is_human() ) return nothing;
  // Check if there is a land square under the cursor that is
  // not the center.
  maybe<e_direction> d = direction_under_cursor( where );
  if( !d.has_value() ) return nothing;
  // Check if this is the same unit currently being dragged, if
  // so we'll allow it.
  if( dragging_.has_value() && d == dragging_->d ) return o;
  // Check if there is already a unit on the square.
  if( unit_under_cursor( where ).has_value() ) return nothing;
  // Check if there is a colonist from another colony (friendly
  // or foreign) that is already working on this square.
  if( occupied_red_box_[*d] ) return nothing;
  // Note that we don't check for water/docks here; that is
  // done in the check function.

  // We're good to go.
  return o;
}

wait<base::valid_or<DragRejection>> ColonyLandView::sink_check(
    ColViewObject_t const&, int, Coord const where ) {
  Colony const& colony = ss_.colonies.colony_for( colony_.id );
  maybe<e_direction> d = direction_under_cursor( where );
  CHECK( d );
  MapSquare const& square =
      ss_.terrain.square_at( colony.location.moved( *d ) );

  if( is_water( square ) &&
      !colony_has_building_level( colony,
                                  e_colony_building::docks ) ) {
    co_return DragRejection{
        .reason =
            "We must build @[H]docks@[] in this colony in "
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

ColonyJob_t ColonyLandView::make_job_for_square(
    e_direction d ) const {
  // The original game seems to always choose food.
  return ColonyJob::outdoor{ .direction = d,
                             .job       = e_outdoor_job::food };
}

wait<> ColonyLandView::drop( ColViewObject_t const& o,
                             Coord const&           where ) {
  UNWRAP_CHECK( unit_id, o.get_if<ColViewObject::unit>().member(
                             &ColViewObject::unit::id ) );
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  UNWRAP_CHECK( d, direction_under_cursor( where ) );
  ColonyJob_t job = make_job_for_square( d );
  if( dragging_.has_value() ) {
    // The unit being dragged is coming from another square on
    // the land view, so keep its job the same.
    job = ColonyJob::outdoor{ .direction = d,
                              .job       = dragging_->job };
  }
  move_unit_to_colony( ss_.units, colony, unit_id, job );
  CHECK_HAS_VALUE( colony.validate() );
  co_return;
}

maybe<DraggableObjectWithBounds<ColViewObject_t>>
ColonyLandView::object_here( Coord const& where ) const {
  UNWRAP_RETURN( unit_id, unit_under_cursor( where ) );
  UNWRAP_RETURN( d, direction_under_cursor( where ) );
  return DraggableObjectWithBounds<ColViewObject_t>{
      .obj    = ColViewObject::unit{ .id = unit_id },
      .bounds = rect_for_unit( d ) };
}

bool ColonyLandView::try_drag( ColViewObject_t const&,
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
  Colony& colony = ss_.colonies.colony_for( colony_.id );
  remove_unit_from_colony( ss_.units, colony, unit_id );
  co_return;
}

void ColonyLandView::draw_land_3x3( rr::Renderer& renderer,
                                    Coord         coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation,
      to_double( gfx::size( coord.distance_from_origin() ) ) );

  // This alpha is to fade the land tiles behind the units so
  // as to make the units more visible. Not sure yet if we want
  // to do that.
  double const alpha = 1.0;

  // FIXME: Should not be duplicating land-view rendering code
  // here.
  rr::Painter painter      = renderer.painter();
  Coord const world_square = colony_.location;
  auto        viz = Visibility::create( ss_, player_.nation );
  // Render terrain.
  for( auto local_coord :
       Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
    Coord render_square = world_square +
                          local_coord.distance_from_origin() -
                          Delta{ .w = 1, .h = 1 };
    painter.draw_solid_rect(
        Rect::from( local_coord * g_tile_delta, g_tile_delta ),
        gfx::pixel{ .r = 128, .g = 128, .b = 128, .a = 255 } );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
    render_terrain_square( renderer, local_coord * g_tile_delta,
                           render_square, viz,
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
          rr::Painter::e_border_mode::inside,
          gfx::pixel::red() );
  }
  // Render colonies.
  for( auto local_coord :
       Rect{ .x = 0, .y = 0, .w = 3, .h = 3 } ) {
    auto render_square = world_square +
                         local_coord.distance_from_origin() -
                         Delta{ .w = 1, .h = 1 };
    auto maybe_col_id =
        ss_.colonies.maybe_from_coord( render_square );
    if( !maybe_col_id ) continue;
    render_colony(
        painter,
        local_coord * g_tile_delta - Delta{ .w = 6, .h = 6 },
        ss_.colonies.colony_for( *maybe_col_id ) );
  }
}

void ColonyLandView::draw_land_6x6( rr::Renderer& renderer,
                                    Coord         coord ) const {
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, 2.0 );
    draw_land_3x3( renderer, coord );
  }
  // Further drawing should not be scaled.

  // Render units.
  rr::Painter   painter = renderer.painter();
  Colony const& colony  = ss_.colonies.colony_for( colony_.id );
  Coord const   center  = Coord{ .x = 1, .y = 1 };

  for( auto const& [direction, outdoor_unit] :
       colony.outdoor_jobs ) {
    if( !outdoor_unit.has_value() ) continue;
    if( dragging_.has_value() && dragging_->d == direction )
      continue;
    Coord const square_coord =
        coord + ( center.moved( direction ) * g_tile_delta *
                  Delta{ .w = 2, .h = 2 } )
                    .distance_from_origin();
    Coord const unit_coord =
        square_coord +
        ( g_tile_delta / Delta{ .w = 2, .h = 2 } );
    UnitId const unit_id = outdoor_unit->unit_id;
    Unit const&  unit    = ss_.units.unit_for( unit_id );
    UnitTypeAttributes const& desc = unit_attr( unit.type() );
    render_glow( renderer, unit_coord, unit.type() );
    render_unit_type(
        painter, unit_coord, desc.type,
        UnitRenderOptions{ .shadow = UnitShadow{} } );
    e_outdoor_job const job   = outdoor_unit->job;
    e_tile const product_tile = tile_for_outdoor_job( job );
    Coord const  product_coord =
        square_coord + Delta{ .w = 4, .h = 4 };
    render_sprite( painter, product_coord, product_tile );
    Delta const product_tile_size = sprite_size( product_tile );
    SquareProduction const& production =
        colview_production().land_production[direction];
    int const    quantity = production.quantity;
    string const q_str    = fmt::format( "x {}", quantity );
    Coord const  text_coord =
        product_coord + Delta{ .w = product_tile_size.w };
    render_text_markup(
        renderer, text_coord, {},
        TextMarkupInfo{
            .shadowed_text_color   = gfx::pixel::white(),
            .shadowed_shadow_color = gfx::pixel::black() },
        fmt::format( "@[S]{}@[]", q_str ) );
  }

  // Center square.
  Coord const square_coord =
      coord + ( center * g_tile_delta * Delta{ .w = 2, .h = 2 } )
                  .distance_from_origin();
  ColonyProduction const& production = colview_production();

  // food.
  {
    e_tile const product_tile =
        tile_for_outdoor_job( e_outdoor_job::food );
    Coord const product_coord =
        square_coord + Delta{ .w = 12, .h = 4 };
    render_sprite( painter, product_coord, product_tile );
    Delta const  product_tile_size = sprite_size( product_tile );
    int          quantity = production.center_food_production;
    string const q_str    = fmt::format( "x {}", quantity );
    Coord const  text_coord =
        product_coord + Delta{ .w = product_tile_size.w };
    render_text_markup(
        renderer, text_coord, {},
        TextMarkupInfo{
            .shadowed_text_color   = gfx::pixel::white(),
            .shadowed_shadow_color = gfx::pixel::black() },
        fmt::format( "@[S]{}@[]", q_str ) );
  }

  // secondary.
  if( production.center_extra_production.has_value() ) {
    e_outdoor_job const job =
        production.center_extra_production->what;
    e_tile const product_tile = tile_for_outdoor_job( job );
    Coord const  product_coord =
        square_coord + Delta{ .w = 12, .h = 32 };
    render_sprite( painter, product_coord, product_tile );
    Delta const product_tile_size = sprite_size( product_tile );
    int const   quantity =
        production.center_extra_production->quantity;
    string const q_str = fmt::format( "x {}", quantity );
    Coord const  text_coord =
        product_coord + Delta{ .w = product_tile_size.w };
    render_text_markup(
        renderer, text_coord, {},
        TextMarkupInfo{
            .shadowed_text_color   = gfx::pixel::white(),
            .shadowed_shadow_color = gfx::pixel::black() },
        fmt::format( "@[S]{}@[]", q_str ) );
  }
}

void ColonyLandView::draw( rr::Renderer& renderer,
                           Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  switch( mode_ ) {
    case e_render_mode::_3x3:
      draw_land_3x3( renderer, coord );
      break;
    case e_render_mode::_5x5:
      painter.draw_solid_rect( rect( coord ),
                               gfx::pixel::wood() );
      draw_land_3x3( renderer, coord + g_tile_delta );
      break;
    case e_render_mode::_6x6:
      draw_land_6x6( renderer, coord );
      break;
  }
}

unique_ptr<ColonyLandView> ColonyLandView::create(
    SS& ss, TS& ts, Colony& colony, Player const& player,
    e_render_mode mode ) {
  return make_unique<ColonyLandView>( ss, ts, colony, player,
                                      mode );
}

ColonyLandView::ColonyLandView( SS& ss, TS& ts, Colony& colony,
                                Player const& player,
                                e_render_mode mode )
  : ColonySubView( ss, ts, colony ),
    player_( player ),
    mode_( mode ),
    occupied_red_box_( find_occupied_surrounding_colony_squares(
        ss, colony ) ) {}

} // namespace rn
