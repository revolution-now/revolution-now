/****************************************************************
**difficulty-screen.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-21.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#include "difficulty-screen.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "compositor.hpp"
#include "input.hpp"
#include "interrupts.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "query-enum.hpp"
#include "tiles.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/difficulty.rds.hpp"

// render
#include "render/painter.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// gfx
#include "gfx/iter.hpp"
#include "gfx/matrix.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::NoDiscard;

struct DifficultyLayout {
  gfx::Matrix<maybe<e_difficulty>> grid;
};

gfx::pixel color_for_difficulty( e_difficulty d ) {
  auto const& conf = config_nation.nations;
  // These colors don't really have anything to do with nations,
  // it just so happens that the OG reuses the nations' flag
  // colors here.
  switch( d ) {
    case e_difficulty::conquistador: {
      return conf[e_nation::spanish].flag_color;
    }
    case e_difficulty::discoverer: {
      static auto const green =
          gfx::pixel::parse_from_hex( "04B410" ).value();
      return green;
    }
    case e_difficulty::explorer: {
      return conf[e_nation::french].flag_color;
    }
    case e_difficulty::governor: {
      return conf[e_nation::dutch].flag_color;
    }
    case e_difficulty::viceroy: {
      return conf[e_nation::english].flag_color;
    }
  }
}

void draw_empty_rect_no_corners( rr::Painter&     painter,
                                 const gfx::rect  box,
                                 const gfx::pixel color ) {
  using namespace gfx;
  using ::gfx::size;
  // Left.
  {
    point const start = box.nw();
    point const end   = box.sw();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Right.
  {
    point const start = box.ne();
    point const end   = box.se();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Top.
  {
    point const start = box.nw();
    point const end   = box.ne();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
  // Bottom.
  {
    point const start = box.sw();
    point const end   = box.se();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
}

void draw_difficulty_box_contents( rr::Renderer&      renderer,
                                   const gfx::rect    box,
                                   const e_difficulty d ) {
  string const label = [&] {
    string_view const name   = refl::enum_value_name( d );
    string const capitalized = base::capitalize_initials( name );
    return fmt::format( "<{}>", capitalized );
  }();
  gfx::size const text_box_size =
      rr::rendered_text_line_size_pixels( label );
  gfx::rect const text_box = gfx::rect{
      .origin = gfx::centered_in( text_box_size, box ),
      .size   = text_box_size };
  rr::Typer typer =
      renderer.typer( text_box.nw(), color_for_difficulty( d ) );
  typer.write( label );
}

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  wait_promise<maybe<e_difficulty>> result_   = {};
  DifficultyLayout                  layout_   = {};
  gfx::point                        selected_ = {};

 public:
  DifficultyScreen() {
    using namespace gfx;
    auto& grid = layout_.grid;
    grid       = Delta{ .w = 3, .h = 2 };

    grid[{ .x = 0, .y = 0 }] = e_difficulty::discoverer;
    grid[{ .x = 1, .y = 0 }] = nothing;
    grid[{ .x = 2, .y = 0 }] = e_difficulty::viceroy;
    grid[{ .x = 0, .y = 1 }] = e_difficulty::explorer;
    grid[{ .x = 1, .y = 1 }] = e_difficulty::conquistador;
    grid[{ .x = 2, .y = 1 }] = e_difficulty::governor;
  }

  void draw( rr::Renderer& renderer ) const override {
    using namespace gfx;
    using size = gfx::size;
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    rr::Painter painter = renderer.painter();

    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .7 );
      tile_sprite( painter, e_tile::wood_middle, normal_area );
    }

    auto const& grid = layout_.grid;
    size const  subrect_size =
        ( normal_area / grid.size() ).delta();
    for( point const square : rect_iterator( grid.rect() ) ) {
      auto const p = square * subrect_size;
      rect const r{ .origin = p, .size = subrect_size };
      rect const inner_r = r.with_edges_removed( 4 );
      maybe<e_difficulty> const difficulty = grid[square];
      if( difficulty.has_value() )
        draw_difficulty_box_contents( renderer, inner_r,
                                      *difficulty );
      draw_empty_rect_no_corners( painter, inner_r,
                                  pixel::black() );
      if( square == selected_ ) {
        CHECK( difficulty.has_value() );
        auto color = color_for_difficulty( *difficulty );
        draw_empty_rect_no_corners( painter, inner_r, color );
      }
    }
  }

  void normalize_selected() {
    if( selected_.x < 0 )
      selected_.x = layout_.grid.rect().right_edge() - 1;
    if( selected_.x == layout_.grid.rect().right_edge() )
      selected_.x = 0;
    if( selected_.y < 0 )
      selected_.y = layout_.grid.rect().bottom_edge() - 1;
    if( selected_.y == layout_.grid.rect().bottom_edge() )
      selected_.y = 0;
  }

  template<typename F>
  void alter_selected( F&& op ) {
    do {
      op( selected_ );
      normalize_selected();
    } while( !layout_.grid[selected_].has_value() );
  }

  e_input_handled input( input::event_t const& event ) override {
    switch( event.to_enum() ) {
      using enum input::e_input_event;
      case key_event: {
        auto const& key = event.get<input::key_event_t>();
        if( key.change != input::e_key_change::down ) break;
        if( input::has_mod_key( key ) ) break;
        switch( key.keycode ) {
          case ::SDLK_SPACE:
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
          case ::SDLK_KP_5:
            CHECK( layout_.grid[selected_].has_value() );
            result_.set_value( layout_.grid[selected_] );
            break;
          case ::SDLK_ESCAPE:
            result_.set_value( nothing );
            break;
          case ::SDLK_LEFT:
          case ::SDLK_KP_4:
            alter_selected( []( auto& sel ) { --sel.x; } );
            break;
          case ::SDLK_RIGHT:
          case ::SDLK_KP_6:
            alter_selected( []( auto& sel ) { ++sel.x; } );
            break;
          case ::SDLK_UP:
          case ::SDLK_KP_8:
            alter_selected( []( auto& sel ) { --sel.y; } );
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            alter_selected( []( auto& sel ) { ++sel.y; } );
            break;
        }
        break;
      }
      default:
        break;
    }
    return e_input_handled::yes;
  }

  wait<e_difficulty> run() {
    auto const difficulty = co_await result_.wait();
    if( !difficulty.has_value() ) throw main_menu_interrupt{};
    co_return *difficulty;
  }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<NoDiscard<e_difficulty>> choose_difficulty_screen(
    Planes& planes ) {
  auto        owner = planes.push();
  PlaneGroup& group = owner.group;

  DifficultyScreen difficulty_screen;
  group.bottom = &difficulty_screen;

  e_difficulty const difficulty =
      co_await difficulty_screen.run();

  lg.info( "selected difficulty level: {}", difficulty );

  co_return difficulty;
}

} // namespace rn
