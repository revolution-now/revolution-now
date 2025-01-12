/****************************************************************
**harbor-view-entities.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The various UI sections/entities in the european
*              harbor view.
*
*****************************************************************/
#include "harbor-view-entities.hpp"

// Revolution Now
#include "harbor-view-backdrop.hpp"
#include "harbor-view-cargo.hpp"
#include "harbor-view-dock.hpp"
#include "harbor-view-inbound.hpp"
#include "harbor-view-inport.hpp"
#include "harbor-view-market.hpp"
#include "harbor-view-outbound.hpp"
#include "harbor-view-rpt.hpp"
#include "views.hpp"

// render
#include "render/painter.hpp"
#include "render/renderer.hpp"

// gfx
#include "gfx/resolution-enum.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::rect;
using ::gfx::size;

} // namespace

// Use this as the vtable key function.
void HarborSubView::update_this_and_children() {}

/****************************************************************
** HarborViewComposited
*****************************************************************/
void HarborViewComposited::update() {
  top_level->update_this_and_children();
}

/****************************************************************
** Entities
*****************************************************************/
// This file only contains some small entities that don't need
// their own files. The larger ones go in their own files for the
// sake of not having this module get too large.

/****************************************************************
** HarborStatusBar
*****************************************************************/
class HarborStatusBar : public ui::View, public HarborSubView {
 public:
  static unique_ptr<HarborStatusBar> create( SS& ss, TS& ts,
                                             Player& player,
                                             Delta size ) {
    return make_unique<HarborStatusBar>( ss, ts, player, size );
  }

  HarborStatusBar( SS& ss, TS& ts, Player& player, Delta size )
    : HarborSubView( ss, ts, player ), size_( size ) {}

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_harbor_view_entity::status_bar );
  }

  ui::View& view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  string status() const { return status_; }

  void draw( rr::Renderer& renderer,
             Coord coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( bounds( coord ),
                             gfx::pixel::wood() );
    renderer
        .typer( centered( Delta::from_gfx(
                              rr::rendered_text_line_size_pixels(
                                  status() ) ),
                          bounds( coord ) ),
                gfx::pixel::banana() )
        .write( status() );
  }

 private:
  Delta size_;
  string status_;
};

/****************************************************************
** CompositeHarborSubView
*****************************************************************/
// FIXME: should try to dedupe this with the one in the colony
// view since it is nearly identical.
struct CompositeHarborSubView : public ui::InvisibleView,
                                public HarborSubView {
  CompositeHarborSubView(
      SS& ss, TS& ts, Player& player, Delta size,
      std::vector<ui::OwningPositionedView> views )
    : ui::InvisibleView( size, std::move( views ) ),
      HarborSubView( ss, ts, player ) {
    for( ui::PositionedView v : *this ) {
      auto* col_view = dynamic_cast<HarborSubView*>( v.view );
      CHECK( col_view );
      ptrs_.push_back( col_view );
    }
    CHECK( int( ptrs_.size() ) == count() );
  }

  // Implement HarborSubView.
  ui::View& view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    for( int i = count() - 1; i >= 0; --i ) {
      ui::PositionedView pos_view = at( i );
      if( !event.pos.is_inside( pos_view.rect() ) ) continue;
      input::event_t const shifted_event =
          input::mouse_origin_moved_by(
              event, pos_view.coord.distance_from_origin() );
      UNWRAP_CHECK(
          shifted_mouse_button_event,
          shifted_event.get_if<input::mouse_button_event_t>() );
      // Need to co_await so that shifted_event stays alive.
      co_await ptrs_[i]->perform_click(
          shifted_mouse_button_event );
      break;
    }
  }

  // Implement AwaitView.
  wait<bool> perform_key(
      input::key_event_t const& event ) override {
    for( int i = count() - 1; i >= 0; --i ) {
      bool const handled =
          co_await ptrs_[i]->perform_key( event );
      if( handled ) break;
    }
    co_return false; // not handled.
  }

  maybe<PositionedDraggableSubView<HarborDraggableObject>>
  view_here( Coord coord ) override {
    for( int i = count() - 1; i >= 0; --i ) {
      ui::PositionedView pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<PositionedDraggableSubView<HarborDraggableObject>>
          p_view = ptrs_[i]->view_here(
              coord.with_new_origin( pos_view.coord ) );
      if( !p_view ) continue;
      p_view->upper_left =
          p_view->upper_left.as_if_origin_were( pos_view.coord );
      return p_view;
    }
    if( coord.is_inside( bounds( {} ) ) )
      return PositionedDraggableSubView<HarborDraggableObject>{
        this, Coord{} };
    return nothing;
  }

  maybe<DraggableObjectWithBounds<HarborDraggableObject>>
  object_here( Coord const& coord ) const override {
    for( int i = count() - 1; i >= 0; --i ) {
      ui::PositionedViewConst pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<DraggableObjectWithBounds<HarborDraggableObject>>
          obj = ptrs_[i]->object_here(
              coord.with_new_origin( pos_view.coord ) );
      if( !obj ) continue;
      obj->bounds =
          obj->bounds.as_if_origin_were( pos_view.coord );
      return obj;
    }
    // This view itself has no objects.
    return nothing;
  }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override { return nothing; }

  void update_this_and_children() override {
    for( HarborSubView* p : ptrs_ )
      p->update_this_and_children();
  }

  vector<HarborSubView*> ptrs_;
};

/****************************************************************
** Public API
*****************************************************************/
HarborViewComposited recomposite_harbor_view(
    SS& ss, TS& ts, Player& player,
    e_resolution const resolution ) {
  size const canvas_size = resolution_size( resolution );
  rect const canvas_rect{ .size =
                              resolution_size( resolution ) };
  lg.trace( "recompositing harbor view." );

  HarborViewComposited composition{ .canvas_size = canvas_size,
                                    .top_level   = nullptr,
                                    .entities    = {} };

  vector<ui::OwningPositionedView> views;

  // [HarborStatusBar] ------------------------------------------
  auto status_bar = HarborStatusBar::create(
      ss, ts, player, Delta{ .w = canvas_size.w, .h = 10 } );
  composition.entities[e_harbor_view_entity::status_bar] =
      status_bar.get();
  Y const status_bar_bottom =
      status_bar->bounds( canvas_rect.nw() ).bottom_edge();
  views.push_back( ui::OwningPositionedView{
    .view  = std::move( status_bar ),
    .coord = canvas_rect.nw() } );

  // [HarborMarketCommodities] ----------------------------------
  PositionedHarborSubView<HarborMarketCommodities>
      market_commodities = HarborMarketCommodities::create(
          ss, ts, player, canvas_rect );
  HarborMarketCommodities const& market_commodities_ref =
      *market_commodities.actual;
  composition.entities[e_harbor_view_entity::market] =
      market_commodities.harbor;
  views.push_back( std::move( market_commodities.owned ) );

  // [HarborCargo] ----------------------------------------------
  PositionedHarborSubView<HarborCargo> cargo =
      HarborCargo::create( ss, ts, player, canvas_rect );
  composition.entities[e_harbor_view_entity::cargo] =
      cargo.harbor;
  Coord const cargo_upper_left = cargo.owned.rect().upper_left();
  Coord const cargo_upper_right =
      cargo.owned.rect().upper_right();
  views.push_back( std::move( cargo.owned ) );

  // [HarborInPortShips] ----------------------------------------
  PositionedHarborSubView<HarborInPortShips> in_port =
      HarborInPortShips::create( ss, ts, player, canvas_rect,
                                 market_commodities_ref,
                                 cargo_upper_left );
  composition.entities[e_harbor_view_entity::in_port] =
      in_port.harbor;
  Coord const inport_upper_left =
      in_port.owned.rect().upper_left();
  Coord const inport_upper_right =
      in_port.owned.rect().upper_right();
  views.push_back( std::move( in_port.owned ) );

  // [HarborBackdrop] -------------------------------------------
  PositionedHarborSubView<HarborBackdrop> backdrop =
      HarborBackdrop::create( ss, ts, player, canvas_rect );
  auto const& backdrop_ref = *backdrop.actual;
  composition.entities[e_harbor_view_entity::backdrop] =
      backdrop.harbor;
  // NOTE: this one needs to be inserted at the beginning because
  // it needs to be drawn first.
  views.insert( views.begin(), std::move( backdrop.owned ) );

  // [HarborOutboundShips]
  // ----------------------------------------
  PositionedHarborSubView<HarborOutboundShips> outbound =
      HarborOutboundShips::create( ss, ts, player, canvas_rect,
                                   market_commodities_ref,
                                   inport_upper_left );
  composition.entities[e_harbor_view_entity::outbound] =
      outbound.harbor;
  Coord const outbound_upper_left =
      outbound.owned.rect().upper_left();
  views.push_back( std::move( outbound.owned ) );

  // [HarborInboundShips]
  // ----------------------------------------
  PositionedHarborSubView<HarborInboundShips> inbound =
      HarborInboundShips::create( ss, ts, player, canvas_rect,
                                  market_commodities_ref,
                                  outbound_upper_left );
  composition.entities[e_harbor_view_entity::inbound] =
      inbound.harbor;
  views.push_back( std::move( inbound.owned ) );

  // [HarborDockUnits]
  // ----------------------------------------
  PositionedHarborSubView<HarborDockUnits> dock =
      HarborDockUnits::create( ss, ts, player, canvas_rect,
                               backdrop_ref );
  auto& dock_units_ref = *dock.actual;
  composition.entities[e_harbor_view_entity::dock] = dock.harbor;
  views.push_back( std::move( dock.owned ) );

  // [HarborRptButtons]
  // ----------------------------------------
  PositionedHarborSubView<HarborRptButtons> buttons =
      HarborRptButtons::create( ss, ts, player, canvas_rect,
                                backdrop_ref, dock_units_ref );
  composition.entities[e_harbor_view_entity::rpt] =
      buttons.harbor;
  views.push_back( std::move( buttons.owned ) );

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<CompositeHarborSubView>(
      ss, ts, player, canvas_size, std::move( views ) );
  invisible_view->set_delta( canvas_size );
  composition.top_level = std::move( invisible_view );

  for( auto e : refl::enum_values<e_harbor_view_entity> ) {
    CHECK( composition.entities.contains( e ),
           "harbor view entity {} is missing.", e );
  }
  return composition;
}

} // namespace rn
