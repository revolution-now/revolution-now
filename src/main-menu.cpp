/****************************************************************
**main-menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#include "main-menu.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "compositor.hpp"
#include "enum.hpp"
#include "game.hpp"
#include "gui.hpp"
#include "interrupts.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "tiles.hpp"
#include "turn.hpp"
#include "window.hpp"

// config
#include "config/tile-enum.rds.hpp"

// render
#include "render/renderer.hpp"

// refl
#include "refl/query-enum.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace {

string item_name( e_main_menu_item item ) {
  switch( item ) {
    case e_main_menu_item::new_: return "New Game";
    case e_main_menu_item::load: return "Load Game";
    case e_main_menu_item::settings_graphics:
      return "Graphics Settings";
    case e_main_menu_item::settings_sound:
      return "Sound Settings";
    case e_main_menu_item::quit: return "Quit";
  }
}

} // namespace

/****************************************************************
** MainMenuPlane::Impl
*****************************************************************/
struct MainMenuPlane::Impl : public Plane {
  // State
  Planes&                      planes_;
  RealGui                      gui_;
  e_main_menu_item             curr_item_ = {};
  co::stream<e_main_menu_item> selection_stream_;

 public:
  Impl( Planes& planes )
    : planes_( planes ), gui_( planes.window() ) {}

  bool covers_screen() const override { return true; }

  void draw( rr::Renderer& renderer ) const override {
    rr::Painter painter = renderer.painter();
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    tile_sprite( painter, e_tile::wood_middle, normal_area );
    H    h         = normal_area.h / 2;
    auto num_items = refl::enum_count<e_main_menu_item>;
    h -= H{ rr::rendered_text_line_size_pixels( "X" ).h } *
         SY{ int( num_items ) } / 2;
    for( auto e : refl::enum_values<e_main_menu_item> ) {
      gfx::pixel c         = gfx::pixel::banana().shaded( 3 );
      Delta      text_size = Delta::from_gfx(
          rr::rendered_text_line_size_pixels( item_name( e ) ) );
      auto w   = normal_area.w / 2 - text_size.w / 2;
      auto dst = Rect::from( Coord{}, text_size )
                     .shifted_by( Delta{ .w = w, .h = h } );
      dst = dst.as_if_origin_were( normal_area.upper_left() );
      rr::Typer typer = renderer.typer( dst.upper_left(), c );
      typer.write( item_name( e ) );
      dst = dst.with_border_added( 2 );
      dst.x -= 3;
      dst.w += 6;
      if( e == curr_item_ )
        painter.draw_empty_rect(
            dst, rr::Painter::e_border_mode::outside,
            gfx::pixel::banana() );
      h += dst.delta().h;
    }
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::key_event: {
        auto& val = event.get<input::key_event_t>();
        if( val.change != input::e_key_change::down ) break;
        // It's a key down.
        switch( val.keycode ) {
          case ::SDLK_UP:
          case ::SDLK_KP_8:
            curr_item_ = util::find_previous_and_cycle(
                refl::enum_values<e_main_menu_item>,
                curr_item_ );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            curr_item_ = util::find_subsequent_and_cycle(
                refl::enum_values<e_main_menu_item>,
                curr_item_ );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
            selection_stream_.send( curr_item_ );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_ESCAPE:
            selection_stream_.send( e_main_menu_item::quit );
            handled = e_input_handled::yes;
            break;
          default: break;
        }
        break;
      }
      default: //
        break;
    }
    return handled;
  }

  wait<> item_selected( e_main_menu_item item ) {
    switch( item ) {
      case e_main_menu_item::new_: //
        co_await run_game_with_mode( planes_,
                                     StartMode::new_{} );
        break;
      case e_main_menu_item::load:
        co_await run_game_with_mode( planes_,
                                     StartMode::load{} );
        break;
      case e_main_menu_item::quit: //
        throw game_quit_interrupt{};
      case e_main_menu_item::settings_graphics:
        co_await gui_.message_box( "No graphics settings yet." );
        break;
      case e_main_menu_item::settings_sound:
        co_await gui_.message_box( "No sound settings yet." );
        break;
    }
  }
};

/****************************************************************
** MainMenuPlane
*****************************************************************/
Plane& MainMenuPlane::impl() { return *impl_; }

MainMenuPlane::~MainMenuPlane() = default;

MainMenuPlane::MainMenuPlane( Planes& planes )
  : impl_( new Impl( planes ) ) {}

wait<> MainMenuPlane::run() {
  co::stream<e_main_menu_item>& selections =
      impl_->selection_stream_;
  // TODO: Temporary
  selections.send( e_main_menu_item::load );
  while( true ) {
    e_main_menu_item item = co_await selections.next();
    try {
      co_await impl_->item_selected( item );
    } catch( game_quit_interrupt const& ) { co_return; }
  }
}

/****************************************************************
** API
*****************************************************************/
wait<> run_main_menu( Planes& planes ) {
  auto        popper    = planes.new_copied_group();
  PlaneGroup& new_group = planes.back();
  WindowPlane window_plane;
  new_group.window = &window_plane;

  // Relies on the window plane being present.
  MainMenuPlane main_menu_plane( planes );
  new_group.main_menu = &main_menu_plane;
  co_await main_menu_plane.run();
}

} // namespace rn
