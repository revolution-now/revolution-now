/****************************************************************
**console.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-03.
*
* Description: The developer/mod console.
*
*****************************************************************/
#include "console.hpp"

// Revolution Now
#include "compositor.hpp"
#include "deferred.hpp"
#include "frame.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "terminal.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "variant.hpp"
#include "views.hpp"

// config
#include "config/rn.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// gfx
#include "gfx/coord.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Console Commands
*****************************************************************/
constexpr string_view prompt = "> ";

/****************************************************************
** ConsolePlane::Impl
*****************************************************************/
constexpr uint8_t text_alpha  = 225;
constexpr uint8_t cmds_alpha  = 240;
constexpr uint8_t stats_alpha = 255;

constexpr H kDividerHeight = 1;
constexpr W kDividerWidth  = 1;

struct ConsolePlane::Impl : public Plane {
  // State.
  Terminal&                    terminal_;
  bool                         show_{ false };
  double                       show_percent_{ 0.0 };
  deferred<ui::LineEditorView> le_view_{};
  int                          history_index_{ 0 };

  Impl( Terminal& terminal ) : terminal_( terminal ) {
    // FIXME: move this into method that gets called when logical
    // window size changes and/or compositor layout changes.
    UNWRAP_CHECK(
        total_area,
        compositor::section( compositor::e_section::total ) );
    le_view_.emplace(
        config_rn.console.font, total_area.w,
        []( string const& ) {}, config_ui.dialog_text.normal,
        gfx::pixel::wood(), prompt, /*initial_text=*/"" );
  }

  void advance_state() override {
    show_percent_ += show_ ? .1 : -.1;
    show_percent_ = std::clamp( show_percent_, 0.0, 1.0 );
    compositor::set_console_size( console_height() );
  }

  bool covers_screen() const override { return false; }

  void draw( rr::Renderer& renderer ) const override {
    if( show_percent_ < .0001 ) return;
    rr::Painter painter = renderer.painter();
    UNWRAP_CHECK(
        console_rect,
        compositor::section( compositor::e_section::console ) );
    bool const mouse_over_console =
        is_mouse_over_rect( console_rect );
    bool const render_edit_box = mouse_over_console;

    Rect divider_rect = console_rect;

    UNWRAP_CHECK(
        total_area,
        compositor::section( compositor::e_section::total ) );

    if( console_rect.h < total_area.h ) {
      // Console is either at the top or bottom.
      if( console_rect.y == 0 ) {
        // Console is at the top.
        console_rect = console_rect.with_new_bottom_edge(
            console_rect.bottom_edge() - kDividerHeight );
        divider_rect.h = kDividerHeight;
        divider_rect.y = console_rect.bottom_edge();
      } else {
        // Console is at the bottom.
        divider_rect.h = kDividerHeight;
        divider_rect.y = console_rect.top_edge();
        console_rect   = console_rect.with_new_top_edge(
            console_rect.top_edge() + kDividerHeight );
      }
    }
    if( console_rect.w < total_area.w ) {
      // Console is either at the left or right.
      if( console_rect.x == 0 ) {
        // Console is on the left.
        console_rect = console_rect.with_new_right_edge(
            console_rect.right_edge() - kDividerWidth );
        divider_rect.w = kDividerWidth;
        divider_rect.x = console_rect.right_edge();
      } else {
        // Console is on the right.
        divider_rect.w = kDividerWidth;
        divider_rect.x = console_rect.left_edge();
        console_rect   = console_rect.with_new_left_edge(
            console_rect.left_edge() + kDividerWidth );
      }
    }

    // Render edit box.
    Delta const edit_box_delta = le_view_.get().delta();
    auto        console_edit_rect =
        Rect::from( console_rect.lower_left() -
                        Delta{ .h = edit_box_delta.h },
                    edit_box_delta );
    Rect text_rect = console_rect;
    if( render_edit_box ) text_rect.h -= edit_box_delta.h;

    tile_sprite( painter, e_tile::wood_middle, console_rect );
    painter.draw_solid_rect( divider_rect,
                             config_ui.window.border_dark );

    auto text_color =
        config_ui.dialog_text.normal.with_alpha( text_alpha );
    auto stats_color =
        config_ui.dialog_text.normal.highlighted( 5 ).with_alpha(
            stats_alpha );

    // FIXME: better way to get this?
    static constexpr int kFontHeight = 8;

    auto delta_for = []( string_view text ) {
      return Delta{ .w = W{ int( text.size() ) * 6 },
                    .h = H{ kFontHeight } };
    };

    // Render the log
    int const max_lines = text_rect.h / kFontHeight;
    auto      log_px_start =
        text_rect.lower_left() - Delta{ .h = H{ kFontHeight } };
    for( auto i = 0; i < max_lines; ++i ) {
      auto maybe_line = terminal_.line( i );
      if( !maybe_line ) break;
      auto color = text_color;
      if( maybe_line->starts_with( prompt ) )
        color = color.highlighted( 5 ).with_alpha( cmds_alpha );
      Delta text_size = delta_for( *maybe_line );
      renderer.typer( "simple", log_px_start, color )
          .write( *maybe_line );
      log_px_start -= Delta{ .h = text_size.h };
    }

    if( render_edit_box )
      le_view_.get().draw(
          renderer,
          console_edit_rect.upper_left() - Delta{ .w = 1 } );

    auto info_start = text_rect.lower_right() - Delta{ .w = 1 };

    for( auto const& [name, mv_avg] : event_counts() ) {
      auto formatted = fmt::format(
          "{}/f: {}", name,
          std::lround( mv_avg.average() / avg_frame_rate() ) );
      Delta formatted_size = delta_for( formatted );
      renderer
          .typer( "simple", info_start - formatted_size,
                  stats_color )
          .write( formatted );
      info_start -= Delta{ .h = formatted_size.h };
    }
  }

  e_input_handled input( input::event_t const& event ) override {
    if( !holds<input::key_event_t>( event ) )
      return e_input_handled::no;
    auto const& key_event =
        *std::get_if<input::key_event_t>( &event );
    if( key_event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( key_event.keycode == ::SDLK_BACKQUOTE ) {
      if( key_event.mod.shf_down ) {
        compositor::rotate_console();
        return e_input_handled::yes;
      }
      show_ = !show_;
      return e_input_handled::yes;
    }
    if( !show_ ) return e_input_handled::no;
    if( !is_mouse_over_console() ) return e_input_handled::no;
    if( key_event.keycode == ::SDLK_l &&
        key_event.mod.ctrl_down ) {
      terminal_.clear();
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_u &&
        key_event.mod.ctrl_down ) {
      auto text = le_view_.get().text();
      text.erase( 0, le_view_.get().cursor_pos() );
      le_view_.get().set( text, 0 );
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_TAB ) {
      auto const& text = le_view_.get().text();
      if( int( text.size() ) == le_view_.get().cursor_pos() ) {
        auto options = terminal_.autocomplete_iterative( text );
        if( options.size() == 1 ) {
          // Set cursor pos to one-past-the-end.
          le_view_.get().set( options[0], /*cursor_pos=*/-1 );
        } else if( options.size() > 1 ) {
          terminal_.log( "--" );
          for( auto const& option : options )
            terminal_.log( option );
        }
      }
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_UP ) {
      CHECK( history_index_ >= -1 );
      auto maybe_item_ref =
          terminal_.history( history_index_ + 1 );
      if( !maybe_item_ref ) return e_input_handled::yes;
      history_index_++;
      le_view_.get().set( *maybe_item_ref,
                          /*cursor_pos=*/-1 );
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_DOWN ) {
      if( history_index_ == -1 ) return e_input_handled::yes;
      CHECK( history_index_ >= 0 );
      history_index_--;
      if( history_index_ == -1 ) return e_input_handled::yes;
      auto maybe_item_ref = terminal_.history( history_index_ );
      if( !maybe_item_ref ) return e_input_handled::yes;
      le_view_.get().set( *maybe_item_ref,
                          /*cursor_pos=*/-1 );
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_RETURN ) {
      auto text = le_view_.get().text();
      if( !text.empty() ) {
        history_index_ = -1;
        // Don't care here whether command succeeded or not.
        (void)terminal_.run_cmd( text );
        le_view_.get().clear();
        return e_input_handled::yes;
      }
      return e_input_handled::no;
    }
    return le_view_.get().on_key( key_event )
               ? e_input_handled::yes
               : e_input_handled::no;
  }

  double console_height() const {
    return show_percent_ * config_ui.console.size_percentage;
  }

  bool is_mouse_over_rect( Rect rect ) const {
    return input::current_mouse_position().is_inside( rect );
  }

  bool is_mouse_over_console() const {
    maybe<Rect> rect =
        compositor::section( compositor::e_section::console );
    if( !rect.has_value() ) return false;
    return is_mouse_over_rect( *rect );
  }
};

/****************************************************************
** ConsolePlane
*****************************************************************/
Plane& ConsolePlane::impl() { return *impl_; }

ConsolePlane::~ConsolePlane() = default;

ConsolePlane::ConsolePlane( Terminal& terminal )
  : impl_( new Impl( terminal ) ) {}

Terminal& ConsolePlane::terminal() { return impl_->terminal_; }

lua::state& ConsolePlane::lua_state() {
  return impl_->terminal_.lua_state();
}

} // namespace rn
