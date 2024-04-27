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

// rds
#include "rds/switch-macro.hpp"

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
  gfx::point                       selected = {};

  void check_invariants() const {
    CHECK( grid.size().area() == 6 );
    CHECK( find( grid.data().begin(), grid.data().end(),
                 nothing ) != grid.data().end(),
           "could not find blank square in grid of size {}.",
           grid.size() );
    for( e_difficulty const difficulty :
         refl::enum_values<e_difficulty> ) {
      CHECK( find( grid.data().begin(), grid.data().end(),
                   difficulty ) != grid.data().end(),
             "could not find {} in grid of size {}.", difficulty,
             grid.size() );
    }
    CHECK( grid[selected].has_value() );
  }

  void resize_grid_for_screen_size(
      const gfx::size workarea_pixels ) {
    CHECK_GT( workarea_pixels.area(), 0 );
    double const ratio =
        double( workarea_pixels.w ) / workarea_pixels.h;
    CHECK( selected.is_inside( grid.rect() ) );
    CHECK( grid[selected].has_value() );
    CHECK( grid[selected].has_value() );
    // By value so that we don't hold a reference into the grid
    // data, which will soon become dangling.
    UNWRAP_CHECK_T( e_difficulty const previous_selected,
                    grid[selected] );

    if( ratio < .5 ) {
      grid                     = Delta{ .w = 1, .h = 6 };
      grid[{ .x = 0, .y = 0 }] = nothing;
      grid[{ .x = 0, .y = 1 }] = e_difficulty::discoverer;
      grid[{ .x = 0, .y = 2 }] = e_difficulty::explorer;
      grid[{ .x = 0, .y = 3 }] = e_difficulty::conquistador;
      grid[{ .x = 0, .y = 4 }] = e_difficulty::governor;
      grid[{ .x = 0, .y = 5 }] = e_difficulty::viceroy;
    } else if( ratio < 1.0 ) {
      grid                     = Delta{ .w = 2, .h = 3 };
      grid[{ .x = 0, .y = 0 }] = e_difficulty::discoverer;
      grid[{ .x = 1, .y = 0 }] = e_difficulty::explorer;
      grid[{ .x = 0, .y = 1 }] = nothing;
      grid[{ .x = 1, .y = 1 }] = e_difficulty::conquistador;
      grid[{ .x = 0, .y = 2 }] = e_difficulty::viceroy;
      grid[{ .x = 1, .y = 2 }] = e_difficulty::governor;
    } else if( ratio < 2.0 ) {
      grid                     = Delta{ .w = 3, .h = 2 };
      grid[{ .x = 0, .y = 0 }] = e_difficulty::discoverer;
      grid[{ .x = 1, .y = 0 }] = nothing;
      grid[{ .x = 2, .y = 0 }] = e_difficulty::viceroy;
      grid[{ .x = 0, .y = 1 }] = e_difficulty::explorer;
      grid[{ .x = 1, .y = 1 }] = e_difficulty::conquistador;
      grid[{ .x = 2, .y = 1 }] = e_difficulty::governor;
    } else {
      grid                     = Delta{ .w = 6, .h = 1 };
      grid[{ .x = 0, .y = 0 }] = nothing;
      grid[{ .x = 1, .y = 0 }] = e_difficulty::discoverer;
      grid[{ .x = 2, .y = 0 }] = e_difficulty::explorer;
      grid[{ .x = 3, .y = 0 }] = e_difficulty::conquistador;
      grid[{ .x = 4, .y = 0 }] = e_difficulty::governor;
      grid[{ .x = 5, .y = 0 }] = e_difficulty::viceroy;
    }

    for( gfx::point const p :
         gfx::rect_iterator( grid.rect() ) ) {
      if( grid[p] == previous_selected ) {
        selected = p;
        break;
      }
    }
    CHECK( grid[selected] == previous_selected );

    // Sanity checks.
    check_invariants();
  }
};

gfx::pixel color_for_difficulty( e_difficulty difficulty ) {
  auto const& conf = config_nation.nations;
  // These difficulty levels don't really have anything to do
  // with nations, it just so happens that the OG reuses the na-
  // tions' flag colors here.
  switch( difficulty ) {
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

string_view description_for_difficulty(
    e_difficulty difficulty ) {
  switch( difficulty ) {
    case e_difficulty::conquistador:
      return "Moderate";
    case e_difficulty::discoverer:
      return "Easiest";
    case e_difficulty::explorer:
      return "Easy";
    case e_difficulty::governor:
      return "Tough";
    case e_difficulty::viceroy:
      return "Toughest";
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

void draw_difficulty_box_contents(
    rr::Renderer& renderer, const gfx::rect box,
    const e_difficulty difficulty ) {
  string const label = [&] {
    string_view const name = refl::enum_value_name( difficulty );
    string const capitalized = base::capitalize_initials( name );
    return fmt::format( "{}", capitalized );
  }();

  gfx::pixel const text_color =
      color_for_difficulty( difficulty );
  gfx::pixel const shadow_color = gfx::pixel::black();

  int const text_height =
      rr::rendered_text_line_size_pixels( "X" ).h;

  gfx::rect cur_box = box;
  cur_box = cur_box.with_new_top_edge( cur_box.center().y -
                                       text_height / 2 );

  auto render_line_centered = [&]( string_view line ) {
    gfx::size const text_box_size =
        rr::rendered_text_line_size_pixels( line );
    if( cur_box.size.h < text_box_size.h ) return;
    gfx::rect const text_box = gfx::rect{
        .origin = gfx::centered_at_top( text_box_size, cur_box ),
        .size   = text_box_size };
    { // shadow
      renderer
          .typer( text_box.nw() + gfx::size{ .w = 1 },
                  shadow_color )
          .write( line );
      renderer
          .typer( text_box.nw() + gfx::size{ .h = 1 },
                  shadow_color )
          .write( line );
    }
    { // foreground text.
      rr::Typer typer =
          renderer.typer( text_box.nw(), text_color );
      typer.write( line );
      typer.newline();
      // Plus 1 because we're also doing downward shadows.
      cur_box =
          cur_box.with_new_top_edge( typer.position().y + 1 );
    }
  };

  render_line_centered( label );
  render_line_centered( fmt::format(
      "({})", description_for_difficulty( difficulty ) ) );
}

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  wait_promise<maybe<e_difficulty>> result_ = {};
  DifficultyLayout                  layout_ = {};

 public:
  DifficultyScreen() {
    using namespace gfx;
    layout_.grid                     = Delta{ .w = 1, .h = 1 };
    layout_.grid[{ .x = 0, .y = 0 }] = e_difficulty::discoverer;
    recomposite();
  }

  void recomposite() {
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    layout_.resize_grid_for_screen_size( normal_area.delta() );
  }

  void advance_state() override { recomposite(); }

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
      if( square == layout_.selected ) {
        CHECK( difficulty.has_value() );
        auto color = color_for_difficulty( *difficulty );
        draw_empty_rect_no_corners( painter, inner_r, color );
      }
    }
  }

  void normalize_selected() {
    if( layout_.selected.x < 0 )
      layout_.selected.x = layout_.grid.rect().right_edge() - 1;
    if( layout_.selected.x == layout_.grid.rect().right_edge() )
      layout_.selected.x = 0;
    if( layout_.selected.y < 0 )
      layout_.selected.y = layout_.grid.rect().bottom_edge() - 1;
    if( layout_.selected.y == layout_.grid.rect().bottom_edge() )
      layout_.selected.y = 0;
  }

  template<typename F>
  void alter_selected( F&& op ) {
    do {
      op( layout_.selected );
      normalize_selected();
    } while( !layout_.grid[layout_.selected].has_value() );
  }

  e_input_handled input( input::event_t const& event ) override {
    SWITCH( event ) {
      CASE( key_event ) {
        if( key_event.change != input::e_key_change::down )
          break;
        if( input::has_mod_key( key_event ) ) break;
        switch( key_event.keycode ) {
          case ::SDLK_SPACE:
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
          case ::SDLK_KP_5:
            CHECK( layout_.grid[layout_.selected].has_value() );
            result_.set_value( layout_.grid[layout_.selected] );
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
