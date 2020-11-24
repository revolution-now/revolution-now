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
#include "config-files.hpp"
#include "coord.hpp"
#include "deferred.hpp"
#include "frame.hpp"
#include "gfx.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "terminal.hpp"
#include "text.hpp"
#include "ttf.hpp"
#include "variant.hpp"
#include "views.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Console Commands
*****************************************************************/
constexpr string_view prompt = "> ";

/****************************************************************
** Console Plane
*****************************************************************/
constexpr uint8_t console_alpha = 60;
constexpr uint8_t text_alpha    = 225;
constexpr uint8_t cmds_alpha    = 240;
constexpr uint8_t stats_alpha   = 255;

struct ConsolePlane : public Plane {
  ConsolePlane() = default;
  void initialize() override {
    // FIXME: move this into method that gets called when logical
    // window size changes.
    le_view_.emplace(
        config_rn.console.font, main_window_logical_size().w,
        []( string const& ) {}, Color::banana(), Color::wood(),
        prompt );
  }
  void advance_state() override {
    show_percent_ += show_ ? .1 : -.1;
    show_percent_ = std::clamp( show_percent_, 0.0, 1.0 );
  }
  bool covers_screen() const override { return false; }
  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    if( show_percent_ < .0001 ) return;
    auto console_rect = Rect::from(
        compositor::section( compositor::e_section::menu_bar )
            .value_or( Rect::from(
                Coord{},
                Delta{ 0_h, main_window_logical_size().w } ) )
            .lower_left(),
        main_window_logical_rect().lower_right() -
            le_view_.get().delta().h );
    console_rect.h -=
        H{ int( console_rect.h._ * ( 1.0 - show_percent_ ) ) };

    // Render edit box.
    auto console_edit_rect = Rect::from(
        console_rect.lower_left(), le_view_.get().delta() );

    render_fill_rect( tx,
                      Color::wood().with_alpha( console_alpha ),
                      console_rect );

    auto text_color = Color::banana().with_alpha( text_alpha );
    auto stats_color =
        Color::banana().highlighted( 5 ).with_alpha(
            stats_alpha );

    // auto info_start = Coord{} + 16_h;

    // Coord mouse;
    //::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

    //{
    //  auto mouse_coords = fmt::format( "unscaled: {}", mouse );
    //  auto const& mouse_coords_tx = render_text(
    //      config_rn.console.font, Color::white(), mouse_coords
    //      );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    //// mouse.clip( ... );
    // mouse.x /= g_resolution_scale_factor.sx;
    // mouse.y /= g_resolution_scale_factor.sy;
    //{
    //  auto mouse_coords = fmt::format( "  scaled: {}", mouse );
    //  auto const& mouse_coords_tx = render_text(
    //      config_rn.console.font, Color::white(), mouse_coords
    //      );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    auto info_start = console_rect.lower_right() - 1_w;

    auto frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    auto const& frame_rate_tx = render_text(
        config_rn.console.font, stats_color, frame_rate );
    copy_texture( frame_rate_tx, tx,
                  info_start - frame_rate_tx.size() );
    info_start -= frame_rate_tx.size().h;

    auto tx_count =
        fmt::format( "tx count: {}", live_texture_count() );
    // This needs to use an uncached rendering function so that
    // it doesn't cause a feedback loop that continually in-
    // creases the texture count. This is because rendering this
    // number, in general, causes a new texture to be created.
    //
    // Disabled by default since ttf_render_text_line_uncached is
    // super slow.
#if 0
    auto tx_count_tx = ttf_render_text_line_uncached(
        config_rn.console.font, stats_color, tx_count );
    copy_texture( tx_count_tx, tx,
                  info_start - tx_count_tx.size() );
    info_start -= tx_count_tx.size().h;
#endif

    // This needs to use an uncached rendering function so that
    // it doesn't cause a feedback loop that continually in-
    // creases the texture count. This is because rendering this
    // number, in general, causes a new texture to be created.
    //
    // Disabled by default since ttf_render_text_line_uncached is
    // super slow.
#if 0
    auto text_tx_count =
        fmt::format( "text cache size: {}", text_cache_size() );
    auto text_tx_count_tx = ttf_render_text_line_uncached(
        config_rn.console.font, stats_color, text_tx_count );
    copy_texture( text_tx_count_tx, tx,
                  info_start - text_tx_count_tx.size() );
    info_start -= text_tx_count_tx.size().h;
#endif

    // FIXME: better way to get this?
    auto text_height = frame_rate_tx.size().h;

    auto        dashes = fmt::format( "--------------------" );
    auto const& dashes_tx = render_text( config_rn.console.font,
                                         text_color, dashes );
    copy_texture( dashes_tx, tx, info_start - dashes_tx.size() );
    info_start -= dashes_tx.size().h;

    for( auto const& [name, mv_avg] : event_counts() ) {
      auto formatted = fmt::format(
          "{}/f: {}", name,
          std::lround( mv_avg.average() / avg_frame_rate() ) );
      auto const& src_tx = render_text( config_rn.console.font,
                                        stats_color, formatted );
      copy_texture( src_tx, tx, info_start - src_tx.size() );
      info_start -= src_tx.size().h;
    }

    // Render the log
    int  max_lines = console_rect.h / text_height;
    auto log_px_start =
        console_rect.lower_left() -
        ttf_get_font_info( config_rn.console.font ).height;
    for( auto i = 0; i < max_lines; ++i ) {
      auto maybe_line = term::line( i );
      if( !maybe_line ) break;
      auto color = text_color;
      if( util::starts_with( maybe_line->get(), prompt ) )
        color = color.highlighted( 5 ).with_alpha( cmds_alpha );
      auto const& src_tx = render_text(
          config_rn.console.font, color, maybe_line->get() );
      copy_texture( src_tx, tx, log_px_start );
      log_px_start -= src_tx.size().h;
    }

    le_view_.get().draw( tx,
                         console_edit_rect.upper_left() - 1_w );
  }

  e_input_handled input( input::event_t const& event ) override {
    if( !holds<input::key_event_t>( event ) )
      return e_input_handled::no;
    auto const& key_event =
        *std::get_if<input::key_event_t>( &event );
    if( key_event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( key_event.keycode == ::SDLK_BACKQUOTE ) {
      show_ = !show_;
      return e_input_handled::yes;
    }
    if( !show_ ) return e_input_handled::no;
    if( key_event.keycode == ::SDLK_l &&
        key_event.mod.ctrl_down ) {
      term::clear();
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
        auto options = term::autocomplete_iterative( text );
        if( options.size() == 1 ) {
          // Set cursor pos to one-past-the-end.
          le_view_.get().set( options[0], /*cursor_pos=*/-1 );
        } else if( options.size() > 1 ) {
          term::log( "--" );
          for( auto const& option : options )
            term::log( option );
        }
      }
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_UP ) {
      CHECK( history_index_ >= -1 );
      auto maybe_item_ref = term::history( history_index_ + 1 );
      if( !maybe_item_ref ) return e_input_handled::yes;
      history_index_++;
      le_view_.get().set( maybe_item_ref->get(),
                          /*cursor_pos=*/-1 );
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_DOWN ) {
      if( history_index_ == -1 ) return e_input_handled::yes;
      CHECK( history_index_ >= 0 );
      history_index_--;
      if( history_index_ == -1 ) return e_input_handled::yes;
      auto maybe_item_ref = term::history( history_index_ );
      if( !maybe_item_ref ) return e_input_handled::yes;
      le_view_.get().set( maybe_item_ref->get(),
                          /*cursor_pos=*/-1 );
      return e_input_handled::yes;
    }
    if( key_event.keycode == ::SDLK_RETURN ) {
      auto text = le_view_.get().text();
      if( !text.empty() ) {
        history_index_ = -1;
        // Don't care here whether command succeeded or not.
        (void)term::run_cmd( text );
        le_view_.get().clear();
        return e_input_handled::yes;
      }
      return e_input_handled::no;
    }
    return le_view_.get().on_key( key_event )
               ? e_input_handled::yes
               : e_input_handled::no;
  }

  bool                         show_{ false };
  double                       show_percent_{ 0.0 };
  deferred<ui::LineEditorView> le_view_{};
  int                          history_index_{ 0 };
};

ConsolePlane g_console_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* console_plane() { return &g_console_plane; }

//
MENU_ITEM_HANDLER(
    toggle_console,
    [] { g_console_plane.show_ = !g_console_plane.show_; },
    [] { return true; } )

} // namespace rn
