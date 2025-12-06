/****************************************************************
**panel-plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-01.
*
* Description: The side panel on land view.
*
*****************************************************************/
#include "panel-plane.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "error.hpp"
#include "fathers.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imenu-handler.hpp"
#include "imenu-server.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "mini-map.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "player-mgr.hpp"
#include "render.hpp"
#include "roles.hpp"
#include "screen.hpp"
#include "screen.hpp" // FIXME
#include "tiles.hpp"
#include "trade-route.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp" // FIXME
#include "unit-flag.hpp"
#include "unit-mgr.hpp"
#include "views.hpp"
#include "visibility.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"
#include "render/typer.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

// FIXME FIXME FIXME
//
// The minimap is a huge performance drain since it is redrawn
// each frame... it causes framerate in debug mode to drop to 30
// fps just with it being enabled... this needs to be improved.
//
// FIXME FIXME FIXME

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** PanelPlane::Impl
*****************************************************************/
struct PanelPlane::Impl : public IPlane, public IMenuHandler {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  ILandViewPlane const& land_view_plane_;
  unique_ptr<ui::InvisibleView> view_;
  wait_promise<> w_promise_;
  vector<IMenuServer::Deregistrar> dereg_;
  rect mini_map_area_;

  Rect panel_rect() const {
    auto const r = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    return r
        .with_new_left_edge( r.right() - config_ui.panel.width )
        .with_new_top_edge( config_ui.menus.menu_bar_height );
  }

  Rect mini_map_available_rect() const {
    Rect mini_map_available_rect = panel_rect();
    int const kBorder            = 8;
    mini_map_available_rect.x += kBorder;
    mini_map_available_rect.y += kBorder;
    mini_map_available_rect.w -= kBorder * 2;
    // This mirrors the OG's ratio, which appears to be a good
    // one both in terms of function and appearance.
    mini_map_available_rect.h =
        static_cast<int>( mini_map_available_rect.h / 4.5 );
    return mini_map_available_rect;
  }

  Impl( IEngine& engine, SS& ss, TS& ts,
        ILandViewPlane& land_view_plane )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      land_view_plane_( land_view_plane ) {
    // Register menu handlers.
    dereg_.push_back(
        ts.planes.get().menu.typed().register_handler(
            e_menu_item::next_turn, *this ) );

    vector<ui::OwningPositionedView> view_vec;

    Rect const mini_map_available = mini_map_available_rect();
    auto mini_map                 = make_unique<MiniMapView>(
        ss, ts, land_view_plane.viewport(),
        mini_map_available.delta() );
    mini_map_area_.size = mini_map->delta();
    mini_map_area_.origin =
        centered( mini_map->delta(), mini_map_available );
    view_vec.emplace_back( ui::OwningPositionedView{
      .view  = std::move( mini_map ),
      .coord = mini_map_area_.origin.point_becomes_origin(
          panel_rect().upper_left() ) } );

    auto button_view = make_unique<ui::ButtonView>(
        engine_.textometer(), "Next Turn", [this] {
          w_promise_.set_value( {} );
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    Coord where      = centered( button_size, panel_rect() );
    where.y          = panel_height() - button_size.h * 2;
    where = Coord{} + ( where - panel_rect().upper_left() );

    ui::OwningPositionedView p_view{
      .view = std::move( button_view ), .coord = where };
    view_vec.emplace_back( std::move( p_view ) );

    view_ = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void advance_state() override { view_->advance_state(); }

  W panel_width() const { return panel_rect().w; }
  H panel_height() const { return panel_rect().h; }

  Delta delta() const override {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return panel_rect().upper_left(); };

  ui::ButtonView& next_turn_button() const {
    auto p_view = view_->at( 1 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void render_divider( rr::Renderer& renderer, point const pos,
                       int const max_width ) const {
    int const width     = max_width - 8;
    rr::Painter painter = renderer.painter();
    painter.draw_horizontal_line( pos + size{ .w = 1, .h = -1 },
                                  width + 2,
                                  config_ui.window.border_dark );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4 + 7, .h = -1 },
        width / 3 - 12, config_ui.window.border_light );
    painter.draw_horizontal_line(
        pos, width, config_ui.window.border_darker );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4 + 4 }, width / 3 - 9,
        config_ui.window.border_dark );
    painter.draw_horizontal_line(
        pos + size{ .w = 1, .h = +1 }, width + 3,
        config_ui.window.border_light );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4, .h = +1 },
        width / 3 - 5, config_ui.window.border_lighter );
  }

  void draw_some_stats( rr::Renderer& renderer,
                        Coord const where ) const {
    auto const viz = create_visibility_for(
        ss_, player_for_role( ss_, e_player_role::viewer ) );
    PanelEntities const entities =
        entities_shown_on_panel( ss_, *viz );
    render_panel_layout(
        renderer, ss_, where, *viz,
        panel_render_plan( ss_, *viz, engine_.textometer(),
                           panel_layout( ss_, *viz, entities ),
                           panel_rect().w ) );

    // Bottom.
    if( entities.player_turn.has_value() ) {
      size const padding = { .w = 8, .h = -4 };
      int const height   = 4;
      gfx::rect const curr_player_rect{
        .origin = panel_rect().to_gfx().sw() +
                  size{ .h = -height } + padding,
        .size = {
          .w = panel_rect().to_gfx().size.w - padding.w * 2,
          .h = height } };
      renderer.painter().draw_solid_rect(
          curr_player_rect,
          config_nation.players[*entities.player_turn]
              .flag_color );
    }
  }

  void draw( rr::Renderer& renderer, Coord ) const override {
    tile_sprite( renderer, e_tile::wood_middle, panel_rect() );
    // Render border on left and bottom.
    Rect const r = panel_rect();
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .75 );
      rr::Painter painter = renderer.painter();
      painter.draw_vertical_line(
          r.upper_left(), r.h, config_ui.window.border_darker );
      painter.draw_vertical_line(
          r.upper_left() + Delta{ .w = 1 }, r.h,
          config_ui.window.border_dark );
    }

    view_->draw( renderer, origin() );

    int const x_margin =
        std::max( mini_map_area_.origin.x - panel_rect().x, 0 );
    Coord const divider_p =
        mini_map_area_.sw()
            .with_x( panel_rect().x )
            .moved_down( std::min( 8, x_margin ) + 2 );
    render_divider( renderer, divider_p, panel_rect().w );

    Coord const p = divider_p + Delta{ .w = 8, .h = 6 };
    draw_some_stats( renderer, p );
  }

  bool input( input::event_t const& event ) override {
    if( event.holds<input::cheat_event_t>() ) {
      enable_cheat_mode( ss_, ts_ );
      return true;
    }

    // FIXME: we need a window manager in the panel to avoid du-
    // plicating logic between here and the window module.
    if( input::is_mouse_event( event ) ) {
      UNWRAP_CHECK( pos, input::mouse_position( event ) );
      // Normally we require that the cursor be over this view to
      // send input events, but the one exception is for drag
      // events. If we are receiving a drag event here that means
      // that we started within this view, and so (for a smooth
      // UX) we allow the subsequent drag events to be received
      // even if the cursor runs out of this view in the process.
      if( pos.is_inside( panel_rect() ) ||
          event.holds<input::mouse_drag_event_t>() ) {
        auto new_event =
            mouse_origin_moved_by( event, origin() - Coord{} );
        (void)view_->input( new_event );
        return true;
      }
      return false;
    } else
      return view_->input( event ) ? true : false;
  }

  // Override IPlane.
  e_accept_drag can_drag( input::e_mouse_button,
                          Coord origin ) override {
    if( !origin.is_inside( panel_rect() ) )
      return e_accept_drag::no;
    // We're doing this so that the minimap can handle dragging
    // without adding the special drag methods to the interface.
    // If we need to change this then we will need to fix the
    // minimap.
    return e_accept_drag::yes_but_raw;
  }

  wait<> user_hits_eot_button() {
    next_turn_button().enable( /*enabled=*/true );
    // Use a scoped setter here so that the button gets disabled
    // if this coroutine gets cancelled.
    SCOPE_EXIT {
      next_turn_button().enable( /*enabled=*/false );
    };
    w_promise_.reset();
    co_await w_promise_.wait();
  }

  bool will_handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    return next_turn_button().enabled();
  }

  void handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    w_promise_.set_value_emplace_if_not_set();
  }

  wait<> wait_for_eot_button_click() {
    return user_hits_eot_button();
  }

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {
    vector<ui::OwningPositionedView> view_vec;
    Rect const mini_map_available = mini_map_available_rect();
    auto mini_map                 = make_unique<MiniMapView>(
        ss_, ts_, land_view_plane_.viewport(),
        mini_map_available.delta() );
    Coord const mini_map_upper_left =
        centered( mini_map->delta(), mini_map_available );
    view_vec.emplace_back( ui::OwningPositionedView{
      .view  = std::move( mini_map ),
      .coord = mini_map_upper_left.with_new_origin(
          panel_rect().upper_left() ) } );

    auto button_view = make_unique<ui::ButtonView>(
        engine_.textometer(), "Next Turn", [this] {
          w_promise_.set_value( {} );
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    Coord where      = centered( button_size, panel_rect() );
    where.y          = panel_height() - button_size.h * 2;
    where = Coord{} + ( where - panel_rect().upper_left() );

    ui::OwningPositionedView p_view{
      .view = std::move( button_view ), .coord = where };
    view_vec.emplace_back( std::move( p_view ) );

    view_ = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );
  }
};

/****************************************************************
** PanelPlane
*****************************************************************/
IPlane& PanelPlane::impl() { return *impl_; }

PanelPlane::~PanelPlane() = default;

PanelPlane::PanelPlane( IEngine& engine, SS& ss, TS& ts,
                        ILandViewPlane& land_view_plane )
  : impl_( new Impl( engine, ss, ts, land_view_plane ) ) {}

wait<> PanelPlane::wait_for_eot_button_click() {
  return impl_->wait_for_eot_button_click();
}

} // namespace rn
