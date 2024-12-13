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
#include "config/ui.rds.hpp"

// ss
#include "ss/difficulty.rds.hpp"

// render
#include "render/extra.hpp"
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
using namespace gfx;

namespace rn {

namespace {

struct DifficultyLayout {
  Matrix<maybe<e_difficulty>> grid;
  point selected = {};

  e_difficulty selected_difficulty() const {
    CHECK( grid[selected].has_value() );
    return *grid[selected];
  }

  maybe<point> find_difficulty(
      e_difficulty const difficulty ) const {
    for( auto const p : rect_iterator( grid.rect() ) ) {
      auto const& cell = grid[p];
      if( !cell.has_value() ) continue;
      if( *cell == difficulty ) return p;
    }
    return nothing;
  }

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

struct CenteredTyper {
  CenteredTyper( rr::Renderer& renderer, gfx::pixel color,
                 gfx::rect box )
    : renderer_( renderer ), color_( color ), cur_box_( box ) {}

  void render_line_centered( string_view line ) {
    gfx::size const text_box_size =
        rr::rendered_text_line_size_pixels( line );
    if( cur_box_.size.h < text_box_size.h ) return;
    gfx::rect const text_box = gfx::rect{
      .origin = gfx::centered_at_top( text_box_size, cur_box_ ),
      .size   = text_box_size };
    gfx::pixel const shadow_color = gfx::pixel::black();
    { // shadow
      renderer_
          .typer( text_box.nw() + gfx::size{ .w = 1 },
                  shadow_color )
          .write( line );
      renderer_
          .typer( text_box.nw() + gfx::size{ .h = 1 },
                  shadow_color )
          .write( line );
    }
    { // foreground text.
      rr::Typer typer = renderer_.typer( text_box.nw(), color_ );
      typer.write( line );
      typer.newline();
      // Plus 2 because we're also doing downward shadows.
      cur_box_ =
          cur_box_.with_new_top_edge( typer.position().y + 2 );
    }
  };

 private:
  rr::Renderer& renderer_;
  gfx::pixel color_  = {};
  gfx::rect cur_box_ = {};
};

void draw_difficulty_box_contents(
    rr::Renderer& renderer, gfx::rect const box,
    e_difficulty const difficulty ) {
  string const label = [&] {
    string_view const name = refl::enum_value_name( difficulty );
    string const capitalized = base::capitalize_initials( name );
    return fmt::format( "{}", capitalized );
  }();

  gfx::pixel const text_color =
      color_for_difficulty( difficulty );
  int const text_height =
      rr::rendered_text_line_size_pixels( "X" ).h;

  gfx::rect cur_box = box;
  cur_box = cur_box.with_new_top_edge( cur_box.center().y -
                                       text_height / 2 );

  CenteredTyper ctyper( renderer, text_color, cur_box );

  ctyper.render_line_centered( label );
  ctyper.render_line_centered( fmt::format(
      "({})", description_for_difficulty( difficulty ) ) );
}

void draw_info_box_contents( rr::Renderer& renderer,
                             gfx::rect const box ) {
  gfx::pixel const text_color = config_ui.dialog_text.normal;
  int const text_height =
      rr::rendered_text_line_size_pixels( "X" ).h;

  gfx::rect cur_box = box;
  cur_box = cur_box.with_new_top_edge( cur_box.center().y -
                                       text_height / 2 );

  CenteredTyper ctyper( renderer, text_color, cur_box );

  ctyper.render_line_centered( "Choose Difficulty Level" );
  ctyper.render_line_centered( "" );
  ctyper.render_line_centered( "" );
  ctyper.render_line_centered( "(click here when finished)" );
}

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  wait_promise<maybe<e_difficulty>> result_ = {};
  DifficultyLayout layout_                  = {};

  int const kBorderWidth  = 6;
  int const kPaddingWidth = 4;

 public:
  DifficultyScreen() {
    using namespace gfx;
    layout_.grid                     = Delta{ .w = 1, .h = 1 };
    layout_.grid[{ .x = 0, .y = 0 }] = e_difficulty::discoverer;
    recomposite();
    UNWRAP_CHECK_T(
        point const conquistador,
        layout_.find_difficulty( e_difficulty::conquistador ) );
    layout_.selected = conquistador;
  }

  void recomposite() {
    UNWRAP_RETURN_VOID_T(
        auto const normal_area,
        compositor::section( compositor::e_section::normal ) );
    if( normal_area.area() == 0 ) return;
    layout_.resize_grid_for_screen_size( normal_area.delta() );
  }

  void on_logical_resolution_changed( e_resolution ) override {}

  void advance_state() override { recomposite(); }

  gfx::size get_subrect_size() const {
    auto const normal_area =
        compositor::section( compositor::e_section::normal )
            .value_or( {} );
    return ( normal_area / layout_.grid.size() )
        .delta()
        .to_gfx();
  }

  rect outer_square_for_block( point block ) const {
    gfx::size const subrect_size = get_subrect_size();
    auto const p                 = block * subrect_size;
    rect const outer_r{ .origin = p, .size = subrect_size };
    return outer_r;
  }

  rect inner_square_for_block( point block ) const {
    rect const outer_r = outer_square_for_block( block );
    rect const inner_r = outer_r.with_edges_removed(
        kPaddingWidth + kBorderWidth );
    return inner_r;
  }

  maybe<point> clicked_grid_square( point const p ) const {
    UNWRAP_RETURN(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    if( normal_area.area() == 0 ) return nothing;
    gfx::size const subrect_size = get_subrect_size();
    point const block            = p / subrect_size;
    if( !block.is_inside( layout_.grid.rect() ) ) return nothing;
    rect const inner_r = inner_square_for_block( block );
    if( !p.is_inside( inner_r ) ) return nothing;
    return block;
  }

  void draw( rr::Renderer& renderer ) const override {
    UNWRAP_RETURN_VOID_T(
        auto const normal_area,
        compositor::section( compositor::e_section::normal ) );
    if( normal_area.area() == 0 ) return;
    rr::Painter painter = renderer.painter();

    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .7 );
      tile_sprite( renderer, e_tile::wood_middle, normal_area );
    }

    auto const& grid = layout_.grid;
    for( point const block : rect_iterator( grid.rect() ) ) {
      rect const outer_r = outer_square_for_block( block );
      rect const inner_r = inner_square_for_block( block );

      maybe<e_difficulty> const difficulty = grid[block];

      if( !difficulty.has_value() ) {
        draw_info_box_contents( renderer, outer_r );
        continue;
      }

      // Draw shadow gradient inside box.
      {
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 )
        SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                                 0 )

        double const gradient_slope =
            1.0 / ( inner_r.size.pythagorean() / 3 );
        {
          SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .3 )
          SCOPED_RENDERER_MOD_SET(
              painter_mods.depixelate.stage_gradient,
              dsize{ .w = -gradient_slope,
                     .h = gradient_slope } );
          rect const ne_quadrant =
              rect::from( inner_r.center(), inner_r.ne() );
          SCOPED_RENDERER_MOD_SET(
              painter_mods.depixelate.stage_anchor,
              ne_quadrant.center().to_double() );
          rr::Painter painter = renderer.painter();
          painter.draw_solid_rect( inner_r, pixel::black() );
        }
        {
          SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .1 )
          SCOPED_RENDERER_MOD_SET(
              painter_mods.depixelate.stage_gradient,
              dsize{ .w = gradient_slope,
                     .h = -gradient_slope } );
          rect const sw_quadrant =
              rect::from( inner_r.sw(), inner_r.center() );
          SCOPED_RENDERER_MOD_SET(
              painter_mods.depixelate.stage_anchor,
              sw_quadrant.center().to_double() );
          rr::Painter painter = renderer.painter();
          painter.draw_solid_rect( inner_r, pixel::banana() );
        }

        painter.draw_solid_rect(
            inner_r, pixel::black().with_alpha( 200 ) );
      }

      // Draw border.
      {
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .55 )

        for( int i = 0; i < kBorderWidth; ++i ) {
          double const percent =
              1.0 - double( i + 1 ) / kBorderWidth;
          SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, percent )
          rr::Painter painter = renderer.painter();

          // top bar.
          painter.draw_horizontal_line(
              { .x = inner_r.left() - i,
                .y = inner_r.top() - i - 1 },
              inner_r.size.w + 2 * i, gfx::pixel::black() );

          // right bar.
          painter.draw_vertical_line(
              { .x = inner_r.right() + i,
                .y = inner_r.top() - i - 1 },
              inner_r.size.h + 2 * i + 1, gfx::pixel::black() );

          // bottom bar.
          painter.draw_horizontal_line(
              { .x = inner_r.left() - i - 1,
                .y = inner_r.bottom() + i },
              inner_r.size.w + 2 * i + 1,
              gfx::pixel::banana().with_alpha( 127 ) );

          // left bar.
          painter.draw_vertical_line(
              { .x = inner_r.left() - i - 1,
                .y = inner_r.top() - i },
              inner_r.size.h + 2 * i,
              gfx::pixel::banana().with_alpha( 127 ) );
        }
      }

      CHECK( difficulty.has_value() );
      draw_difficulty_box_contents( renderer, inner_r,
                                    *difficulty );
      if( block == layout_.selected ) {
        CHECK( difficulty.has_value() );
        auto color        = color_for_difficulty( *difficulty );
        int const padding = 2;
        rr::draw_empty_rect_no_corners(
            painter,
            inner_r.with_edges_removed( padding )
                .with_dec_size(),
            color );
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
      CASE( mouse_button_event ) {
        if( mouse_button_event.buttons !=
            input::e_mouse_button_event::left_up )
          break;
        maybe<point> const grid_square =
            clicked_grid_square( mouse_button_event.pos );
        if( !grid_square.has_value() )
          // This means that they clicked on the border some-
          // where.
          break;
        CHECK( grid_square->is_inside( layout_.grid.rect() ) );
        maybe<e_difficulty> const difficulty =
            layout_.grid[*grid_square];
        if( !difficulty.has_value() ) {
          // This means that they clicked on the info square, so
          // they are finished.
          result_.set_value( layout_.selected_difficulty() );
          break;
        }
        // They clicked on a difficulty inner square, so select
        // it.
        layout_.selected = *grid_square;
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
wait<e_difficulty> choose_difficulty_screen( Planes& planes ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  DifficultyScreen difficulty_screen;
  group.bottom = &difficulty_screen;

  e_difficulty const difficulty =
      co_await difficulty_screen.run();

  lg.info( "selected difficulty level: {}", difficulty );

  co_return difficulty;
}

} // namespace rn
