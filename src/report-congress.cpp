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
#include "fathers.hpp"
#include "iengine.hpp"
#include "input.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"

// config
#include "config/fathers.rds.hpp"
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

// base
#include "base/scope-exit.hpp"

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

  // Next founding father.
  string founding_father_title;
  point founding_father_text_nw;
  rect founding_father_bounds;
  TileSpreadRenderPlan founding_father_spreads;

  // Expeditionary force.
  string expeditionary_force_title;
  point expeditionary_force_text_nw;
  rect expeditionary_force_bounds;
  TileSpreadRenderPlans expeditionary_force_spreads;
};

/****************************************************************
** Auto-Layout.
*****************************************************************/
Layout layout_auto( SSConst const& ss, Player const& player,
                    rr::ITextometer const& textometer,
                    e_resolution const resolution ) {
  Layout l;
  l.canvas = { .size = resolution_size( resolution ) };

  int const margin = 7;
  point cur{ .x = margin, .y = margin };

  int const kBufferAfterTitle   = 12;
  int const kBufferAfterSection = 10;
  int const kHalfTextHeight     = 4; // TODO

  l.title = "AFFAIRS OF THE CONTINENTAL CONGRESS";
  // We need kHalfTextHeight here because the text is centered on
  // the point that we give it.
  l.title_center = { .x = l.canvas.center().x,
                     .y = cur.y + kHalfTextHeight };
  cur.y += kBufferAfterTitle + kHalfTextHeight;
  cur.y += kBufferAfterTitle;

  [&] {
    // This will be nothing if the player has all fathers, in
    // which case we won't bother showing this section.
    auto const bells_needed =
        bells_needed_for_next_father( ss, player );
    if( !bells_needed.has_value() ) return;
    e_tile const kBellsTile      = e_tile::product_bells_20;
    int const spread_tile_height = sprite_size( kBellsTile ).h;
    SCOPE_EXIT {
      cur.y += spread_tile_height;
      cur.y += kBufferAfterSection;
    };
    l.founding_father_title = fmt::format(
        "Next Continental Congress Session{}:",
        player.fathers.in_progress.has_value()
            ? fmt::format(
                  " ({})",
                  config_fathers
                      .fathers[*player.fathers.in_progress]
                      .name )
            : "" );
    l.founding_father_text_nw = cur;
    cur.y += kBufferAfterTitle;
    if( !player.fathers.in_progress.has_value() ) return;
    l.founding_father_bounds = {
      .origin = cur,
      .size   = { .w = l.canvas.left() + margin, .h = 32 } };
    // Should have been checked above; we shouldn't be rendering
    // this section if it is not true.
    CHECK( bells_needed.has_value() );
    // In the OG this is only revealed in cheat mode, but there
    // doesn't seem to be a good reason to hide this from the
    // player normally, so we'll just show it.
    l.founding_father_title += fmt::format(
        " [{}/{}]", player.fathers.bells, *bells_needed );
    ProgressTileSpreadConfig const founding_father_spread_opts{
      .tile           = kBellsTile,
      .count          = *bells_needed,
      .progress_count = player.fathers.bells,
      .label_override = nothing,
      .options =
          {
            .bounds       = l.canvas.size.w - 2 * margin,
            .label_policy = SpreadLabels::always{},
            .label_opts =
                { .placement =
                      SpreadLabelPlacement::in_first_tile{
                        .placement = e_cdirection::sw } },
          },
    };
    l.founding_father_spreads = build_progress_tile_spread(
        textometer, founding_father_spread_opts );
  }();

  [&] {
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
    int const spread_tile_height = sprite_size( regular_tile ).h;
    SCOPE_EXIT {
      cur.y += spread_tile_height;
      cur.y += kBufferAfterSection;
    };
    // Always use the "pre declaration" form here because this
    // refers to the King's army.
    l.expeditionary_force_title =
        fmt::format( "{} Expeditionary Force",
                     config_nation.nations[player.nation]
                         .possessive_pre_declaration );
    l.expeditionary_force_text_nw = cur;
    cur.y += kBufferAfterTitle;
    l.expeditionary_force_bounds = {
      .origin = cur,
      .size   = { .w = l.canvas.left() + margin, .h = 32 } };
    auto const& force = player.old_world.expeditionary_force;
    TileSpreadConfigMulti const expeditionary_force_spread_opts{
      .tiles{
        { .tile = regular_tile, .count = force.regulars },
        { .tile = cavalry_tile, .count = force.cavalry },
        { .tile = artillery_tile, .count = force.artillery },
        { .tile = man_o_war_tile, .count = force.men_of_war },
      },
      .options =
          {
            .bounds       = l.canvas.size.w - 2 * margin,
            .label_policy = SpreadLabels::always{},
            .label_opts =
                { .placement =
                      SpreadLabelPlacement::in_first_tile{
                        .placement = e_cdirection::sw } },
          },
      .group_spacing = 4,
    };
    l.expeditionary_force_spreads = build_tile_spread_multi(
        textometer, expeditionary_force_spread_opts );
  }();

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
    return layout_auto( ss_, player_, engine_.textometer(),
                        resolution );
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

  void draw( rr::Renderer& renderer, const Layout& l ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( renderer.logical_screen_rect(),
                             pixel::from_hex_rgb( 0xb86624 ) );

    write_centered( renderer, pixel::from_hex_rgb( 0xeeeeaa ),
                    /*color_bg=*/nothing, l.title_center,
                    l.title );

    // Founding father.
    renderer.typer( l.founding_father_text_nw, pixel::banana() )
        .write( l.founding_father_title );
    draw_rendered_icon_spread( renderer,
                               l.founding_father_bounds.nw(),
                               l.founding_father_spreads );

    // Expeditionary force.
    renderer
        .typer( l.expeditionary_force_text_nw, pixel::banana() )
        .write( l.expeditionary_force_title );
    draw_rendered_icon_spread( renderer,
                               l.expeditionary_force_bounds.nw(),
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
