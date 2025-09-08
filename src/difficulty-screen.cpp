/****************************************************************
**difficulty-screen.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-03.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#include "difficulty-screen.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "input.hpp"
#include "interrupts.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/difficulty.rds.hpp"

// render
#include "render/extra.hpp"
#include "render/renderer.hpp"

// rds
#include "rds/switch-macro.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"
#include "gfx/resolution-enum.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_map;

/****************************************************************
** Layout.
*****************************************************************/
struct DifficultyCell {
  std::string_view label = {};

  point plate_origin = {};

  enum_map<e_cardinal_direction, e_difficulty> next = {};

  pixel selected_color = {};

  std::string_view description_label = {};

  e_tile character = {};
};

struct InstructionsCell {
  point center_for_title_label = {};

  point center_for_click_here_label = {};

  rect select_rect = {};
};

struct Layout {
  e_resolution named_resolution = {};

  rect bg_rect = {};

  // The amount of buffer around the plate tile to draw the se-
  // lection rectangle
  size selected_buffer = {};

  // The point, relative to the ne of the plate, which will be
  // the center for the label text.
  size center_for_label = {};

  // Same as above but for the "(easy)", "(hard)" labels.
  size center_for_description_label = {};

  // The point, relative to the ne of the plate, which will be
  // the nw for the character sprites (including stencil).
  size stencil_nw = {};

  InstructionsCell instructions_cell = {};

  enum_map<e_difficulty, DifficultyCell> cells = {};
};

/****************************************************************
** Auto-Layout.
*****************************************************************/
Layout layout_auto( e_resolution const resolution ) {
  Layout l;
  l.named_resolution = resolution;
  l.bg_rect.size     = resolution_size( l.named_resolution );

  l.selected_buffer              = { .w = -5, .h = -5 };
  l.center_for_label             = { .w = 61, .h = 16 };
  l.center_for_description_label = { .w = 63, .h = 145 };
  l.stencil_nw                   = { .w = 10, .h = 22 };

  using enum e_difficulty;
  using enum e_cardinal_direction;

  // Find positions of plates.
  static size const kPlateSize =
      sprite_size( e_tile::wood_plate );
  size const margins = {
    .w = ( l.bg_rect.size.w - 3 * kPlateSize.w ) / 4,
    .h = ( l.bg_rect.size.h - 2 * kPlateSize.h ) / 3 };
  int const plate_row_y0 = margins.h;
  int const plate_row_y1 = margins.h * 2 + kPlateSize.h;
  int const plate_col_x0 = margins.w;
  int const plate_col_x1 = margins.w * 2 + kPlateSize.w;
  int const plate_col_x2 = margins.w * 3 + kPlateSize.w * 2;

  auto& instructions       = l.instructions_cell;
  instructions.select_rect = {
    .origin = { .x = plate_col_x1 - margins.w / 2,
                .y = plate_row_y0 },
    .size   = { .w = kPlateSize.w + margins.w,
                .h = kPlateSize.h } };
  point const select_rect_center =
      instructions.select_rect.center();

  instructions.center_for_title_label = {
    .x = select_rect_center.x,
    .y = instructions.select_rect.top() +
         instructions.select_rect.size.h / 3 };
  instructions.center_for_click_here_label = {
    .x = select_rect_center.x,
    .y = instructions.select_rect.top() +
         2 * instructions.select_rect.size.h / 3 };

  // Discoverer.
  {
    auto& cell             = l.cells[discoverer];
    cell.label             = "Discoverer";
    cell.description_label = "Easiest";
    cell.character         = e_tile::discoverer;
    cell.next[n]           = explorer;
    cell.next[e]           = viceroy;
    cell.next[w]           = viceroy;
    cell.next[s]           = explorer;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0x04B410 );
    cell.plate_origin = { .x = plate_col_x0, .y = plate_row_y0 };
  }

  // Explorer.
  {
    auto& cell             = l.cells[explorer];
    cell.label             = "Explorer";
    cell.description_label = "Easy";
    cell.character         = e_tile::explorer;
    cell.next[n]           = discoverer;
    cell.next[e]           = conquistador;
    cell.next[w]           = governor;
    cell.next[s]           = discoverer;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0x5555ff );
    cell.plate_origin = { .x = plate_col_x0, .y = plate_row_y1 };
  }

  // Conquistador.
  {
    auto& cell             = l.cells[conquistador];
    cell.label             = "Conquistador";
    cell.description_label = "Moderate";
    cell.character         = e_tile::conquistador;
    cell.next[n]           = conquistador;
    cell.next[e]           = governor;
    cell.next[w]           = explorer;
    cell.next[s]           = conquistador;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xfffe54 );
    cell.plate_origin = { .x = plate_col_x1, .y = plate_row_y1 };
  }

  // Governor.
  {
    auto& cell             = l.cells[governor];
    cell.label             = "Governor";
    cell.description_label = "Tough";
    cell.character         = e_tile::governor;
    cell.next[n]           = viceroy;
    cell.next[e]           = explorer;
    cell.next[w]           = conquistador;
    cell.next[s]           = viceroy;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xff7100 );
    cell.plate_origin = { .x = plate_col_x2, .y = plate_row_y1 };
  }

  // Viceroy.
  {
    auto& cell             = l.cells[viceroy];
    cell.label             = "Viceroy";
    cell.description_label = "Toughest";
    cell.character         = e_tile::viceroy;
    cell.next[n]           = governor;
    cell.next[e]           = discoverer;
    cell.next[w]           = discoverer;
    cell.next[s]           = governor;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xff0000 );
    cell.plate_origin = { .x = plate_col_x2, .y = plate_row_y0 };
  }

  return l;
}

/****************************************************************
** Layouts.
*****************************************************************/
// Example for how to customize for a particular resolution.
Layout layout_576x360() {
  Layout l = layout_auto( e_resolution::_576x360 );
  // Customize.
  return l;
}

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  IEngine& engine_;
  wait_promise<maybe<e_difficulty>> result_ = {};
  maybe<Layout> layout_                     = {};
  e_difficulty selected_           = e_difficulty::conquistador;
  maybe<e_difficulty> highlighted_ = nothing;
  bool hover_finished_             = false;

 public:
  DifficultyScreen( IEngine& engine ) : engine_( engine ) {
    if( auto const named = named_resolution( engine_ );
        named.has_value() )
      on_logical_resolution_changed( *named );
    // Make the hover state consistent with the current mouse po-
    // sition while waiting for the first mouse move event.
    on_mouse_move( input::mouse_move_event_from_curr_pos() );
  }

  Layout layout_gen( e_resolution const resolution ) {
    using enum e_resolution;
    switch( resolution ) {
      case _576x360:
        // Example for how to customize per resolution.
        return layout_576x360();
      default:
        return layout_auto( resolution );
    }
  }

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    layout_ = layout_gen( resolution );
  }

  static void render_aged( rr::Renderer& renderer,
                           double const age, auto const& fn ) {
    fn();
    // This will overlay some translucent grey noise over the
    // image to give it a faded/aged/degraded look.
    SCOPED_RENDERER_MOD_SET( painter_mods.desaturate, true );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, age );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             1.0 - age );
    fn();
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             e_difficulty const difficulty,
             DifficultyCell const& cell ) const {
    bool const this_selected    = ( selected_ == difficulty );
    bool const this_highlighted = ( highlighted_ == difficulty );
    render_sprite( renderer, cell.plate_origin,
                   e_tile::wood_plate );

    // Character.
    auto draw_character = [&] {
      render_sprite_stencil( renderer,
                             cell.plate_origin + l.stencil_nw,
                             e_tile::plate_stencil,
                             cell.character, pixel::black() );
    };
    if( this_selected )
      render_aged( renderer, /*age=*/0.0, draw_character );
    else
      render_aged( renderer, /*age=*/0.5, draw_character );
    auto const write = [&]( size const center,
                            string_view const text ) {
      if( this_highlighted || this_selected )
        rr::write_centered( renderer, cell.selected_color,
                            pixel::black(),
                            cell.plate_origin + center, text );
    };
    auto const write_text = [&] {
      write( l.center_for_label, cell.label );
      write( l.center_for_description_label,
             cell.description_label );
    };
    write_text();
    if( this_selected ) {
      static size const kPlateSize =
          sprite_size( e_tile::wood_plate );
      rect const selected_rect{
        .origin = cell.plate_origin - l.selected_buffer,
        .size   = kPlateSize + l.selected_buffer * 2 };
      rr::draw_empty_rect_faded_corners(
          renderer, selected_rect.with_dec_size(),
          cell.selected_color );
    }
  }

  void draw( rr::Renderer& renderer, const Layout& l ) const {
    // Background.
    tile_sprite( renderer, e_tile::wood_middle, l.bg_rect );

    // plates.
    for( auto const& [difficulty, cell] : l.cells )
      draw( renderer, l, difficulty, cell );

    // Instructions box.
    static gfx::pixel const kUiTextColor =
        config_ui.dialog_text.normal;
    rr::write_centered(
        renderer, kUiTextColor, pixel::black(),
        l.instructions_cell.center_for_title_label,
        "Choose Difficulty Level" );
    rr::write_centered(
        renderer,
        hover_finished_ ? kUiTextColor.highlighted( 3 )
                        : kUiTextColor,
        pixel::black(),
        l.instructions_cell.center_for_click_here_label,
        "(Click here when finished)" );
  }

  void draw( rr::Renderer& renderer ) const override {
    if( !layout_ ) return;
    draw( renderer, *layout_ );
  }

  e_input_handled on_key(
      input::key_event_t const& event ) override {
    if( event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( input::has_mod_key( event ) ) return e_input_handled::no;
    if( !layout_ ) return e_input_handled::no;
    auto const& l = *layout_;
    auto handled  = e_input_handled::no;
    switch( event.keycode ) {
      case ::SDLK_SPACE:
      case ::SDLK_RETURN:
      case ::SDLK_KP_ENTER:
      case ::SDLK_KP_5:
        result_.set_value( selected_ );
        break;
      case ::SDLK_ESCAPE:
        result_.set_value( nothing );
        break;
      case ::SDLK_LEFT:
      case ::SDLK_KP_4:
      case ::SDLK_h:
      case ::SDLK_j:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::w];
        break;
      case ::SDLK_RIGHT:
      case ::SDLK_KP_6:
      case ::SDLK_l:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::e];
        break;
      case ::SDLK_UP:
      case ::SDLK_KP_8:
      case ::SDLK_i:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::n];
        break;
      case ::SDLK_DOWN:
      case ::SDLK_KP_2:
      case ::SDLK_COMMA:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::s];
        break;
    }
    return handled;
  }

  e_input_handled on_mouse_move(
      input::mouse_move_event_t const& event ) override {
    if( !layout_ ) return e_input_handled::no;
    auto const& l = *layout_;

    static size const kPlateSize =
        sprite_size( e_tile::wood_plate );
    highlighted_ = nothing;
    for( const auto& [difficulty, cell] : l.cells ) {
      rect const click_area{ .origin = cell.plate_origin,
                             .size   = kPlateSize };
      if( event.pos.to_gfx().is_inside( click_area ) )
        highlighted_ = difficulty;
    }

    hover_finished_ =
        event.pos.is_inside( l.instructions_cell.select_rect );

    return e_input_handled::yes;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    if( event.buttons != input::e_mouse_button_event::left_down )
      return e_input_handled::no;

    if( !layout_ ) return e_input_handled::no;
    auto const& l = *layout_;

    if( event.pos.to_gfx().is_inside(
            l.instructions_cell.select_rect ) ) {
      result_.set_value( selected_ );
    } else {
      static size const kPlateSize =
          sprite_size( e_tile::wood_plate );
      for( const auto& [difficulty, cell] : l.cells ) {
        rect const click_area{ .origin = cell.plate_origin,
                               .size   = kPlateSize };
        if( event.pos.to_gfx().is_inside( click_area ) )
          selected_ = difficulty;
      }
    }

    return e_input_handled::yes;
  }

  wait<e_difficulty> run() {
    auto const difficulty = co_await result_.wait();
    if( !difficulty.has_value() ) throw main_menu_interrupt{};
    lg.info( "selected difficulty level: {}", *difficulty );
    co_return *difficulty;
  }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<e_difficulty> choose_difficulty_screen( IEngine& engine,
                                             Planes& planes ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  DifficultyScreen difficulty_screen( engine );
  group.bottom = &difficulty_screen;

  co_return co_await difficulty_screen.run();
}

} // namespace rn
