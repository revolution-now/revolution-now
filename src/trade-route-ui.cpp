/****************************************************************
**trade-route-ui.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Shows the Trade Route editing UI screen.
*
*****************************************************************/
#include "trade-route-ui.hpp"

// rds
#include "trade-route-ui-impl.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "input.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "trade-route.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/trade-route.rds.hpp"

// render
#include "render/extra.hpp"
#include "render/painter.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"
#include "gfx/resolution-enum.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/string.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_values;
using ::rn::trade_gui::Hover;
using ::rn::trade_gui::Input;
using ::rn::trade_gui::RouteSelection;

enum class e_button_state {
  none,
  hover,
  clicking,
};

bool constexpr DEBUG_RECTS = false;

// TODO: Move this out since it is a general utility.
void text_cutoff_dots( rr::ITextometer const& textometer,
                       rr::TextLayout const& text_layout,
                       int const max_pixel_width,
                       string_view const suffix,
                       string_view const fallback,
                       string& text ) {
  auto const width_good = [&]( string const& candidate ) {
    return textometer
               .dimensions_for_line( text_layout, candidate )
               .w <= max_pixel_width;
  };

  if( width_good( text ) ) return;

  while( !text.empty() ) {
    text.pop_back();
    string candidate = format( "{}{}", text, suffix );
    if( width_good( candidate ) ) {
      text = std::move( candidate );
      return;
    }
  }

  text = string( fallback );
}

string name_for_target( SSConst const& ss, Player const& player,
                        TradeRouteTarget const& target ) {
  string name;
  SWITCH( target ) {
    CASE( colony ) {
      name = ss.colonies.colony_for( colony.colony_id ).name;
      break;
    }
    CASE( harbor ) {
      name =
          config_nation.nations[player.nation].harbor_city_name;
      break;
    }
  }
  return name;
}

string name_for_target_with_cutoff(
    SSConst const& ss, Player const& player,
    TradeRouteTarget const& target,
    rr::ITextometer const& textometer,
    rr::TextLayout const& text_layout,
    int const max_pixel_width ) {
  string name = name_for_target( ss, player, target );
  text_cutoff_dots( textometer, text_layout, max_pixel_width,
                    /*suffix=*/"...", /*fallback=*/"-", name );
  return name;
}

/****************************************************************
** Layout.
*****************************************************************/
struct LayoutString {
  string text;
  point nw;
};

struct LayoutHeader {
  rect r;

  LayoutString destination;
  LayoutString unload;
  LayoutString load;

  rect destination_r;
  rect unload_r;
  rect load_r;
};

struct LayoutCommodity {
  int index        = {};
  e_commodity type = {};
  rect r;
};

struct LayoutIcons {
  point icons_nw;
  rect bounds;
  TileSpreadRenderPlan render_plan;
  vector<LayoutCommodity> rects;
  point add_icon_nw;
};

struct LayoutStop {
  int index = {};

  rect r;

  rect destination;
  rect unload;
  rect load;

  // Contents of the cells.
  LayoutString destination_text;
  point destination_ui_icon_nw;

  LayoutIcons icons_unload;
  LayoutIcons icons_load;
};

struct LayoutInfo {
  rect r;

  LayoutString route_name_label;
  LayoutString route_name;
  rect route_name_change_click_area;
  point name_change_icon_nw;

  LayoutString route_type_label;
  LayoutString route_type;
  rect route_type_change_click_area;
};

struct LayoutButton {
  rect r;
  string label;
};

struct Layout {
  e_resolution named_resolution = {};

  rect canvas;

  rr::TextLayout text_layout;

  int margin = {};

  pixel fg = pixel::from_hex_rgb( 0xeeeeff );

  string title;
  point title_center;

  LayoutInfo info;

  LayoutHeader header;

  array<LayoutStop, 4> stops;

  LayoutButton ok_button;
  LayoutButton cxl_button;
  pixel button_color      = pixel::from_hex_rgb( 0x24347c );
  pixel button_text_color = pixel::from_hex_rgb( 0x88a3d1 );
};

/****************************************************************
** Auto-Layout.
*****************************************************************/
Layout layout_auto( IEngine& engine, SSConst const& ss,
                    Player const& player,
                    e_resolution const resolution,
                    TradeRoute const& route,
                    rr::ITextometer const& textometer ) {
  Layout l;

  l.text_layout = {
    .monospace    = false,
    .spacing      = 1,
    .line_spacing = 1,
  };

  l.named_resolution = resolution;
  l.canvas           = { .size = resolution_size( resolution ) };

  int const margin = 7;
  l.margin         = margin;
  point cur{ .x = margin, .y = margin };
  int const kBufferAfterTitle = 15;
  int const kLineHeight =
      textometer.font_height() +
      textometer.spacing_between_lines( l.text_layout );
  int const kHalfLineHeight = kLineHeight / 2;
  int const kCanvasWidthWithMargins =
      l.canvas.size.w - 2 * margin;
  size const kIconSize = kCommodityTileLargeSize;

  l.title        = format( "EDIT TRADE ROUTE {}", route.id );
  l.title_center = { .x = l.canvas.center().x,
                     .y = cur.y + kHalfLineHeight };

  cur.y += kBufferAfterTitle * 2;

  auto& info                 = l.info;
  info.route_name_label.text = "Route Name: ";
  info.route_name_label.nw   = cur;
  // Need two lines here so that we have room for the "change"
  // icon that appears next to the route name when hovering the
  // mouse over it.
  cur.y += kLineHeight;
  cur.y += kLineHeight;
  info.route_type_label.text = "Route Type: ";
  info.route_type_label.nw   = cur;

  // This is the spacing between the end of the cell contents
  // (either text or commodities) and the placement of the little
  // UI icon representing add/change.
  int const ui_icon_spacing = 3;

  int const info_offset = std::max(
      textometer
          .dimensions_for_line( l.text_layout,
                                info.route_name_label.text )
          .w,
      textometer
          .dimensions_for_line( l.text_layout,
                                info.route_type_label.text )
          .w );
  info.route_name = {
    .text = route.name,
    .nw = info.route_name_label.nw.moved_right( info_offset ) };
  info.name_change_icon_nw =
      info.route_name_label.nw.moved_down( kHalfLineHeight )
          .moved_right( info_offset )
          .moved_right( textometer
                            .dimensions_for_line( l.text_layout,
                                                  route.name )
                            .w )
          .moved_right( ui_icon_spacing )
          .moved_up( 5 );
  info.route_name_change_click_area = {
    .origin = info.route_name_label.nw,
    .size   = { .w = kCanvasWidthWithMargins / 2,
                .h = kLineHeight } };
  info.route_type = {
    .text =
        base::capitalize_initials( base::to_str( route.type ) ),
    .nw = info.route_type_label.nw.moved_right( info_offset ) };
  info.route_type_change_click_area = {
    .origin = info.route_type_label.nw,
    .size   = { .w = kCanvasWidthWithMargins / 2,
                .h = kLineHeight } };

  l.info.r = { .origin = info.route_name_label.nw,
               .size   = { .w = kCanvasWidthWithMargins,
                           .h = kLineHeight * 2 } };

  cur.y += kLineHeight;

  int const stops_total_height = l.canvas.size.h / 3;
  int const stop_height        = stops_total_height / 4;

  int const column_width = ( l.canvas.size.w - 2 * margin ) / 3;
  size const cell_sz = { .w = column_width, .h = stop_height };
  int const cell_inner_width = column_width - 2 * margin;

  int const num_prefix_width =
      engine.textometer()
          .dimensions_for_line( l.text_layout, "8. " )
          .w;

  // Header.
  cur.y =
      l.canvas.size.h / 2 - stops_total_height / 2 - stop_height;
  l.header.destination.text = "Destination";
  l.header.unload.text      = "Unload Cargo";
  l.header.load.text        = "Load Cargo";
  l.header.destination_r    = { .origin = cur, .size = cell_sz };
  l.header.unload_r         = {
            .origin = cur.moved_right( column_width ), .size = cell_sz };
  l.header.load_r = {
    .origin = cur.moved_right( column_width * 2 ),
    .size   = cell_sz };
  l.header.destination.nw =
      l.header.destination_r.origin.moved_right( margin )
          .moved_down( cell_sz.h / 2 - kHalfLineHeight )
          .moved_right( num_prefix_width );
  l.header.unload.nw =
      l.header.unload_r.origin.moved_right( margin ).moved_down(
          cell_sz.h / 2 - kHalfLineHeight );
  l.header.load.nw =
      l.header.load_r.origin.moved_right( margin ).moved_down(
          cell_sz.h / 2 - kHalfLineHeight );
  l.header.r = {
    .origin = l.header.destination_r.origin,
    .size   = { .w = column_width * 3, .h = stop_height } };

  cur.y += stop_height;
  for( int i = 0; i < 4; ++i ) {
    SCOPE_EXIT { cur.y += stop_height; };
    auto& stop       = l.stops[i];
    stop.index       = i;
    stop.r           = { .origin = cur,
                         .size   = { .w = kCanvasWidthWithMargins,
                                     .h = stop_height } };
    stop.destination = { .origin = cur, .size = cell_sz };
    stop.unload = { .origin = cur.moved_right( column_width ),
                    .size   = cell_sz };
    stop.load = { .origin = cur.moved_right( column_width * 2 ),
                  .size   = cell_sz };
    stop.destination_text = {
      .text = "", // filled in below.
      .nw   = stop.destination.point_at( e_direction::w )
                .moved_right( margin )
                .moved_up( kHalfLineHeight )
                .moved_down( 1 ) };
    point const ui_icon_nw_min =
        gfx::centered_at_left(
            sprite_size( e_tile::circle_for_change ),
            stop.destination )
            .moved_right( l.margin )
            .moved_down();
    point const ui_icon_nw_max =
        gfx::centered_at_right(
            sprite_size( e_tile::circle_for_change ),
            stop.destination )
            .moved_left( l.margin )
            .moved_down();
    // This may get overridden below if there is a stop here.
    stop.destination_ui_icon_nw = ui_icon_nw_min;
    if( i >= ssize( route.stops ) ) continue;
    TradeRouteStop const& route_stop = route.stops.at( i );

    stop.icons_unload.icons_nw =
        stop.unload.point_at( e_direction::w )
            .moved_right( margin )
            .moved_up( kIconSize.h / 2 )
            .moved_down( 1 );
    stop.icons_load.icons_nw =
        stop.load.point_at( e_direction::w )
            .moved_right( margin )
            .moved_up( kIconSize.h / 2 )
            .moved_down( 1 );

    stop.destination_text.text =
        format( "{}. {}", stop.index + 1,
                name_for_target_with_cutoff(
                    ss, player, route_stop.target,
                    engine.textometer(), l.text_layout,
                    cell_inner_width - num_prefix_width ) );
    stop.destination_ui_icon_nw =
        stop.destination_ui_icon_nw
            .with_x( stop.destination_text.nw.x )
            .moved_right( textometer
                              .dimensions_for_line(
                                  l.text_layout,
                                  stop.destination_text.text )
                              .w )
            .moved_right( ui_icon_spacing );
    stop.destination_ui_icon_nw.x =
        clamp( stop.destination_ui_icon_nw.x, ui_icon_nw_min.x,
               ui_icon_nw_max.x );

    // Unload icons.
    auto const make_spread = [&]( vector<e_commodity> const&
                                      comms,
                                  rect const cell_r,
                                  LayoutIcons& layout_icons ) {
      vector<TileWithOptions> tiles;
      tiles.reserve( comms.size() );
      for( int index = 0; e_commodity const type : comms ) {
        tiles.push_back( TileWithOptions{
          .tile = tile_for_commodity_20( type ) } );
        layout_icons.rects.push_back( LayoutCommodity{
          .index = index++,
          .type  = type,
          .r     = {} // filled out below
        } );
      }
      InhomogeneousTileSpreadConfig const spread_config{
        .tiles       = std::move( tiles ),
        .max_spacing = 3,
        .options     = { .bounds = cell_inner_width },
        .sort_tiles  = false };
      layout_icons.render_plan = build_inhomogeneous_tile_spread(
          engine.textometer(), spread_config );
      if( !comms.empty() )
        layout_icons.bounds =
            layout_icons.render_plan.bounds
                .origin_becomes_point( layout_icons.icons_nw )
                // Make these rects extend from the top to the
                // bottom of the cell and to the left edge so
                // that mouse hover behavior is less eratic.
                .with_new_top_edge( stop.r.top() )
                .with_new_bottom_edge( stop.r.bottom() )
                .with_new_left_edge( cell_r.left() );
      point const add_icon_nw_x_min =
          gfx::centered_at_left(
              sprite_size( e_tile::circle_for_change ), cell_r )
              .moved_right( l.margin )
              .moved_down();
      point const add_icon_nw_x_max =
          gfx::centered_at_right(
              sprite_size( e_tile::circle_for_change ), cell_r )
              .moved_left( l.margin )
              .moved_down();
      layout_icons.add_icon_nw = add_icon_nw_x_min;
      if( !comms.empty() )
        layout_icons.add_icon_nw.x =
            layout_icons.bounds.right() + ui_icon_spacing;
      layout_icons.add_icon_nw.x =
          clamp( layout_icons.add_icon_nw.x, add_icon_nw_x_min.x,
                 add_icon_nw_x_max.x );
      // Just in case we ended up rendering fewer than we re-
      // quested due to lack of space. zip will handle that
      // case but we don't want to leave extra rects on the
      // end that don't correspond to anything rendered.
      CHECK_GE( layout_icons.rects.size(),
                layout_icons.render_plan.tiles.size() );
      layout_icons.rects.resize(
          layout_icons.render_plan.tiles.size() );
      for( auto const [comm_layout, tile_plan] : rv::zip(
               layout_icons.rects,
               as_const( layout_icons.render_plan.tiles ) ) ) {
        rect const trimmed = trimmed_area_for( tile_plan.tile );
        comm_layout.r =
            rect{
              .origin =
                  trimmed.origin_becomes_point( tile_plan.where )
                      .nw(),
              .size = trimmed.size }
                .origin_becomes_point( layout_icons.icons_nw )
                // Make these rects extend from the top to the
                // bottom of the cell so that mouse hover be-
                // havior is less eratic.
                .with_new_top_edge( stop.r.top() )
                .with_new_bottom_edge( stop.r.bottom() );
      }
    };

    make_spread( route_stop.unloads, stop.unload,
                 stop.icons_unload );
    make_spread( route_stop.loads, stop.load, stop.icons_load );
  }

  size const button_sz{ .w = 64, .h = 16 };
  int const buttons_top =
      l.canvas.bottom() - margin - button_sz.h;
  point const ok_nw = l.canvas.point_at( e_cdirection::s )
                          .with_y( buttons_top )
                          .moved_left( margin / 2 )
                          .moved_left( button_sz.w );
  point const cxl_nw = l.canvas.point_at( e_cdirection::s )
                           .with_y( buttons_top )
                           .moved_right( margin / 2 );

  l.ok_button = LayoutButton{
    .r     = { .origin = ok_nw, .size = button_sz },
    .label = "OK",
  };
  l.cxl_button = LayoutButton{
    .r     = { .origin = cxl_nw, .size = button_sz },
    .label = "Cancel",
  };
  return l;
}

/****************************************************************
** Layouts.
*****************************************************************/
// Example for how to customize for a particular resolution.
Layout layout_576x360( IEngine& engine, SSConst const& ss,
                       Player const& player,
                       TradeRoute const& route,
                       rr::ITextometer const& textometer ) {
  Layout l =
      layout_auto( engine, ss, player, e_resolution::_576x360,
                   route, textometer );
  // Customize.
  return l;
}

/****************************************************************
** TradeRouteUI
*****************************************************************/
struct TradeRouteUI : public IPlane {
  // Doing private by default allows the compiler to tell us
  // which methods aren't used.

 private:
  // State
  IEngine& engine_;
  SSConst const& ss_;
  Player const& player_;
  IGui& gui_;
  TerrainConnectivity const& connectivity_;
  TradeRouteState& trade_route_state_modify_only_on_save_;
  TradeRouteState trade_route_state_;
  maybe<Layout> layout_    = {};
  wait_promise<> finished_ = {};
  co::stream<Input> in_;
  TradeRouteId curr_id_ = {};
  maybe<Hover> mouse_hover_;
  // Keep this instead of getting the real value otherwise we
  // react to mouse clicks on other planes.
  input::mouse_buttons_state_t mouse_buttons_state_;

 public:
  TradeRouteUI( IEngine& engine, SSConst const& ss,
                Player const& player, IGui& gui,
                TerrainConnectivity const& connectivity,
                TradeRouteState& trade_route_state_real,
                TradeRouteState const& trade_route_state_to_edit,
                TradeRouteId const initial_id )
    : engine_( engine ),
      ss_( ss ),
      player_( player ),
      gui_( gui ),
      connectivity_( connectivity ),
      trade_route_state_modify_only_on_save_(
          trade_route_state_real ),
      trade_route_state_( trade_route_state_to_edit ),
      curr_id_( initial_id ) {
    if( auto const named = named_resolution( engine_ );
        named.has_value() )
      on_logical_resolution_changed( *named );
    mouse_hover_ =
        mouse_hover( input::current_mouse_position() );
  }

  // Doing private by default allows the compiler to tell us
  // which methods aren't used.

 private:
  TradeRoute& curr_trade_route() {
    auto const it = trade_route_state_.routes.find( curr_id_ );
    CHECK( it != trade_route_state_.routes.end() );
    return it->second;
  }

  TradeRoute const& curr_trade_route() const {
    auto const it = trade_route_state_.routes.find( curr_id_ );
    CHECK( it != trade_route_state_.routes.end() );
    return it->second;
  }

  [[nodiscard]] bool has_stop( int const idx ) const {
    return idx >= 0 && idx < ssize( curr_trade_route().stops );
  }

  Layout layout_gen( e_resolution const resolution ) {
    using enum e_resolution;
    TradeRoute const& route = curr_trade_route();
    switch( resolution ) {
      case _576x360:
        // Example for how to customize per resolution.
        return layout_576x360( engine_, ss_, player_, route,
                               engine_.textometer() );
      default:
        return layout_auto( engine_, ss_, player_, resolution,
                            route, engine_.textometer() );
    }
  }

  void update_layout(
      maybe<e_resolution> resolution = nothing ) {
    // NOTE: Recomputing mouse hover in this method is important
    // to avoid crashes because we don't want it to be left re-
    // ferring to some entity that has been deleted.
    mouse_hover_.reset();
    if( !resolution.has_value() )
      resolution = layout_.member( &Layout::named_resolution );
    if( !resolution.has_value() )
      // There is no resolution available at which to compute the
      // layout.
      return;
    layout_ = layout_gen( *resolution );
    mouse_hover_ =
        mouse_hover( input::current_mouse_position() );
  }

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    update_layout( resolution );
  }

  void write_centered( rr::Renderer& renderer,
                       pixel const color_fg,
                       maybe<pixel> const color_bg,
                       point const center,
                       string_view const text ) const {
    rr::Typer typer      = renderer.typer();
    size const text_size = typer.dimensions_for_line( text );
    rect const text_rect = gfx::centered_on( text_size, center );
    if( color_bg.has_value() ) {
      typer.set_color( *color_bg );
      typer.set_position( text_rect.nw() + size{ .w = 1 } );
      typer.write( text );
      typer.set_position( text_rect.nw() + size{ .h = 1 } );
      typer.write( text );
    }
    typer.set_color( color_fg );
    typer.set_position( text_rect.nw() );
    typer.write( text );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutString const& str ) const {
    rr::Typer typer = renderer.typer( str.nw, l.fg );

    typer.write( "{}", str.text );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutInfo const& info ) const {
    draw( renderer, l, info.route_name_label );
    draw( renderer, l, info.route_name );
    draw( renderer, l, info.route_type_label );
    draw( renderer, l, info.route_type );

    if( mouse_hover_.get_if<Hover::name>() )
      render_sprite( renderer, info.name_change_icon_nw,
                     e_tile::circle_for_change );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutHeader const& header ) const {
    draw( renderer, l, header.destination );
    draw( renderer, l, header.unload );
    draw( renderer, l, header.load );
  }

  // This gets run whether or not the stop exists.
  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutStop const& layout_stop ) const {
    auto const draw_rect = [&]( rect const r ) {
      draw_empty_rect_faded_corners( renderer, r, l.fg );
    };

    draw_rect( layout_stop.destination );
    draw_rect( layout_stop.unload );
    draw_rect( layout_stop.load );

    if( auto const destination =
            mouse_hover_.get_if<Hover::destination>();
        destination.has_value() &&
        destination->stop == layout_stop.index ) {
      e_tile const tile = has_stop( destination->stop )
                              ? e_tile::circle_for_change
                              : e_tile::plus_for_add;
      render_sprite( renderer,
                     layout_stop.destination_ui_icon_nw, tile );
    }
  }

  // This is for the load/unload cells when the stop exists.
  template<typename HoverT>
  void draw( rr::Renderer& renderer, Layout const&,
             LayoutStop const& layout_stop,
             LayoutIcons const& layout_icons ) const {
    draw_rendered_icon_spread( renderer, layout_icons.icons_nw,
                               layout_icons.render_plan );
    auto const stop_hover = mouse_hover_.get_if<HoverT>();
    if( !stop_hover.has_value() ) return;

    int const stop = stop_hover->stop;
    if( stop != layout_stop.index ) return;

    bool const tiles = stop_hover->tiles;
    if constexpr( DEBUG_RECTS ) {
      if( tiles )
        renderer.painter().draw_empty_rect(
            layout_icons.bounds.moved_down()
                .moved_right()
                .with_dec_size(),
            rr::Painter::e_border_mode::inside, pixel::green() );
    }

    auto const tile = stop_hover->tile;
    if( tile.has_value() ) {
      rect const r = layout_icons.rects[*tile].r;
      if constexpr( DEBUG_RECTS ) {
        renderer.painter().draw_empty_rect(
            r.moved_down().with_dec_size( size{ .h = 1 } ),
            rr::Painter::e_border_mode::inside, pixel::red() );
      }
      // For the y coordinate to be consistent for all tiles oth-
      // erwise the y position of the X icon will vary since the
      // icons are of different sizes and positions.
      point p = gfx::centered_in(
                    sprite_size( e_tile::x_for_remove ), r )
                    .with_y( layout_icons.add_icon_nw.y );
      // Make sure that id never goes to the right of the + icon.
      p.x = std::min( p.x, layout_icons.add_icon_nw.x );
      render_sprite( renderer, p, e_tile::x_for_remove );
    } else if( !tiles ) {
      render_sprite( renderer, layout_icons.add_icon_nw,
                     e_tile::plus_for_add );
    }
  }

  // This is for when the stop exists.
  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutStop const& layout_stop,
             TradeRouteStop const& ) const {
    // Destination cell.
    rr::Typer typer =
        renderer.typer( layout_stop.destination_text.nw, l.fg );
    typer.write( "{}", layout_stop.destination_text.text );

    draw<Hover::unloads>( renderer, l, layout_stop,
                          layout_stop.icons_unload );
    draw<Hover::loads>( renderer, l, layout_stop,
                        layout_stop.icons_load );
  }

  void draw_button( rr::Renderer& renderer, rect const r,
                    pixel const button_color,
                    pixel const text_color, string const& label,
                    e_button_state const state ) const {
    using enum e_button_state;
    rr::Painter P = renderer.painter();
    size const click_shift =
        ( state >= clicking ) ? size{ .w = -1, .h = 1 } : size{};
    // This draws the body in a way that it will appear to move
    // up and to the right when pressed.
    rect const r_body = r.moved_right().moved( click_shift );
    draw_rect_noisy_filled( renderer, r_body, button_color );
    P.draw_solid_rect( r_body, button_color.with_alpha( 128 ) );
    pixel const shaded    = button_color.shaded( 2 );
    pixel const highlight = button_color.highlighted( 2 );
    if( state >= clicking ) {
      P.draw_vertical_line( r.ne(), r.size.h + 1, shaded );
      P.draw_horizontal_line( r.nw(), r.size.w, shaded );
    } else {
      P.draw_vertical_line( r.ne(), r.size.h + 1, highlight );
      P.draw_horizontal_line( r.nw(), r.size.w, highlight );
      P.draw_vertical_line( r.nw(), r.size.h, shaded );
      P.draw_horizontal_line( r.sw(), r.size.w, shaded );
      P.draw_point( r.nw(), highlight.with_alpha( 128 ) );
      P.draw_point( r.se(), shaded.with_alpha( 128 ) );
    }

    pixel shaded_text_color = text_color;
    if( state == hover ) // don't highlight on click.
      shaded_text_color = pixel::white();
    write_centered( renderer, shaded_text_color,
                    /*color_bg=*/nothing,
                    r.center() + click_shift, label );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutButton const& layout_button,
             e_button_state const state ) const {
    draw_button( renderer, layout_button.r, l.button_color,
                 l.button_text_color, layout_button.label,
                 state );
  }

  template<typename HoverT>
  [[nodiscard]] e_button_state get_button_state() const {
    using enum e_button_state;
    if( !mouse_hover_.get_if<HoverT>().has_value() ) return none;
    if( mouse_buttons_state_.l_down ) return clicking;
    return hover;
  }

  void draw( rr::Renderer& renderer, Layout const& l ) const {
    TradeRoute const& route = curr_trade_route();
    draw_rect_noisy_filled( renderer, l.canvas,
                            pixel::from_hex_rgb( 0x555588 ) );

    write_centered( renderer, l.fg,
                    /*color_bg=*/nothing, l.title_center,
                    l.title );

    draw( renderer, l, l.info );
    draw( renderer, l, l.header );

    for( auto const [layout_stop, route_stop] :
         rv::zip( l.stops, route.stops ) )
      draw( renderer, l, layout_stop, route_stop );
    for( LayoutStop const& stop : l.stops )
      draw( renderer, l, stop );

    draw( renderer, l, l.ok_button,
          get_button_state<Hover::button_ok>() );
    draw( renderer, l, l.cxl_button,
          get_button_state<Hover::button_cxl>() );
  }

  void draw( rr::Renderer& renderer ) const override {
    if( !layout_ ) return;
    draw( renderer, *layout_ );
  }

  // This allows you to do e.g.:
  //
  //   send_input<Input::load_add>() = { .stop = 2 };
  //
  template<typename T>
  auto send_input() {
    struct sender {
      sender( co::stream<Input>& in ) : in_( in ) {}
      ~sender() { in_.send( Input{ std::move( o ) } ); }

      void operator=( T&& in ) { o = std::move( in ); }

     private:
      co::stream<Input>& in_;
      T o{};
    };
    return sender( in_ );
  }

  void close_screen() {
    finished_.set_value_if_not_set( monostate{} );
  }

  e_input_handled on_key(
      input::key_event_t const& event ) override {
    if( event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( input::is_mod_key( event ) ) return e_input_handled::no;

    switch( event.keycode ) {
      case ::SDLK_ESCAPE:
        send_input<Input::cancel>() = {};
        break;
      case ::SDLK_RETURN:
      case ::SDLK_KP_ENTER:
        send_input<Input::done>() = {};
        break;
    }

    return e_input_handled::yes;
  }

  maybe<Hover> mouse_hover( point const p ) const {
    if( !layout_.has_value() ) return nothing;
    Layout const& l = *layout_;

    if( p.is_inside( l.cxl_button.r ) )
      return Hover::button_cxl{};

    if( p.is_inside( l.ok_button.r ) ) return Hover::button_ok{};

    if( p.is_inside( l.info.route_name_change_click_area ) )
      return Hover::name{};

    if( p.is_inside( l.info.route_type_change_click_area ) )
      return Hover::type{};

    for( LayoutStop const& stop : l.stops ) {
      if( !p.is_inside( stop.r ) ) continue;
      if( p.is_inside( stop.destination ) )
        return Hover::destination{ .stop = stop.index };
      if( p.is_inside( stop.unload ) ) {
        maybe<int> tile;
        for( LayoutCommodity const& comm :
             rv::reverse( stop.icons_unload.rects ) ) {
          if( !p.is_inside( comm.r ) ) continue;
          tile = comm.index;
          break;
        }
        return Hover::unloads{
          .stop  = stop.index,
          .tiles = p.is_inside( stop.icons_unload.bounds ),
          .tile  = tile };
      }
      if( p.is_inside( stop.load ) ) {
        maybe<int> tile;
        for( LayoutCommodity const& comm :
             rv::reverse( stop.icons_load.rects ) ) {
          if( !p.is_inside( comm.r ) ) continue;
          tile = comm.index;
          break;
        }
        return Hover::loads{
          .stop  = stop.index,
          .tiles = p.is_inside( stop.icons_load.bounds ),
          .tile  = tile };
      }
    }

    return nothing;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    mouse_buttons_state_ = event.mouse_buttons_state;
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::yes;
    if( !layout_.has_value() ) return e_input_handled::yes;
    point const p = event.pos;

    auto const hover = mouse_hover( p );
    if( !hover.has_value() ) return e_input_handled::yes;

    SWITCH( *hover ) {
      CASE( button_cxl ) {
        // Require button up here since if we all the down click
        // to close this screen then the up click will go to
        // whatever the next screen is.
        send_input<Input::cancel>() = {};
        break;
      }
      CASE( button_ok ) {
        // Require button up here since if we all the down click
        // to close this screen then the up click will go to
        // whatever the next screen is.
        send_input<Input::done>() = {};
        break;
      }
      CASE( name ) {
        send_input<Input::change_name>() = {};
        break;
      }
      CASE( type ) {
        // NOTE: we don't currently allow changing the type of
        // the trade route since that would require a bunch of
        // validation of the stops.
        break;
      }
      CASE( destination ) {
        if( has_stop( destination.stop ) )
          send_input<Input::destination_change>() = {
            .stop = destination.stop };
        else
          send_input<Input::destination_add>();
        break;
      }
      CASE( unloads ) {
        if( !has_stop( unloads.stop ) ) break;
        if( unloads.tile.has_value() ) {
          send_input<Input::unload_remove>() = {
            .stop = unloads.stop, .cargo = *unloads.tile };
          break;
        }
        if( !unloads.tiles )
          send_input<Input::unload_add>() = { .stop =
                                                  unloads.stop };
        break;
      }
      CASE( loads ) {
        if( !has_stop( loads.stop ) ) break;
        if( loads.tile.has_value() ) {
          send_input<Input::load_remove>() = {
            .stop = loads.stop, .cargo = *loads.tile };
          break;
        }
        if( !loads.tiles )
          send_input<Input::load_add>() = { .stop = loads.stop };
        break;
      }
    }

    return e_input_handled::yes;
  }

  e_input_handled on_mouse_move(
      input::mouse_move_event_t const& event ) override {
    mouse_hover_ = mouse_hover( event.pos );
    return e_input_handled::yes;
  }

  IPlane::e_accept_drag can_drag(
      input::e_mouse_button /*button*/,
      Coord /*origin*/ ) override {
    return IPlane::e_accept_drag::motion;
  }

  wait<maybe<RouteSelection>> ask_target(
      vector<TradeRouteTarget> const& targets, string const& msg,
      bool const show_delete ) const {
    if( targets.empty() ) co_return nothing;
    ChoiceConfig config{
      .msg = msg,
    };
    maybe<int> show_delete_idx;
    int idx = 0;
    for( TradeRouteTarget const& target : targets )
      config.options.push_back( ChoiceConfigOption{
        .key = to_string( idx++ ),
        .display_name =
            name_for_target( ss_, player_, target ) } );
    if( show_delete ) {
      show_delete_idx = idx++;
      config.options.push_back( ChoiceConfigOption{
        .key          = to_string( *show_delete_idx ),
        .display_name = "(Delete Stop)" } );
    }
    auto const choice =
        co_await gui_.optional_choice_int_key( config );
    if( !choice.has_value() ) co_return nothing;
    if( choice == show_delete_idx )
      co_return RouteSelection::remove{};
    CHECK_LT( *choice, ssize( targets ) );
    co_return RouteSelection::target{ .value =
                                          targets[*choice] };
  }

  wait<maybe<TradeRouteTarget>> ask_target_no_delete(
      vector<TradeRouteTarget> const& targets,
      string const& msg ) const {
    auto route_selection = co_await ask_target(
        targets, msg, /*show_delete=*/false );
    if( !route_selection.has_value() ) co_return nothing;
    UNWRAP_CHECK_T(
        TradeRouteTarget & target,
        route_selection->inner_if<RouteSelection::target>() );
    co_return target;
  }

  wait<> input_processor() {
    TradeRoute& route = curr_trade_route();
    while( true ) {
      Input const in = co_await in_.next();
      SWITCH( in ) {
        CASE( done ) {
          trade_route_state_modify_only_on_save_ =
              trade_route_state_;
          close_screen();
          break;
        }
        CASE( cancel ) {
          if( trade_route_state_modify_only_on_save_ !=
              trade_route_state_ ) {
            YesNoConfig const config{
              .msg            = "Discard Unsaved Changes?",
              .yes_label      = "Yes, discard and leave.",
              .no_label       = "No, stay here",
              .no_comes_first = true,
            };
            if( co_await gui_.optional_yes_no( config ) !=
                ui::e_confirm::yes )
              break;
          }
          close_screen();
          break;
        }
        CASE( change_name ) {
          auto const new_name =
              co_await gui_.optional_string_input(
                  StringInputConfig{
                    .msg = "Enter new name for trade route:",
                    .initial_text = route.name } );
          if( new_name.value_or( "" ).empty() ) break;
          route.name = *new_name;
          update_layout();
          break;
        }
        CASE( destination_remove ) {
          int const num_stops = ssize( route.stops );
          if( num_stops == 1 )
            // Must always have at least one stop.
            break;
          CHECK_LT( destination_remove.stop, num_stops );
          TradeRouteTarget const& target =
              route.stops[destination_remove.stop].target;
          YesNoConfig const config{
            .msg = format(
                "Are you sure you want to delete stop [{}]?",
                name_for_target( ss_, player_, target ) ),
            .yes_label      = "Delete",
            .no_label       = "Cancel",
            .no_comes_first = true,
          };
          if( co_await gui_.optional_yes_no( config ) !=
              ui::e_confirm::yes )
            break;
          route.stops.erase( route.stops.begin() +
                             destination_remove.stop );
          update_layout();
          break;
        }
        CASE( destination_change ) {
          CHECK_LT( destination_change.stop,
                    ssize( route.stops ) );
          vector<ColonyId> const colonies =
              available_colonies_for_route(
                  ss_, player_, connectivity_, route );
          vector<TradeRouteTarget> targets;
          targets.reserve( colonies.size() + 1 );
          if( route.type == e_trade_route_type::sea )
            targets.push_back( TradeRouteTarget::harbor{} );
          for( ColonyId const colony_id : colonies )
            targets.push_back( TradeRouteTarget::colony{
              .colony_id = colony_id } );
          int const num_stops    = ssize( route.stops );
          bool const show_delete = num_stops > 1;
          auto const selection   = co_await ask_target(
              targets, "Select New Destination:", show_delete );
          if( !selection.has_value() ) break;
          SWITCH( *selection ) {
            CASE( target ) {
              // Only change the target, that way we keep all the
              // load/unload settings.
              route.stops[destination_change.stop].target =
                  target.value;
              update_layout();
              break;
            }
            CASE( remove ) {
              send_input<Input::destination_remove>() = {
                .stop = destination_change.stop };
              break;
            }
          }
          break;
        }
        CASE( destination_add ) {
          vector<ColonyId> const colonies =
              available_colonies_for_route(
                  ss_, player_, connectivity_, route );
          vector<TradeRouteTarget> targets;
          targets.reserve( colonies.size() + 1 );
          if( route.type == e_trade_route_type::sea )
            targets.push_back( TradeRouteTarget::harbor{} );
          for( ColonyId const colony_id : colonies )
            targets.push_back( TradeRouteTarget::colony{
              .colony_id = colony_id } );
          auto const target = co_await ask_target_no_delete(
              targets, "Select New Destination:" );
          if( !target.has_value() ) break;
          route.stops.push_back(
              TradeRouteStop{ .target = *target } );
          update_layout();
          break;
        }
        CASE( load_add ) {
          CHECK_LT( load_add.stop, ssize( route.stops ) );
          TradeRouteStop& stop = route.stops[load_add.stop];
          EnumChoiceConfig config{
            .msg = format(
                "Select cargo to load at [{}]:",
                name_for_target( ss_, player_, stop.target ) ),
          };
          auto const choice =
              co_await gui_.optional_enum_choice<e_commodity>(
                  config );
          if( !choice.has_value() ) break;
          stop.loads.push_back( *choice );
          update_layout();
          break;
        }
        CASE( unload_add ) {
          CHECK_LT( unload_add.stop, ssize( route.stops ) );
          TradeRouteStop& stop = route.stops[unload_add.stop];
          EnumChoiceConfig config{
            .msg = format(
                "Select cargo to unload at [{}]:",
                name_for_target( ss_, player_, stop.target ) ),
          };
          auto const choice =
              co_await gui_.optional_enum_choice<e_commodity>(
                  config );
          if( !choice.has_value() ) break;
          stop.unloads.push_back( *choice );
          update_layout();
          break;
        }
        CASE( load_remove ) {
          CHECK_LT( load_remove.stop, ssize( route.stops ) );
          TradeRouteStop& stop = route.stops[load_remove.stop];
          int const cargo_idx  = load_remove.cargo;
          CHECK_LT( cargo_idx, ssize( stop.loads ) );
          stop.loads.erase( stop.loads.begin() + cargo_idx );
          update_layout();
          break;
        }
        CASE( unload_remove ) {
          CHECK_LT( unload_remove.stop, ssize( route.stops ) );
          TradeRouteStop& stop = route.stops[unload_remove.stop];
          int const cargo_idx  = unload_remove.cargo;
          CHECK_LT( cargo_idx, ssize( stop.unloads ) );
          stop.unloads.erase( stop.unloads.begin() + cargo_idx );
          update_layout();
          break;
        }
      }
    }
  }

 public:
  wait<> run() {
    auto const _ = input_processor();
    co_await finished_.wait();
  }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> show_trade_route_edit_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, TerrainConnectivity const& connectivity,
    Planes& planes, TradeRouteState& trade_route_state,
    TradeRouteId const trade_route_id ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  TradeRouteUI trade_route_ui(
      engine, ss, player, gui, connectivity, trade_route_state,
      trade_route_state, trade_route_id );
  group.bottom = &trade_route_ui;
  co_await trade_route_ui.run();
}

wait<> show_trade_route_create_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, TerrainConnectivity const& connectivity,
    Planes& planes, TradeRouteState& trade_route_state,
    TradeRoute const& new_trade_route ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  CHECK_GT( new_trade_route.id, 0 );
  CHECK(
      !trade_route_state.routes.contains( new_trade_route.id ) );
  TradeRouteState with_new_added            = trade_route_state;
  with_new_added.routes[new_trade_route.id] = new_trade_route;
  TradeRouteUI trade_route_ui(
      engine, ss, player, gui, connectivity, trade_route_state,
      with_new_added, new_trade_route.id );
  group.bottom = &trade_route_ui;
  co_await trade_route_ui.run();
}

} // namespace rn
