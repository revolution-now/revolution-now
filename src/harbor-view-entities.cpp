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
#include "harbor-view-cargo.hpp"
#include "harbor-view-market.hpp"
#include "logger.hpp"
#include "views.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

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
                                             Delta   size ) {
    return make_unique<HarborStatusBar>( ss, ts, player, size );
  }

  HarborStatusBar( SS& ss, TS& ts, Player& player, Delta size )
    : HarborSubView( ss, ts, player ), size_( size ) {}

  Delta delta() const override { return size_; }

  // Implement IDraggableObjectsView.
  maybe<int> entity() const override {
    return static_cast<int>( e_harbor_view_entity::status_bar );
  }

  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  string status() const { return status_; }

  void draw( rr::Renderer& renderer,
             Coord         coord ) const override {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( rect( coord ), gfx::pixel::wood() );
    renderer
        .typer( centered( Delta::from_gfx(
                              rr::rendered_text_line_size_pixels(
                                  status() ) ),
                          rect( coord ) ),
                gfx::pixel::banana() )
        .write( status() );
  }

 private:
  Delta  size_;
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
  ui::View&       view() noexcept override { return *this; }
  ui::View const& view() const noexcept override {
    return *this;
  }

  // Implement AwaitView.
  wait<> perform_click(
      input::mouse_button_event_t const& event ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !event.pos.is_inside( pos_view.rect() ) ) continue;
      input::event_t const shifted_event =
          input::move_mouse_origin_by(
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

  maybe<PositionedDraggableSubView> view_here(
      Coord coord ) override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedView pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<PositionedDraggableSubView> p_view =
          ptrs_[i]->view_here(
              coord.with_new_origin( pos_view.coord ) );
      if( !p_view ) continue;
      p_view->upper_left =
          p_view->upper_left.as_if_origin_were( pos_view.coord );
      return p_view;
    }
    if( coord.is_inside( rect( {} ) ) )
      return PositionedDraggableSubView{ this, Coord{} };
    return nothing;
  }

  maybe<DraggableObjectWithBounds> object_here(
      Coord const& coord ) const override {
    for( int i = 0; i < count(); ++i ) {
      ui::PositionedViewConst pos_view = at( i );
      if( !coord.is_inside( pos_view.rect() ) ) continue;
      maybe<DraggableObjectWithBounds> obj =
          ptrs_[i]->object_here(
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
    SS& ss, TS& ts, Player& player, Delta const& canvas_size ) {
  lg.trace( "recompositing harbor view." );

  Rect const canvas_rect = Rect::from( Coord{}, canvas_size );
  HarborViewComposited composition{ .canvas_size = canvas_size,
                                    .top_level   = nullptr,
                                    .entities    = {} };

  vector<ui::OwningPositionedView> views;

  Rect available = canvas_rect;

  // [HarborStatusBar] ------------------------------------------
  auto status_bar = HarborStatusBar::create(
      ss, ts, player, Delta{ .w = canvas_size.w, .h = 10 } );
  composition.entities[e_harbor_view_entity::status_bar] =
      status_bar.get();
  Y const status_bar_bottom =
      status_bar->rect( available.upper_left() ).bottom_edge();
  views.push_back( ui::OwningPositionedView{
      .view  = std::move( status_bar ),
      .coord = available.upper_left() } );
  available = available.with_new_top_edge( status_bar_bottom );

  // [HarborMarketCommodities] ----------------------------------
  PositionedHarborSubView market_commodities =
      HarborMarketCommodities::create( ss, ts, player,
                                       available );
  composition.entities[e_harbor_view_entity::market] =
      market_commodities.harbor;
  available = available.with_new_bottom_edge(
      market_commodities.owned.rect().top_edge() );
  views.push_back( std::move( market_commodities.owned ) );

  // [HarborCargo] ----------------------------------------------
  PositionedHarborSubView cargo =
      HarborCargo::create( ss, ts, player, available );
  composition.entities[e_harbor_view_entity::cargo] =
      cargo.harbor;
  views.push_back( std::move( cargo.owned ) );

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
