/****************************************************************
**difficulty-screen-2.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-03.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#include "difficulty-screen-2.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "input.hpp"
#include "interrupts.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
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
#include "gfx/resolution.rds.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

#define HANDLED( r )                          \
  case e_resolution::_##r: {                  \
    static auto const layout = kLayout_##r(); \
    layout_                  = &layout;       \
    break;                                    \
  }

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

  point scroll_origin = {};

  enum_map<e_cardinal_direction, e_difficulty> next = {};

  pixel selected_color = {};

  std::string_view description_label = {};
};

struct InstructionsCell {
  point center_for_title_label = {};

  point center_for_click_here_label = {};

  rect select_rect = {};
};

struct Layout {
  rect bg_rect = {};

  pixel const bg_color = pixel::from_hex_rgb( 0x632a10 );

  // The amount of buffer around the scroll tile to draw the se-
  // lection rectangle
  size selected_buffer = {};

  // The point, relative to the ne of the scroll, which will be
  // the center for the label text.
  size center_for_label = {};

  // Same as above but for the "(easy)", "(hard)" labels.
  size center_for_description_label = {};

  InstructionsCell instructions_cell = {};

  enum_map<e_difficulty, DifficultyCell> cells = {};
};

/****************************************************************
** Layouts.
*****************************************************************/
auto const kLayout_640x360 = [] {
  Layout l;
  l.bg_rect.size                 = { .w = 640, .h = 360 };
  l.selected_buffer              = { .w = 5, .h = 5 };
  l.center_for_label             = { .w = 63, .h = 20 };
  l.center_for_description_label = { .w = 63, .h = 146 };

  auto& instructions                  = l.instructions_cell;
  instructions.center_for_title_label = { .x = 320, .y = 65 };
  instructions.center_for_click_here_label = { .x = 320,
                                               .y = 120 };
  instructions.select_rect = { .origin = { .x = 234, .y = 28 },
                               .size = { .w = 172, .h = 123 } };

  using enum e_difficulty;
  using enum e_cardinal_direction;

  // Discoverer.
  {
    auto& cell             = l.cells[discoverer];
    cell.label             = "Discoverer";
    cell.description_label = "(easiest)";
    cell.scroll_origin     = { .x = 46, .y = 12 };
    cell.next[n]           = explorer;
    cell.next[e]           = viceroy;
    cell.next[w]           = viceroy;
    cell.next[s]           = explorer;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0x04B410 );
  }

  // Explorer.
  {
    auto& cell             = l.cells[explorer];
    cell.label             = "Explorer";
    cell.description_label = "(easy)";
    cell.scroll_origin     = { .x = 46, .y = 188 };
    cell.next[n]           = discoverer;
    cell.next[e]           = conquistador;
    cell.next[w]           = governor;
    cell.next[s]           = discoverer;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0x5555ff );
  }

  // Conquistador.
  {
    auto& cell             = l.cells[conquistador];
    cell.label             = "Conquistador";
    cell.description_label = "(moderate)";
    cell.scroll_origin     = { .x = 260, .y = 188 };
    cell.next[n]           = conquistador;
    cell.next[e]           = governor;
    cell.next[w]           = explorer;
    cell.next[s]           = conquistador;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xfffe54 );
  }

  // Governor.
  {
    auto& cell             = l.cells[governor];
    cell.label             = "Governor";
    cell.description_label = "(tough)";
    cell.scroll_origin     = { .x = 473, .y = 188 };
    cell.next[n]           = viceroy;
    cell.next[e]           = explorer;
    cell.next[w]           = conquistador;
    cell.next[s]           = viceroy;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xff7100 );
  }

  // Viceroy.
  {
    auto& cell             = l.cells[viceroy];
    cell.label             = "Viceroy";
    cell.description_label = "(toughest)";
    cell.scroll_origin     = { .x = 473, .y = 12 };
    cell.next[n]           = governor;
    cell.next[e]           = discoverer;
    cell.next[w]           = discoverer;
    cell.next[s]           = governor;
    cell.selected_color = gfx::pixel::from_hex_rgb( 0xff0000 );
  }

  return l;
};

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  IEngine& engine_;
  wait_promise<maybe<e_difficulty>> result_ = {};
  Layout const* layout_                     = {};
  e_difficulty selected_           = e_difficulty::conquistador;
  maybe<e_difficulty> highlighted_ = nothing;

 public:
  DifficultyScreen( IEngine& engine ) : engine_( engine ) {
    // TODO
  }

  void on_logical_resolution_changed(
      e_resolution const resolution ) override {
    switch( resolution ) {
      HANDLED( 640x360 );
      case e_resolution::_640x400: {
        layout_ = {};
        break;
      }
      case e_resolution::_576x360: {
        layout_ = {};
        break;
      }
      case e_resolution::_720x450: {
        layout_ = {};
        break;
      }
      case e_resolution::_768x432: {
        layout_ = {};
        break;
      }
    }
  }

  void write_centered( rr::Renderer& renderer,
                       pixel const color_fg,
                       pixel const color_bg, point const center,
                       string_view const text ) const {
    size const text_size =
        rr::rendered_text_line_size_pixels( text );
    rect const text_rect = gfx::centered_on( text_size, center );
    renderer.typer( text_rect.nw() + size{ .w = 1 }, color_bg )
        .write( text );
    renderer.typer( text_rect.nw() + size{ .h = 1 }, color_bg )
        .write( text );
    renderer.typer( text_rect.nw(), color_fg ).write( text );
  }

  void draw( rr::Renderer& renderer, Layout const& l,
             e_difficulty const difficulty,
             DifficultyCell const& cell ) const {
    bool const this_selected    = ( selected_ == difficulty );
    bool const this_highlighted = ( highlighted_ == difficulty );
    render_sprite( renderer, cell.scroll_origin,
                   e_tile::difficulty_scroll );
    auto const write = [&]( size const center,
                            string_view const text ) {
      if( this_highlighted || this_selected )
        write_centered( renderer, cell.selected_color,
                        pixel::black(),
                        cell.scroll_origin + center, text );
      else
        write_centered(
            renderer, pixel::from_hex_rgb( 0x777777 ),
            pixel::black(), cell.scroll_origin + center, text );
    };
    write( l.center_for_label, cell.label );
    write( l.center_for_description_label,
           cell.description_label );
    if( this_selected ) {
      rr::Painter painter = renderer.painter();
      static size const kScrollSize =
          sprite_size( e_tile::difficulty_scroll );
      rect const selected_rect{
        .origin = cell.scroll_origin - l.selected_buffer,
        .size   = kScrollSize + l.selected_buffer * 2 };
      rr::draw_empty_rect_no_corners(
          painter, selected_rect.with_dec_size(),
          cell.selected_color );
    }
  }

  void draw( rr::Renderer& renderer, const Layout& l ) const {
    rr::Painter painter = renderer.painter();

    // Background.
    painter.draw_solid_rect( l.bg_rect, l.bg_color );

    // Scrolls.
    for( auto const& [difficulty, cell] : l.cells )
      draw( renderer, l, difficulty, cell );

    // Instructions box.
    static gfx::pixel const kUiTextColor =
        config_ui.dialog_text.normal;
    write_centered( renderer, kUiTextColor, pixel::black(),
                    l.instructions_cell.center_for_title_label,
                    "Choose Difficulty Level" );
    write_centered(
        renderer, kUiTextColor, pixel::black(),
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
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::w];
        break;
      case ::SDLK_RIGHT:
      case ::SDLK_KP_6:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::e];
        break;
      case ::SDLK_UP:
      case ::SDLK_KP_8:
        selected_ =
            l.cells[selected_].next[e_cardinal_direction::n];
        break;
      case ::SDLK_DOWN:
      case ::SDLK_KP_2:
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

    static size const kScrollSize =
        sprite_size( e_tile::difficulty_scroll );
    highlighted_ = nothing;
    for( const auto& [difficulty, cell] : l.cells ) {
      rect const click_area{ .origin = cell.scroll_origin,
                             .size   = kScrollSize };
      if( event.pos.to_gfx().is_inside( click_area ) )
        highlighted_ = difficulty;
    }

    return e_input_handled::yes;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::no;

    if( !layout_ ) return e_input_handled::no;
    auto const& l = *layout_;

    if( event.pos.to_gfx().is_inside(
            l.instructions_cell.select_rect ) ) {
      result_.set_value( selected_ );
    } else {
      static size const kScrollSize =
          sprite_size( e_tile::difficulty_scroll );
      for( const auto& [difficulty, cell] : l.cells ) {
        rect const click_area{ .origin = cell.scroll_origin,
                               .size   = kScrollSize };
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
wait<e_difficulty> choose_difficulty_screen_2( IEngine& engine,
                                               Planes& planes ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  DifficultyScreen difficulty_screen( engine );
  group.bottom = &difficulty_screen;

  co_return co_await difficulty_screen.run();
}

} // namespace rn
