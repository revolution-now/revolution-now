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
#include "input.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"

// config
#include "config/nation.rds.hpp"

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

string text_cutoff_dots( rr::ITextometer const& textometer,
                         rr::TextLayout const& text_layout,
                         int const max_pixel_width,
                         string_view const dots,
                         string_view const fallback,
                         string& text ) {
  auto const width_good = [&]( string const& candidate ) {
    return textometer
               .dimensions_for_line( text_layout, candidate )
               .w <= max_pixel_width;
  };

  if( width_good( text ) ) return text;

  while( !text.empty() ) {
    text.pop_back();
    string const candidate = format( "{}{}", text, dots );
    if( width_good( candidate ) ) return candidate;
  }

  return string( fallback );
}

string name_for_target( SSConst const& ss, Player const& player,
                        TradeRouteTarget const& target,
                        rr::ITextometer const& textometer,
                        rr::TextLayout const& text_layout,
                        int const max_pixel_width ) {
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
  text_cutoff_dots( textometer, text_layout, max_pixel_width,
                    /*fallback=*/"...", /*dots=*/"...", name );
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

struct LayoutStop {
  rect r;

  rect destination;
  rect unload;
  rect load;

  LayoutString destination_text;
  point unload_icons_nw;
  point load_icons_nw;

  TileSpreadRenderPlan unload_plan;
  TileSpreadRenderPlan load_plan;
};

struct LayoutInfo {
  rect r;

  LayoutString route_name_label;
  LayoutString route_name;

  LayoutString route_type_label;
  LayoutString route_type;
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

  l.title        = "EDIT TRADE ROUTE";
  l.title_center = { .x = l.canvas.center().x,
                     .y = cur.y + kHalfLineHeight };

  cur.y += kBufferAfterTitle * 2;

  auto& info                 = l.info;
  info.route_name_label.text = "Route Name: ";
  info.route_name_label.nw   = cur;
  cur.y += kLineHeight;
  info.route_type_label.text = "Route Type: ";
  info.route_type_label.nw   = cur;

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
  info.route_type = {
    .text =
        base::capitalize_initials( base::to_str( route.type ) ),
    .nw = info.route_type_label.nw.moved_right( info_offset ) };

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
    auto& stop       = l.stops[i];
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
    stop.unload_icons_nw = stop.unload.point_at( e_direction::w )
                               .moved_right( margin )
                               .moved_up( kIconSize.h / 2 )
                               .moved_down( 1 );
    stop.load_icons_nw = stop.load.point_at( e_direction::w )
                             .moved_right( margin )
                             .moved_up( kIconSize.h / 2 )
                             .moved_down( 1 );

    if( i >= ssize( route.stops ) ) continue;
    TradeRouteStop const& route_stop = route.stops.at( i );

    stop.destination_text.text = name_for_target(
        ss, player, route_stop.target, engine.textometer(),
        l.text_layout, cell_inner_width - num_prefix_width );

    // Unload icons.
    auto const make_spread =
        [&]( vector<e_commodity> const& comms,
             TileSpreadRenderPlan& plan ) {
          vector<TileWithOptions> tiles;
          tiles.reserve( comms.size() );
          for( e_commodity const type : comms )
            tiles.push_back( TileWithOptions{
              .tile = tile_for_commodity_20( type ) } );
          InhomogeneousTileSpreadConfig const spread_config{
            .tiles       = std::move( tiles ),
            .max_spacing = 3,
            .options     = { .bounds = cell_inner_width },
            .sort_tiles  = false };
          plan = build_inhomogeneous_tile_spread(
              engine.textometer(), spread_config );
        };

    make_spread( route_stop.unloads, stop.unload_plan );
    make_spread( route_stop.loads, stop.load_plan );

    cur.y += stop_height;
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
  TradeRouteState& trade_route_state_;
  maybe<Layout> layout_    = {};
  wait_promise<> finished_ = {};
  co::stream<TradeRouteInput> in_;
  TradeRouteId curr_id_ = {};

 public:
  TradeRouteUI( IEngine& engine, SSConst const& ss,
                Player const& player,
                TradeRouteState& trade_route_state,
                TradeRouteId const initial_id )
    : engine_( engine ),
      ss_( ss ),
      player_( player ),
      trade_route_state_( trade_route_state ),
      curr_id_( initial_id ) {
    if( auto const named = named_resolution( engine_ );
        named.has_value() )
      on_logical_resolution_changed( *named );
  }

  // Doing private by default allows the compiler to tell us
  // which methods aren't used.

 private:
  TradeRoute& curr_trade_route() {
    CHECK( trade_route_state_.routes.contains( curr_id_ ) );
    return trade_route_state_.routes[curr_id_];
  }

  TradeRoute const& curr_trade_route() const {
    CHECK( trade_route_state_.routes.contains( curr_id_ ) );
    return trade_route_state_.routes[curr_id_];
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

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    layout_ = layout_gen( resolution );
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
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutHeader const& header ) const {
    draw( renderer, l, header.destination );
    draw( renderer, l, header.unload );
    draw( renderer, l, header.load );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutStop const& stop ) const {
    auto const draw_rect = [&]( rect const r ) {
      draw_empty_rect_faded_corners( renderer, r, l.fg );
    };

    draw_rect( stop.destination );
    draw_rect( stop.unload );
    draw_rect( stop.load );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             int const idx, LayoutStop const& layout_stop,
             TradeRouteStop const& ) const {
    // Destination cell.
    string const destination_label = format(
        "{}. {}", idx + 1, layout_stop.destination_text.text );
    rr::Typer typer =
        renderer.typer( layout_stop.destination_text.nw, l.fg );
    typer.write( "{}", destination_label );

    draw_rendered_icon_spread( renderer,
                               layout_stop.unload_icons_nw,
                               layout_stop.unload_plan );
    draw_rendered_icon_spread( renderer,
                               layout_stop.load_icons_nw,
                               layout_stop.load_plan );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             LayoutButton const& layout_button ) const {
    draw_empty_rect_faded_corners( renderer, layout_button.r,
                                   l.fg );
    write_centered( renderer, l.fg,
                    /*color_bg=*/nothing,
                    layout_button.r.center(),
                    layout_button.label );
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

    for( LayoutStop const& stop : l.stops )
      draw( renderer, l, stop );
    for( auto const [idx, layout_stop, route_stop] :
         rv::zip( rv::iota( 0 ), l.stops, route.stops ) )
      draw( renderer, l, idx, layout_stop, route_stop );

    draw( renderer, l, l.ok_button );
    draw( renderer, l, l.cxl_button );
  }

  void draw( rr::Renderer& renderer ) const override {
    if( !layout_ ) return;
    draw( renderer, *layout_ );
  }

  e_input_handled on_key(
      input::key_event_t const& event ) override {
    if( event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( input::is_mod_key( event ) ) return e_input_handled::no;
    finished_.set_value( monostate{} );
    return e_input_handled::yes;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    // Need to use left_up here otherwise the down click will
    // close this screen and then the up click will get picked up
    // by the land-view which requires using the up click in
    // order to distinguish clicks from drags.
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::no;
    finished_.set_value( monostate{} );
    return e_input_handled::yes;
  }

 public:
  wait<> run() { co_await finished_.wait(); }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> show_trade_route_edit_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui&, Planes& planes, TradeRouteState& trade_route_state,
    TradeRouteId const trade_route_id ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  TradeRouteUI trade_route_ui(
      engine, ss, player, trade_route_state, trade_route_id );
  group.bottom = &trade_route_ui;
  co_await trade_route_ui.run();
}

} // namespace rn
