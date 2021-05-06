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
#include "waitable-coro.hpp"
#include "window.hpp"

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
  ColonyId                  id;
  Delta                     screen_size;
  unique_ptr<ColonySubView> top_level;
  ColViewEntityPtrs         top_level_ptrs;
  unordered_map<e_colview_entity, ColViewEntityPtrs> entities;
};

ColViewComposited g_composition;

/****************************************************************
** Helpers
*****************************************************************/
ColonyId colony_id() { return g_composition.id; }

/****************************************************************
** Entities
*****************************************************************/
class TitleBar : public ui::View, public ColonySubView {
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

  static unique_ptr<TitleBar> create( Delta size ) {
    return make_unique<TitleBar>( size );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::title_bar;
  // }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord ) const override {
    return nothing;
  }

  TitleBar( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class MarketCommodities : public ui::View, public ColonySubView {
public:
  Delta delta() const override {
    return Delta{
        block_width_ * SX{ enum_traits<e_commodity>::count },
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
    auto  comm_it = enum_traits<e_commodity>::values.begin();
    auto  label   = CommodityLabel::quantity{ 0 };
    Coord pos     = coord;
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

  static unique_ptr<MarketCommodities> create( W block_width ) {
    return make_unique<MarketCommodities>( block_width );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::commodities;
  // }

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

  MarketCommodities( W block_width )
    : block_width_( block_width ) {}

private:
  W block_width_;
};

class PopulationView : public ui::View, public ColonySubView {
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

  static unique_ptr<PopulationView> create( Delta size ) {
    return make_unique<PopulationView>( size );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::population;
  // }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  PopulationView( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class CargoView : public ui::View, public ColonySubView {
public:
  Delta delta() const override { return size_; }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
    for( auto [unit_id, unit_pos] : positioned_units_ )
      render_unit( tx, unit_id, unit_pos, /*with_icon=*/true );
    // TODO: Draw cargo.
  }

  static unique_ptr<CargoView> create( Delta size ) {
    return make_unique<CargoView>( size );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::cargo;
  // }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  CargoView( Delta size ) : size_( size ) {
    update_unit_layout();
  }

  // Implement AwaitableView.
  waitable<> perform_click( Coord pos ) override {
    CHECK( pos.is_inside( rect( {} ) ) );
    for( auto [unit_id, unit_pos] : positioned_units_ ) {
      if( pos.is_inside(
              Rect::from( unit_pos, g_tile_delta ) ) ) {
        auto& unit = unit_from_id( unit_id );
        co_await ui::message_box( "Clicked on unit: {}",
                                  debug_string( unit ) );
        // FIXME: we need to somehow get this back to the turn
        // module so that it can be added to the back of the
        // queue.
        unit.clear_orders();
      }
    }
  }

private:
  struct PositionedUnit {
    UnitId id;
    Coord  pos; // relative to upper left of this CargoView.
  };

  vector<PositionedUnit> positioned_units_;

  void update_unit_layout() {
    auto const& colony   = colony_from_id( colony_id() );
    auto const& units    = units_from_coord( colony.location() );
    auto        unit_pos = Coord{} + 16_h;
    positioned_units_.clear();
    for( auto unit_id : units ) {
      positioned_units_.push_back(
          { .id = unit_id, .pos = unit_pos } );
      unit_pos += 32_w;
    }
  }

  Delta size_;
};

class ProductionView : public ui::View, public ColonySubView {
public:
  Delta delta() const override { return size_; }

  void draw( Texture& tx, Coord coord ) const override {
    render_rect( tx, Color::black(),
                 rect( coord ).with_inc_size() );
  }

  static unique_ptr<ProductionView> create( Delta size ) {
    return make_unique<ProductionView>( size );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::production;
  // }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  ProductionView( Delta size ) : size_( size ) {}

private:
  Delta size_;
};

class LandView : public ui::View, public ColonySubView {
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

  static unique_ptr<LandView> create( e_render_mode mode ) {
    return make_unique<LandView>(
        mode,
        Texture::create( Delta{ 3_w, 3_h } * g_tile_scale ) );
  }

  // e_colview_entity entity_id() const override {
  //   return e_colview_entity::land;
  // }

  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    if( !coord.is_inside( rect( {} ) ) ) return nothing;
    return nothing;
  }

  LandView( e_render_mode mode, Texture land_tx )
    : mode_( mode ), land_tx_( std::move( land_tx ) ) {}

private:
  e_render_mode   mode_;
  mutable Texture land_tx_;
};

/****************************************************************
** Compositing
*****************************************************************/
struct CompositeColSubView : public ui::InvisibleView,
                             public ColonySubView {
  CompositeColSubView(
      Delta size, std::vector<ui::OwningPositionedView> views )
    : ui::InvisibleView( size, std::move( views ) ) {
    for( ui::PositionedView v : *this ) {
      auto* col_view = dynamic_cast<ColonySubView*>( v.view );
      CHECK( col_view );
      ptrs_.push_back( ColViewEntityPtrs{
          .col_view = col_view,
          .view     = v.view,
      } );
    }
  }

  ui::InvisibleView&       as_view() { return *this; }
  ColonySubView&           as_colview() { return *this; }
  ui::InvisibleView const& as_view() const { return *this; }
  ColonySubView const&     as_colview() const { return *this; }

  // Implement AwaitableView.
  waitable<> perform_click( Coord pos ) override {
    DCHECK( int( ptrs_.size() ) == count() );
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !pos.is_inside( pos_view.rect() ) ) continue;
      return ptrs_[i].col_view->perform_click(
          pos.as_if_origin_were( pos_view.coord ) );
    }
    return make_waitable<>();
  }

  // Implement ColonySubView.
  maybe<ColViewObjectUnderCursor> obj_under_cursor(
      Coord coord ) const override {
    DCHECK( int( ptrs_.size() ) == count() );
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedViewConst pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      if( auto maybe_obj = ptrs_[i].col_view->obj_under_cursor(
              coord.as_if_origin_were( pos_view.coord ) );
          maybe_obj )
        return maybe_obj;
    }
    return nothing;
  }

  vector<ColViewEntityPtrs> ptrs_;
};

template<typename T>
ColViewEntityPtrs MakeEntityPtrs( T* p ) {
  return { .col_view = p, .view = p };
}

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
      TitleBar::create( Delta{ 10_h, screen_size.w } );
  g_composition.entities[e_colview_entity::title_bar] =
      MakeEntityPtrs( title_bar.get() );
  pos = Coord{};
  Y const title_bar_bottom =
      title_bar->rect( pos ).bottom_edge();
  views.push_back(
      ui::OwningPositionedView( std::move( title_bar ), pos ) );

  // [MarketCommodities] ----------------------------------------
  W comm_block_width = screen_rect.delta().w /
                       SX{ enum_traits<e_commodity>::count };
  comm_block_width =
      std::clamp( comm_block_width, kCommodityTileSize.w, 32_w );
  auto market_commodities =
      MarketCommodities::create( comm_block_width );
  g_composition.entities[e_colview_entity::commodities] =
      MakeEntityPtrs( market_commodities.get() );
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
  auto population_view =
      PopulationView::create( middle_strip_size.with_width(
          middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::population] =
      MakeEntityPtrs( population_view.get() );
  pos = Coord{ 0_x, middle_strip_top };
  X const population_right_edge =
      population_view->rect( pos ).right_edge();
  views.push_back( ui::OwningPositionedView(
      std::move( population_view ), pos ) );

  // [Cargo] ----------------------------------------------------
  auto cargo_view =
      CargoView::create( middle_strip_size.with_width(
          middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::cargo] =
      MakeEntityPtrs( cargo_view.get() );
  pos = Coord{ population_right_edge, middle_strip_top };
  X const cargo_right_edge =
      cargo_view->rect( pos ).right_edge();
  views.push_back(
      ui::OwningPositionedView( std::move( cargo_view ), pos ) );

  // [Production] -----------------------------------------------
  auto production_view =
      ProductionView::create( middle_strip_size.with_width(
          middle_strip_size.w / 3_sx ) );
  g_composition.entities[e_colview_entity::production] =
      MakeEntityPtrs( production_view.get() );
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
  auto land_view = LandView::create( land_view_mode );
  g_composition.entities[e_colview_entity::land] =
      MakeEntityPtrs( land_view.get() );
  pos = g_composition.entities[e_colview_entity::title_bar]
            .view->rect( Coord{} )
            .lower_right() -
        land_view->delta().w;
  views.push_back(
      ui::OwningPositionedView( std::move( land_view ), pos ) );

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<CompositeColSubView>(
      screen_rect.delta(), std::move( views ) );
  invisible_view->set_delta( screen_rect.delta() );
  g_composition.top_level_ptrs =
      MakeEntityPtrs( invisible_view.get() );
  g_composition.top_level = std::move( invisible_view );

  for( auto e : enum_traits<e_colview_entity>::values ) {
    CHECK( g_composition.entities.contains( e ),
           "colview entity {} is missing.", e );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColViewEntityPtrs colview_entity( e_colview_entity ) {
  NOT_IMPLEMENTED;
  return {};
}

ColViewEntityPtrs colview_top_level() {
  return g_composition.top_level_ptrs;
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
