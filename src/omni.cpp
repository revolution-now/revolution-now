/****************************************************************
**omni.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A plane that is always on the top of the stack.
*
*****************************************************************/
#include "omni.hpp"

// rds
#include "omni-impl.rds.hpp"

// Revolution Now
#include "aspect.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "frame.hpp"
#include "iengine.hpp"
#include "imenu-server.hpp"
#include "input.hpp"
#include "interrupts.hpp"
#include "iuser-config.hpp"
#include "plane.hpp"
#include "query-enum.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/user.rds.hpp"

// video
#include "video/window.hpp"

// luapp
#include "luapp/register.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/monitor.hpp"
#include "gfx/resolution.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::oriented_point;
using ::refl::cycle_enum;
using ::refl::enum_count;
using ::refl::enum_from_integral;
using ::refl::enum_value_name;

e_debug_overlay_level g_debug_overlay = {};

auto const kSupportedMenuItems = [] {
  refl::enum_map<e_menu_item, bool> m;
  m[e_menu_item::toggle_fullscreen] = true;
  m[e_menu_item::scale_down]        = true;
  m[e_menu_item::scale_up]          = true;
  m[e_menu_item::scale_optimal]     = true;
  m[e_menu_item::fit_window]        = true;
  return m;
}();

string_view cycle_omni_overlay() {
  UNWRAP_CHECK_T(
      g_debug_overlay,
      enum_from_integral<e_debug_overlay_level>(
          ( static_cast<int>( g_debug_overlay ) + 1 ) %
          enum_count<e_debug_overlay_level> ) );
  return enum_value_name( g_debug_overlay );
}

string named_ratio_canonical_name(
    gfx::e_named_aspect_ratio const r ) {
  gfx::size const sz = named_aspect_ratio( r );
  return fmt::format( "{}:{}", sz.w, sz.h );
}

auto line_logger( vector<string>& lines ATTR_LIFETIMEBOUND ) {
  // Use a y-combinator like approach to produce a lambda that
  // can call itself.
  static constexpr auto fmt_type_impl = mp::overload{
    []( auto const&, gfx::size const s ) -> string {
      return fmt::format( "{}x{}", s.w, s.h );
    },
    []( auto const&, gfx::point const p ) -> string {
      return fmt::format( "[{},{}]", p.x, p.y );
    },
    []( auto const&, gfx::rect const r ) -> string {
      return fmt::format( "[{},{}] {}x{}", r.origin.x,
                          r.origin.y, r.size.w, r.size.h );
    },
    []( auto const&, double const d ) -> string {
      return fmt::format( "{:.3}", d );
    },
    []( auto const&,
        gfx::e_named_aspect_ratio const& nar ) -> string {
      return fmt::format( "{}",
                          named_ratio_canonical_name( nar ) );
    },
    []<typename T>( auto const& self,
                    maybe<T> const m ) -> string {
      if( !m.has_value() )
        return base::to_str( m );
      else
        return self( self, *m );
    },
    []( auto const&, auto const& o ) -> string {
      return base::to_str( o );
    },
  };
  static constexpr auto fmt_type =
      []<typename... Args>( Args&&... args ) {
        return fmt_type_impl( fmt_type_impl,
                              std::forward<Args>( args )... );
      };
  return [&]<typename... Args>(
             fmt::format_string<Args...> fmt_str,
             Args const&... args ) {
    if constexpr( sizeof...( Args ) == 0 ) {
      (void)fmt_type;      // suppress unused variable warning.
      (void)fmt_type_impl; // suppress unused variable warning.
    }
    // We need the fmt::runtime here because we are changing the
    // type of the args by passing them through fmt_type which
    // would leave them incompatible with the type of the
    // compile-time format string object fmt_str. But we are not
    // losing compile-time format string checking because that
    // has already been done by the construction of fmt_str upon
    // calling this function.
    //
    // TODO: change to std::runtime_format when possible.
    tuple const storage{ fmt_type( args )... };
    lines.push_back( std::vformat(
        fmt_str.get(), apply(
                           []( auto&&... os ) {
                             return std::make_format_args(
                                 os... );
                           },
                           storage ) ) );
  };
}

maybe<int> get_resolution_cycle_size(
    gfx::Resolutions const& resolutions ) {
  if( !resolutions.selected.has_value() ) return nothing;
  return resolutions.scored.size();
}

maybe<gfx::ResolutionScores const&> get_resolution_scores(
    IEngine& engine ) {
  auto const& resolutions = engine.resolutions();
  auto const& selected    = resolutions.selected;
  if( !selected.has_value() ) return nothing;
  return resolutions.scored[*selected].scores;
}

void cycle_resolution( gfx::Resolutions const& resolutions,
                       int const delta ) {
  // Copy; cannot modify the global state directly.
  auto const& curr = resolutions;
  if( !curr.selected.has_value() ) return;
  auto const& available = curr.scored;
  if( available.empty() ) return;
  int idx = *curr.selected;
  // The "better" resolutions, which also tend to be more scaled
  // up (though not always) are at the start of the list, so for
  // "scaling up" we must go negative.
  idx += ( -delta );
  // Need to do this because the c++ modulus is the wrong type.
  while( idx < 0 ) idx += available.size();
  idx %= available.size();
  CHECK_LT( idx, ssize( available ) );
  auto new_resolutions     = resolutions;
  new_resolutions.selected = idx;
  change_resolution( new_resolutions );
}

void set_resolution_idx_to_optimal(
    gfx::Resolutions const& resolutions ) {
  auto const& curr = resolutions;
  if( curr.scored.empty() ) return;
  auto new_resolutions     = resolutions;
  new_resolutions.selected = 0;
  change_resolution( new_resolutions );
}

} // namespace

/****************************************************************
** OmniPlane::Impl
*****************************************************************/
// This plane is intended to be:
//
//   1) Always present.
//   2) Always (mostly) invisible.
//   3) Always on top.
//   4) Catching any global events (such as special key presses).
//
struct OmniPlane::Impl : public IPlane {
  IEngine& engine_;
  IMenuServer& menu_server_;
  bool show_game_cursor_ = true;
  co::stream<char> alt_key_seq;
  wait<> magic_key_seq_thread;

  vector<IMenuServer::Deregistrar> dereg_;

  Impl( IEngine& engine, IMenuServer& menu_server )
    : engine_( engine ), menu_server_( menu_server ) {
    magic_key_seq_thread = run_alt_key_seq_monitor();
    for( auto const& [item, enabled] : kSupportedMenuItems )
      if( enabled )
        dereg_.push_back(
            menu_server.register_handler( item, *this ) );
  }

  wait<> run_alt_key_seq_monitor() {
    while( true ) {
      // This function will return if/when the cheat key sequence
      // is detected.
      co_await monitor_magic_key_sequence( alt_key_seq );
      input::event_queue().push( input::cheat_event_t{} );
      // Since cheat mode is enabled per game, we need to keep
      // this loop going in case the player starts a new game.
    }
  }

  bool can_cycle_resolution_up() {
    auto const idx = engine_.resolutions().selected;
    if( !idx.has_value() ) return false;
    CHECK_GE( *idx, 0 );
    return *idx > 0;
  }

  bool can_cycle_resolution_down() {
    auto const size =
        get_resolution_cycle_size( engine_.resolutions() );
    auto const idx = engine_.resolutions().selected;
    CHECK_EQ( size.has_value(), idx.has_value() );
    if( !size.has_value() ) return false;
    CHECK_LT( *idx, *size );
    CHECK_GT( *size, 0 );
    CHECK_GE( *idx, 0 );
    if( *idx == *size - 1 ) return false;
    return true;
  }

  bool will_handle_menu_click(
      e_menu_item const item ) override {
    switch( item ) {
      case e_menu_item::scale_optimal: {
        auto const idx = engine_.resolutions().selected;
        if( !idx.has_value() || *idx == 0 ) return false;
        break;
      }
      case rn::e_menu_item::scale_up: {
        if( !can_cycle_resolution_up() ) return false;
        break;
      }
      case rn::e_menu_item::scale_down: {
        if( !can_cycle_resolution_down() ) return false;
        break;
      }
      case e_menu_item::fit_window: {
        auto const resolution = get_resolution( engine_ );
        if( !resolution.has_value() ) return false;
        if( !vid::can_shrink_window_to_fit( engine_.video(),
                                            engine_.window(),
                                            *resolution ) )
          return false;
        break;
      }
      default:
        break;
    }
    return kSupportedMenuItems[item];
  }

  void handle_menu_click( e_menu_item const item ) override {
    switch( item ) {
      case e_menu_item::scale_down:
        cycle_resolution( engine_.resolutions(), -1 );
        break;
      case e_menu_item::scale_optimal:
        set_resolution_idx_to_optimal( engine_.resolutions() );
        break;
      case e_menu_item::scale_up:
        cycle_resolution( engine_.resolutions(), 1 );
        break;
      case e_menu_item::toggle_fullscreen:
        this->toggle_fullscreen();
        break;
      case e_menu_item::fit_window: {
        UNWRAP_CHECK( resolution, get_resolution( engine_ ) );
        vid::shrink_window_to_fit(
            engine_.video(), engine_.window(), resolution );
        break;
      }
      default:
        break;
    }
  }

  void toggle_fullscreen() const {
    vid::toggle_fullscreen( engine_.video(), engine_.window() );
  }

  inline static auto SHADED_WOOD =
      gfx::pixel::wood().shaded( 8 );

  inline static auto overlay_fg = gfx::pixel::banana();
  inline static auto overlay_bg = SHADED_WOOD;

  void render_framerate( rr::Renderer& renderer ) const {
    vector<string> lines;
    auto const log = line_logger( lines );

    log( "f/s: {}", fmt::format( "{:.1f}", avg_frame_rate() ) );

    render_text_overlay_with_anchor(
        renderer, rr::TextLayout{}, lines,
        oriented_point{
          .anchor    = renderer.logical_screen_rect().se(),
          .placement = e_cdirection::se },
        overlay_fg, overlay_bg, /*scale=*/1 );
  }

  void render_bad_window_size_overlay(
      rr::Renderer& renderer ) const {
    auto const physical_size = main_window_physical_size(
        engine_.video(), engine_.window() );
    auto const default_logical = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    tile_sprite( renderer, e_tile::hazard,
                 Rect::from_gfx( default_logical ) );
    vector<string> help_msg{
      fmt::format( "  Window size {}x{} not supported.",
                   physical_size.w, physical_size.h ),
      "Please expand the size of your window." };
    render_text_overlay_with_anchor(
        renderer, rr::TextLayout{}, help_msg,
        oriented_point{ .anchor    = default_logical.center(),
                        .placement = e_cdirection::c },
        gfx::pixel::red(), SHADED_WOOD,
        /*scale=*/2 );
  }

  void render_resolution_info( rr::Renderer& renderer ) const {
    auto const resolution = get_resolution( engine_ );
    if( !resolution.has_value() ) {
      render_bad_window_size_overlay( renderer );
      return;
    }

    vector<string> lines;
    auto const log = line_logger( lines );

    CHECK( resolution.has_value() );
    auto const monitor = gfx::monitor_properties(
        main_window_physical_size( engine_.video(),
                                   engine_.window() ),
        monitor_dpi( engine_.video() ) );
    UNWRAP_CHECK_T( auto const scores,
                    get_resolution_scores( engine_ ) );
    // Although this is a tolerance, it is not a rating option,
    // since it is not used to filter results, just to describe
    // them.
    double const RATIO_TOLERANCE = 0.04;
    auto const logical_aspect =
        gfx::find_close_named_aspect_ratio( resolution->logical,
                                            RATIO_TOLERANCE );

    log( "Screen:" );
    log( " dimensions:     {}", monitor.physical_screen );
    log( " dpi.horizontal: {}",
         monitor.dpi.member( &gfx::MonitorDpi::horizontal ) );
    log( " dpi.vertical:   {}",
         monitor.dpi.member( &gfx::MonitorDpi::vertical ) );
    log( " dpi.diagonal:   {}",
         monitor.dpi.member( &gfx::MonitorDpi::diagonal ) );
    log( " inches:         {}", monitor.diagonal_inches );
    log( "Window:" );
    log( " dims:           {}", resolution->physical_window );
    log( "Logical:" );
    log( " logical:        {}", resolution->logical );
    log( " logical.aspect: {}", logical_aspect );
    log( " scale:          {}", resolution->scale );
    log( " viewport.orig:  {}", resolution->viewport.origin );
    log( " viewport.size:  {}", resolution->viewport.size );
    log( "Scores:" );
    log( " fit score:      {}", scores.fitting );
    log( " size score:     {}", scores.pixel_size );
    log( " overall score:  {}", scores.overall );

    gfx::point const info_region_anchor =
        gfx::point{ .x = 0, .y = 16 };

    render_text_overlay_with_anchor(
        renderer, rr::TextLayout{ .monospace = true }, lines,
        oriented_point{ .anchor    = info_region_anchor,
                        .placement = e_cdirection::nw },
        overlay_fg, overlay_bg, /*scale=*/1 );
  }

  void render_logical_resolution(
      rr::Renderer& renderer ) const {
    UNWRAP_CHECK_T( auto const& resolution,
                    get_resolution( engine_ ) );

    vector<string> lines;
    auto const log = line_logger( lines );

    log( "{}", resolution.logical );

    gfx::point const info_region_anchor =
        main_window_logical_rect( engine_.video(),
                                  engine_.window(),
                                  engine_.resolutions() )
            .ne()
            .moved_left( config_ui.panel.width )
            .moved_right( 2 );

    render_text_overlay_with_anchor(
        renderer, rr::TextLayout{}, lines,
        oriented_point{ .anchor    = info_region_anchor,
                        .placement = e_cdirection::nw },
        overlay_fg, overlay_bg, /*scale=*/1 );
  }

  void render_debug_overlays( rr::Renderer& renderer ) const {
    if( g_debug_overlay >= e_debug_overlay_level::fps )
      render_framerate( renderer );
    if( g_debug_overlay >= e_debug_overlay_level::resolution )
      render_logical_resolution( renderer );
    if( g_debug_overlay >= e_debug_overlay_level::full )
      render_resolution_info( renderer );
  }

  bool window_too_small() const {
    return !get_resolution( engine_ ).has_value();
  }

  void draw( rr::Renderer& renderer ) const override {
    // This must be the top-most thing that is drawn in the en-
    // tire game, since nothing else can effectively be drawn if
    // we don't know what the logical resolution is, which in
    // practice would happen if the window is too small.
    if( window_too_small() ) {
      render_bad_window_size_overlay( renderer );
      return;
    }

    render_debug_overlays( renderer );

    if( show_game_cursor_ )
      render_sprite(
          renderer,
          input::current_mouse_position() - Delta{ .w = 16 },
          e_tile::mouse_arrow1 );
  }

  void update_system_cursor() {
    auto const viewport = get_resolution( engine_ ).member(
        &gfx::Resolution::viewport );
    if( !viewport.has_value() ) {
      input::set_show_system_cursor( true );
      show_game_cursor_ = false;
      return;
    } else {
      bool const show_system_cursor =
          input::should_show_system_cursor( *viewport );
      input::set_show_system_cursor( show_system_cursor );
      show_game_cursor_ = !show_system_cursor;
    }
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    update_system_cursor();
    SWITCH( event ) {
      CASE( quit_event ) { throw exception_hard_exit{}; }
      CASE( win_event ) {
        if( win_event.type ==
            input::e_win_event_type::resized ) {
          // This will just have the effect of queuing another
          // input event for a resolution change which will then
          // be handled at the top of the next frame.
          on_main_window_resized( engine_.video(),
                                  engine_.window() );
          handled = e_input_handled::yes;
        }
        break;
      }
      CASE( key_event ) {
        if( key_event.change != input::e_key_change::down )
          break;
        if( key_event.mod.alt_down ) {
          alt_key_seq.send( key_event.keycode );
          // We need to allow other planes to handle this because
          // we don't know if this key is intended to be part of
          // a magic sequence (there are other Alt key combos in
          // the game, such as to open menus).
          handled = e_input_handled::no;
          break;
        }
        switch( key_event.keycode ) {
          case ::SDLK_PAUSE: {
            // This will essentially block the thread until the
            // user hit's the pause key again. Though it will
            // show a message on screen instructing the player
            // about this.
            engine_.pause();
            break;
          }
          case ::SDLK_DELETE:
            if( !key_event.mod.ctrl_down ||
                !key_event.mod.shf_down )
              break;
            throw exception_restart(
                "user-initiated restart via Ctrl-Shift-Del" );
          case ::SDLK_F12:
            // if( !screenshot() )
            //   lg.warn( "failed to take screenshot." );
            break;
          case ::SDLK_F11:
            this->toggle_fullscreen();
            handled = e_input_handled::yes;
            break;
          case ::SDLK_F10: {
            auto const next_mode = cycle_enum(
                engine_.user_config()
                    .read()
                    .graphics.render_framebuffer_mode );
            CHECK( engine_.user_config().modify(
                [&]( config_user_t& conf ) {
                  conf.graphics.render_framebuffer_mode =
                      next_mode;
                } ) );
            engine_.renderer_settings()
                .set_render_framebuffer_mode( next_mode );
            lg.info( "framebuffer render mode: {}", next_mode );
            handled = e_input_handled::yes;
            break;
          }
          case ::SDLK_F3:
            // TODO: temporary until the menu system supports
            // shortcuts keys for menu items.
            if( menu_server_.can_handle_menu_click(
                    e_menu_item::continental_congress ) ) {
              if( menu_server_.click_item(
                      e_menu_item::continental_congress ) )
                handled = e_input_handled::yes;
            }
            break;
          case ::SDLK_o:
            if( key_event.mod.ctrl_down ) {
              cycle_omni_overlay();
              handled = e_input_handled::yes;
            }
            break;
          case ::SDLK_MINUS:
            if( key_event.mod.ctrl_down ) {
              if( can_cycle_resolution_down() )
                cycle_resolution( engine_.resolutions(), -1 );
              handled = e_input_handled::yes;
            }
            break;
          case ::SDLK_EQUALS:
            if( key_event.mod.ctrl_down ) {
              if( can_cycle_resolution_up() )
                cycle_resolution( engine_.resolutions(), 1 );
              handled = e_input_handled::yes;
            }
            break;
          case ::SDLK_q:
            if( key_event.mod.ctrl_down )
              throw exception_hard_exit{};
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
    bool const hazard_is_shown = window_too_small();
    return hazard_is_shown ? e_input_handled::yes : handled;
  }

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {}
};

/****************************************************************
** OmniPlane
*****************************************************************/
IPlane& OmniPlane::impl() { return *impl_; }

OmniPlane::~OmniPlane() = default;

OmniPlane::OmniPlane( IEngine& engine, IMenuServer& menu_server )
  : impl_( new Impl( engine, menu_server ) ) {}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_AUTO_FN( cycle_omni_overlay );

}

} // namespace rn
