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
#include "harbor-view-status.hpp"
#include "revolution.rds.hpp"
#include "views.hpp"

// ss
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"

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

bool shown_after_declaration(
    e_harbor_view_entity const entity ) {
  switch( entity ) {
    case e_harbor_view_entity::backdrop:
    case e_harbor_view_entity::dock:
    case e_harbor_view_entity::market:
    case e_harbor_view_entity::status_bar:
      return true;

    case e_harbor_view_entity::cargo:
    case e_harbor_view_entity::in_port:
    case e_harbor_view_entity::inbound:
    case e_harbor_view_entity::outbound:
    case e_harbor_view_entity::rpt:
      return false;
  }
}

} // namespace

/****************************************************************
** HarborSubView
*****************************************************************/
HarborSubView::HarborSubView( SS& ss, TS& ts, Player& player )
  : ss_( ss ),
    ts_( ts ),
    player_( player ),
    colonial_player_( player_for_player_or_die(
        ss.players, colonial_player_for( player.nation ) ) ) {
  CHECK( player_.nation == colonial_player_.nation );
}

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
      if( handled ) co_return true;
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

  // If we are a player that declared or if we are an REF player
  // then that means we are post declaration.
  bool const declared =
      is_ref( player.type ) ||
      player.revolution.status >= e_revolution_status::declared;
  using E = e_harbor_view_entity;

  // [HarborStatusBar] ------------------------------------------
  auto status_bar =
      HarborStatusBar::create( ss, ts, player, canvas_rect );
  auto& status_bar_ref                = *status_bar.actual;
  composition.entities[E::status_bar] = status_bar.harbor;
  views.push_back( std::move( status_bar.owned ) );

  // [HarborBackdrop] -------------------------------------------
  PositionedHarborSubView<HarborBackdrop> backdrop =
      HarborBackdrop::create( ss, ts, player, canvas_rect );
  auto const& backdrop_ref          = *backdrop.actual;
  composition.entities[E::backdrop] = backdrop.harbor;
  // NOTE: this one needs to be inserted at the beginning because
  // it needs to be drawn first.
  views.insert( views.begin(), std::move( backdrop.owned ) );

  // [HarborDockUnits]
  // ----------------------------------------
  // NOTE: this must be rendered before the market commodities so
  // that the overflow units remain behind the market panel.
  PositionedHarborSubView<HarborDockUnits> dock =
      HarborDockUnits::create( ss, ts, player, canvas_rect,
                               backdrop_ref );
  auto& dock_ref                = *dock.actual;
  composition.entities[E::dock] = dock.harbor;
  views.push_back( std::move( dock.owned ) );

  // [HarborMarketCommodities] ----------------------------------
  // NOTE: this must be rendered after dock units so that it re-
  // mains on top of the rows of overflow units.
  PositionedHarborSubView<HarborMarketCommodities>
      market_commodities = HarborMarketCommodities::create(
          ss, ts, player, canvas_rect, status_bar_ref );
  auto& market_commodities_ref    = *market_commodities.actual;
  composition.entities[E::market] = market_commodities.harbor;
  views.push_back( std::move( market_commodities.owned ) );

  if( !declared ) {
    // [HarborCargo] --------------------------------------------
    if( !declared ) {
      PositionedHarborSubView<HarborCargo> cargo =
          HarborCargo::create( ss, ts, player, canvas_rect,
                               status_bar_ref );
      composition.entities[E::cargo] = cargo.harbor;
      views.push_back( std::move( cargo.owned ) );
    }

    // [HarborInPortShips] --------------------------------------
    PositionedHarborSubView<HarborInPortShips> in_port =
        HarborInPortShips::create( ss, ts, player, canvas_rect,
                                   backdrop_ref,
                                   market_commodities_ref );
    auto& in_port_ships_ref          = *in_port.actual;
    composition.entities[E::in_port] = in_port.harbor;
    views.push_back( std::move( in_port.owned ) );

    // [HarborOutboundShips] ------------------------------------
    PositionedHarborSubView<HarborOutboundShips> outbound =
        HarborOutboundShips::create( ss, ts, player, canvas_rect,
                                     in_port_ships_ref );
    auto& outbound_ships_ref          = *outbound.actual;
    composition.entities[E::outbound] = outbound.harbor;
    views.push_back( std::move( outbound.owned ) );

    // [HarborInboundShips] ------------------------------------
    PositionedHarborSubView<HarborInboundShips> inbound =
        HarborInboundShips::create( ss, ts, player, canvas_rect,
                                    outbound_ships_ref );
    composition.entities[E::inbound] = inbound.harbor;
    views.push_back( std::move( inbound.owned ) );

    // [HarborRptButtons] --------------------------------------
    PositionedHarborSubView<HarborRptButtons> buttons =
        HarborRptButtons::create( ss, ts, player, canvas_rect,
                                  backdrop_ref, dock_ref );
    composition.entities[E::rpt] = buttons.harbor;
    views.push_back( std::move( buttons.owned ) );
  }

  // [Finish] ---------------------------------------------------
  auto invisible_view = std::make_unique<CompositeHarborSubView>(
      ss, ts, player, canvas_size, std::move( views ) );
  invisible_view->set_delta( canvas_size );
  composition.top_level = std::move( invisible_view );

  // Sanity check.
  for( auto e : refl::enum_values<E> ) {
    if( !declared || shown_after_declaration( e ) )
      CHECK( composition.entities[e] != nullptr,
             "harbor view entity {} is missing.", e );
    else if( declared && !shown_after_declaration( e ) )
      CHECK( composition.entities[e] == nullptr,
             "harbor view entity {} should not appear "
             "post-declaration.",
             e );
  }
  return composition;
}

} // namespace rn
