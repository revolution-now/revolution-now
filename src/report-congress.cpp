/****************************************************************
**report-congress.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: The Continental Congress report screen.
*
*****************************************************************/
#include "report-congress.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "input.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "spread.hpp"
#include "tiles.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/pixel.hpp"
#include "gfx/resolution-enum.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Layout.
*****************************************************************/
struct Layout {
  rect canvas;

  string title;
  point title_center;

  string expeditionary_force_title;
  point expeditionary_force_text_nw;
  rect expeditionary_force;
  RenderableIconSpreads expeditionary_force_spreads;
};

/****************************************************************
** Auto-Layout.
*****************************************************************/
Layout layout_auto( Player const& player,
                    e_resolution const resolution ) {
  Layout l;
  l.canvas = { .size = resolution_size( resolution ) };

  int const margin = 7;

  l.title        = "AFFAIRS OF THE CONTINENTAL CONGRESS";
  l.title_center = { .x = l.canvas.center().x, .y = margin };

  point cur{ .x = 5, .y = 15 };

  // Always use the "pre declaration" form here because this
  // refers to the King's army.
  l.expeditionary_force_title =
      fmt::format( "{} Expeditionary Force",
                   config_nation.nations[player.nation]
                       .possessive_pre_declaration );
  l.expeditionary_force_text_nw = cur;
  cur.y += 10;
  l.expeditionary_force = {
    .origin = cur,
    .size   = { .w = l.canvas.left() - margin, .h = 32 } };

  e_tile const regular_tile =
      config_unit_type.composition
          .unit_types[e_unit_type::regular]
          .tile;
  e_tile const cavalry_tile =
      config_unit_type.composition
          .unit_types[e_unit_type::cavalry]
          .tile;
  e_tile const artillery_tile =
      config_unit_type.composition
          .unit_types[e_unit_type::artillery]
          .tile;
  e_tile const man_o_war_tile =
      config_unit_type.composition
          .unit_types[e_unit_type::man_o_war]
          .tile;
  IconSpreadSpecs const expeditionary_force_spread_specs{
    .bounds = l.canvas.size.w - 2 * margin,
    .specs =
        { { .count =
                player.old_world.expeditionary_force.regulars,
            .width = spread_width_for_tile( regular_tile ) },
          { .count =
                player.old_world.expeditionary_force.cavalry,
            .width = spread_width_for_tile( cavalry_tile ) },
          { .count =
                player.old_world.expeditionary_force.artillery,
            .width = spread_width_for_tile( artillery_tile ) },
          { .count =
                player.old_world.expeditionary_force.men_of_war,
            .width = spread_width_for_tile( man_o_war_tile ) } },
    .group_spacing = 4 };
  IconSpreads const icon_spreads =
      compute_icon_spread( expeditionary_force_spread_specs );
  l.expeditionary_force_spreads = RenderableIconSpreads{
    .spreads = icon_spreads,
    .icons   = { regular_tile, cavalry_tile, artillery_tile,
                 man_o_war_tile } };
  return l;
}

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct ContinentalCongressReport : public IPlane {
  // State
  IEngine& engine_;
  SSConst const& ss_;
  Player const& player_;
  maybe<Layout> layout_    = {};
  wait_promise<> finished_ = {};

 public:
  ContinentalCongressReport( IEngine& engine, SSConst const& ss,
                             Player const& player )
    : engine_( engine ), ss_( ss ), player_( player ) {
    if( auto const named = named_resolution( engine_ );
        named.has_value() )
      on_logical_resolution_changed( *named );
  }

  Layout layout_gen( e_resolution const resolution ) {
    return layout_auto( player_, resolution );
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
    size const text_size =
        rr::rendered_text_line_size_pixels( text );
    rect const text_rect = gfx::centered_on( text_size, center );
    if( color_bg.has_value() ) {
      renderer
          .typer( text_rect.nw() + size{ .w = 1 }, *color_bg )
          .write( text );
      renderer
          .typer( text_rect.nw() + size{ .h = 1 }, *color_bg )
          .write( text );
    }
    renderer.typer( text_rect.nw(), color_fg ).write( text );
  }

  void draw( rr::Renderer& renderer, const Layout& l ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( renderer.logical_screen_rect(),
                             pixel::from_hex_rgb( 0xb86624 ) );

    write_centered( renderer, pixel::from_hex_rgb( 0xeeeeaa ),
                    /*color_bg=*/nothing, l.title_center,
                    l.title );

    renderer
        .typer( l.expeditionary_force_text_nw, pixel::banana() )
        .write( l.expeditionary_force_title );

    render_icon_spread( renderer, l.expeditionary_force.nw(),
                        l.expeditionary_force_spreads );
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
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::no;
    finished_.set_value( monostate{} );
    return e_input_handled::yes;
  }

  wait<> run() { co_await finished_.wait(); }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> show_continental_congress_report( IEngine& engine,
                                         SSConst const& ss,
                                         Player const& player,
                                         Planes& planes ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  ContinentalCongressReport ccr( engine, ss, player );
  group.bottom = &ccr;
  co_await ccr.run();
}

} // namespace rn
