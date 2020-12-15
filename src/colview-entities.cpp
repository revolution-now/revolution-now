/****************************************************************
**colview-entities.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-12.
*
* Description: The various UI sections/entities in Colony view.
*
*****************************************************************/
#include "colview-entities.hpp"

// Revolution Now
#include "colony.hpp"
#include "commodity.hpp"
#include "cstate.hpp"
#include "gfx.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "ustate.hpp"
#include "views.hpp"

// magic-enum
#include "magic_enum.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Constants
*****************************************************************/
constexpr Delta kCommodityTileSize  = Delta{ 16_w, 16_h };
constexpr Scale kCommodityTileScale = Scale{ 16_sx, 16_sy };

constexpr W kCommodityTileWidth = 16_w;

/****************************************************************
** Globals
*****************************************************************/
struct ColViewComposited {
  ColonyId             id;
  Delta                screen_size;
  unique_ptr<ui::View> top_level;
  unordered_map<e_colview_entity, ColViewEntityView*> entities;
};

ColViewComposited g_composition;

/****************************************************************
** Entities
*****************************************************************/
class TitleBar : public ColViewEntityView {
public:
  Delta delta() const override { return size_; }

  string title() const {
    auto const& colony = colony_from_id( colony_id() );
    return fmt::format( "{}, population {}", colony.name(),
                        colony.population() );
  }

  void draw( Texture& tx, Coord coord ) const override {
    render_fill_rect( tx, Color::wood(), rect( coord ) );
    Texture const& name = render_text(
        font::standard(), Color::banana(), title() );
    name.copy_to( tx, centered( name.size(), rect( coord ) ) );
  }

  static unique_ptr<TitleBar> create( ColonyId id, Delta size ) {
    return make_unique<TitleBar>( id, size );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::title_bar;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord ) const override {
    return nothing;
  }

  TitleBar( ColonyId id, Delta size )
    : ColViewEntityView( id ), size_( size ) {}

private:
  Delta size_;
};

class MarketCommodities : public ColViewEntityView {
public:
  Delta delta() const override {
    return Delta{
        block_width_ *
            SX{ magic_enum::enum_values<e_commodity>().size() },
        1_h * 32_sy };
  }

  // Offset within a block that the commodity icon should be dis-
  // played.
  Delta rendered_commodity_offset() const {
    Delta res;
    res.h = 3_h;
    res.w = ( block_width_ - kCommodityTileWidth ) / 2_sx;
    if( res.w < 0_w ) res.w = 0_w;
    return res;
  }

  void draw( Texture& tx, Coord coord ) const override {
    auto comm_it =
        magic_enum::enum_values<e_commodity>().begin();
    auto        label  = CommodityLabel::quantity{ 0 };
    Coord       pos    = coord;
    auto const& colony = colony_from_id( colony_id() );
    for( int i = 0; i < kNumCommodityTypes; ++i ) {
      auto rect = Rect::from( pos, Delta{ 32_h, block_width_ } );
      render_rect( tx, Color::black(), rect );
      label.value = colony.commodity_quantity( *comm_it );
      render_commodity_annotated(
          tx, *comm_it++,
          rect.upper_left() + rendered_commodity_offset(),
          label );
      pos += block_width_;
    }
  }

  static unique_ptr<MarketCommodities> create( ColonyId id,
                                               W block_width ) {
    return make_unique<MarketCommodities>( id, block_width );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::commodities;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    auto sprite_scale = Scale{ SX{ block_width_._ }, SY{ 32 } };
    auto box_upper_left =
        ( coord / sprite_scale ) * sprite_scale;
    auto idx        = ( coord / sprite_scale - Coord{} ).w._;
    auto maybe_type = commodity_from_index( idx );
    if( !maybe_type ) return nothing;
    return ColViewObjectUnderCursor{
        .obj =
            ColViewDraggableObject::market_commodity{
                .type = *maybe_type },
        .bounds =
            Rect::from(
                box_upper_left + rendered_commodity_offset(),
                Delta{ 1_w, 1_h } * kCommodityTileScale )
                .as_if_origin_were( coord ) };
  }

  MarketCommodities( ColonyId id, W block_width )
    : ColViewEntityView( id ), block_width_( block_width ) {}

private:
  W block_width_;
};

class PopulationView : public ColViewEntityView {
public:
  Delta delta() const override { return size_; }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    auto const& colony = colony_from_id( colony_id() );
    unordered_map<UnitId, ColonyJob_t> const& units_jobs =
        colony.units_jobs();
    auto unit_pos = coord + 16_h;
    for( auto const& [unit_id, job] : units_jobs ) {
      render_unit( tx, unit_id, unit_pos, /*with_icon=*/false );
      unit_pos += 24_w;
    }
  }

  static unique_ptr<PopulationView> create( ColonyId id,
                                            Delta    size ) {
    return make_unique<PopulationView>( id, size );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::population;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  PopulationView( ColonyId id, Delta size )
    : ColViewEntityView( id ), size_( size ) {}

private:
  Delta size_;
};

class CargoView : public ColViewEntityView {
public:
  Delta delta() const override { return size_; }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    auto const& colony   = colony_from_id( colony_id() );
    auto const& units    = units_from_coord( colony.location() );
    auto        unit_pos = coord + 16_h;
    for( auto unit_id : units ) {
      render_unit( tx, unit_id, unit_pos, /*with_icon=*/true );
      unit_pos += 32_w;
    }
    // TODO: Draw cargo.
  }

  static unique_ptr<CargoView> create( ColonyId id,
                                       Delta    size ) {
    return make_unique<CargoView>( id, size );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::cargo;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  CargoView( ColonyId id, Delta size )
    : ColViewEntityView( id ), size_( size ) {}

private:
  Delta size_;
};

class ProductionView : public ColViewEntityView {
public:
  Delta delta() const override { return size_; }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
  }

  static unique_ptr<ProductionView> create( ColonyId id,
                                            Delta    size ) {
    return make_unique<ProductionView>( id, size );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::production;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  ProductionView( ColonyId id, Delta size )
    : ColViewEntityView( id ), size_( size ) {}

private:
  Delta size_;
};

class LandView : public ColViewEntityView {
public:
  enum class e_render_mode {
    // Three tiles by three tiles, with unscaled tiles and
    // colonists on the land files.
    _3x3,
    // Land is 3x3 with unscaled tiles, and there is a one-tile
    // wood border around it where colonists stay.
    _5x5,
    // Land is 3x3 tiles by each tile is scaled by a factor of
    // two to easily fit both colonists and commodities inline.
    _6x6
  };

  static Delta size_needed( e_render_mode mode ) {
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
    return Delta{ 32_w, 32_h } * Scale{ side_length_in_squares };
  }

  Delta delta() const override { return size_needed( mode_ ); }

  void draw_land_3x3( Texture& tx, Coord coord ) const {
    auto const& colony       = colony_from_id( colony_id() );
    Coord       world_square = colony.location();
    for( auto local_coord : Rect{ 0_x, 0_y, 3_w, 3_h } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ 1_w, 1_h };
      render_terrain_square( tx, render_square,
                             ( local_coord * g_tile_scale )
                                 .as_if_origin_were( coord ) );
    }
    for( auto local_coord : Rect{ 0_x, 0_y, 3_w, 3_h } ) {
      auto render_square = world_square +
                           local_coord.distance_from_origin() -
                           Delta{ 1_w, 1_h };
      auto maybe_col_id = colony_from_coord( render_square );
      if( !maybe_col_id ) continue;
      render_colony( tx, *maybe_col_id,
                     ( local_coord * g_tile_scale )
                             .as_if_origin_were( coord ) -
                         Delta{ 6_w, 6_h } );
    }
  }

  void draw_land_6x6( Texture& tx, Coord coord ) const {
    land_tx_.fill();
    draw_land_3x3( land_tx_, Coord{} );
    auto dst_rect = Rect::from( coord, delta() );
    land_tx_.copy_to( tx, /*src=*/nothing, dst_rect );
  }

  void draw( Texture& tx, Coord coord ) const override {
    switch( mode_ ) {
      case e_render_mode::_3x3:
        draw_land_3x3( tx, coord );
        break;
      case e_render_mode::_5x5:
        render_fill_rect( tx, Color::wood(), rect( coord ) );
        draw_land_3x3( tx, coord + g_tile_delta );
        break;
      case e_render_mode::_6x6:
        draw_land_6x6( tx, coord );
        break;
    }
  }

  static unique_ptr<LandView> create( ColonyId      id,
                                      e_render_mode mode ) {
    return make_unique<LandView>(
        id, mode,
        Texture::create( Delta{ 3_w, 3_h } * g_tile_scale ) );
  }

  e_colview_entity entity_id() const override {
    return e_colview_entity::land;
  }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  LandView( ColonyId id, e_render_mode mode, Texture land_tx )
    : ColViewEntityView( id ),
      mode_( mode ),
      land_tx_( std::move( land_tx ) ) {}

private:
  e_render_mode   mode_;
  mutable Texture land_tx_;
};

/****************************************************************
** Compositing
*****************************************************************/
void recomposite( ColonyId id, Delta screen_size ) {
  CHECK( colony_exists( id ) );
  g_composition.id          = id;
  g_composition.screen_size = screen_size;

  g_composition.top_level = nullptr;
  g_composition.entities.clear();
  vector<ui::OwningPositionedView> views;

  Rect  screen_rect = Rect::from( Coord{}, screen_size );
  Coord pos;
  Delta available;

  // [Title Bar] ------------------------------------------------
  auto title_bar =
      TitleBar::create( id, Delta{ 10_h, screen_size.w } );
  g_composition.entities[e_colview_entity::title_bar] =
      title_bar.get();
  pos = Coord{};
  Y const title_bar_bottom =
      title_bar->rect( pos ).bottom_edge();
  views.push_back(
      ui::OwningPositionedView( std::move( title_bar ), pos ) );

  // [MarketCommodities] ----------------------------------------
  W comm_block_width =
      screen_rect.delta().w /
      SX{ magic_enum::enum_values<e_commodity>().size() };
  comm_block_width =
      std::clamp( comm_block_width, kCommodityTileSize.w, 32_w );
  auto market_commodities =
      MarketCommodities::create( id, comm_block_width );
  g_composition.entities[e_colview_entity::commodities] =
      market_commodities.get();
  pos = centered_bottom( market_commodities->delta(),
                         screen_rect );
  auto const market_commodities_top = pos.y;
  views.push_back( ui::OwningPositionedView(
      std::move( market_commodities ), pos ) );

  // [Middle Strip] ---------------------------------------------
  Delta   middle_strip_size{ screen_size.w, 32_h + 32_h + 16_h };
  Y const middle_strip_top =
      market_commodities_top - middle_strip_size.h;

  // [Population] -----------------------------------------------
  auto population_view = PopulationView::create(
      id, middle_strip_size.with_width( middle_strip_size.w /
                                        3_sx ) );
  g_composition.entities[e_colview_entity::population] =
      population_view.get();
  pos = Coord{ 0_x, middle_strip_top };
  X const population_right_edge =
      population_view->rect( pos ).right_edge();
  views.push_back( ui::OwningPositionedView(
      std::move( population_view ), pos ) );

  // [Cargo] ----------------------------------------------------
  auto cargo_view =
      CargoView::create( id, middle_strip_size.with_width(
                                 middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::cargo] =
      cargo_view.get();
  pos = Coord{ population_right_edge, middle_strip_top };
  X const cargo_right_edge =
      cargo_view->rect( pos ).right_edge();
  views.push_back(
      ui::OwningPositionedView( std::move( cargo_view ), pos ) );

  // [Production] -----------------------------------------------
  auto production_view = ProductionView::create(
      id, middle_strip_size.with_width( middle_strip_size.w /
                                        3_sx ) );
  g_composition.entities[e_colview_entity::production] =
      production_view.get();
  pos = Coord{ cargo_right_edge, middle_strip_top };
  views.push_back( ui::OwningPositionedView(
      std::move( production_view ), pos ) );

  // [LandView] -------------------------------------------------
  available = Delta{ screen_size.w,
                     middle_strip_top - title_bar_bottom };

  H max_landview_height = available.h;

  LandView::e_render_mode land_view_mode =
      LandView::e_render_mode::_6x6;
  if( LandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = LandView::e_render_mode::_5x5;
  if( LandView::size_needed( land_view_mode ).h >
      max_landview_height )
    land_view_mode = LandView::e_render_mode::_3x3;
  auto land_view = LandView::create( id, land_view_mode );
  g_composition.entities[e_colview_entity::land] =
      land_view.get();
  pos = g_composition.entities[e_colview_entity::title_bar]
            ->rect( Coord{} )
            .lower_right() -
        land_view->delta().w;
  views.push_back(
      ui::OwningPositionedView( std::move( land_view ), pos ) );

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<ui::InvisibleView>(
      screen_rect.delta(), std::move( views ) );
  invisible_view->set_delta( screen_rect.delta() );
  g_composition.top_level = std::move( invisible_view );

  for( auto e : magic_enum::enum_values<e_colview_entity>() ) {
    CHECK( g_composition.entities.contains( e ),
           "colview entity {} is missing.", e );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColViewEntityView const* colview_entity(
    e_colview_entity entity ) {
  (void)entity;
  return nullptr;
}

ui::View const* colview_top_level() {
  return g_composition.top_level.get();
}

void set_colview_colony( ColonyId id ) {
  auto old_id   = g_composition.id;
  auto old_size = g_composition.screen_size;
  auto new_id   = id;
  auto new_size = main_window_logical_size();
  if( new_id == old_id && new_size == old_size ) return;
  recomposite( new_id, new_size );
}

} // namespace rn
