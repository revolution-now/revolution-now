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

// Revolution Now
#include "aspect.hpp"
#include "frame.hpp"
#include "input.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "resolution.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/tile-enum.rds.hpp"

// luapp
#include "luapp/register.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/logical.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_debug_omni_overlay = true;

auto const kSupportedMenuItems = [] {
  refl::enum_map<e_menu_item, bool> m;
  m[e_menu_item::toggle_fullscreen] = true;
  m[e_menu_item::scale_down]        = true;
  m[e_menu_item::scale_up]          = true;
  m[e_menu_item::scale_optimal]     = true;
  m[e_menu_item::fit_window]        = true;
  return m;
}();

string toggle_omni_overlay() {
  g_debug_omni_overlay = !g_debug_omni_overlay;
  return g_debug_omni_overlay ? "on" : "off";
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
    []<typename T>( auto const&    self,
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
    lines.push_back( fmt::format( fmt::runtime( fmt_str ),
                                  fmt_type( args )... ) );
  };
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
  bool show_game_cursor_ = true;

  vector<MenuPlane::Deregistrar> dereg_;

  Impl( MenuPlane& menu_plane ) {
    for( auto const& [item, enabled] : kSupportedMenuItems )
      if( enabled )
        dereg_.push_back(
            menu_plane.register_handler( item, *this ) );
  }

  bool can_cycle_resolution_up() {
    auto const idx = get_resolution_idx();
    CHECK_GE( *idx, 0 );
    return *idx > 0;
  }

  bool can_cycle_resolution_down() {
    auto const size = get_resolution_cycle_size();
    auto const idx  = get_resolution_idx();
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
        auto const idx = get_resolution_idx();
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
        if( !can_shrink_window_to_fit() ) return false;
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
        cycle_resolution( -1 );
        break;
      case e_menu_item::scale_optimal:
        set_resolution_idx_to_optimal();
        break;
      case e_menu_item::scale_up:
        cycle_resolution( 1 );
        break;
      case e_menu_item::toggle_fullscreen:
        this->toggle_fullscreen();
        break;
      case e_menu_item::fit_window: {
        shrink_window_to_fit();
        break;
      }
      default:
        break;
    }
  }

  void toggle_fullscreen() const { rn::toggle_fullscreen(); }

  void render_framerate( rr::Renderer& renderer ) const {
    vector<string> lines;
    auto const     log = line_logger( lines );

    log( "f/s: {}", fmt::format( "{:.1f}", avg_frame_rate() ) );

    static gfx::pixel shaded_wood =
        gfx::pixel::wood().shaded( 2 );
    render_text_overlay_with_anchor(
        renderer, lines, renderer.logical_screen_rect().se(),
        e_cdirection::se, gfx::pixel::banana(), shaded_wood );
  }

  void render_bad_window_size_overlay(
      rr::Renderer& renderer ) const {
    auto const  physical_size   = main_window_physical_size();
    rr::Painter painter         = renderer.painter();
    auto const  default_logical = main_window_logical_rect();
    painter.draw_solid_rect( default_logical,
                             gfx::pixel::yellow() );
    vector<string> help_msg{
      fmt::format( "Window size {}x{} not supported.",
                   physical_size.w, physical_size.h ),
      "Please resize your window." };
    render_text_overlay_with_anchor(
        renderer, help_msg, default_logical.center(),
        e_cdirection::c, gfx::pixel::white(),
        gfx::pixel::black() );
  }

  void render_aspect_info( rr::Renderer& renderer ) const {
    auto const resolution = get_global_resolution();
    if( !resolution.has_value() ) {
      render_bad_window_size_overlay( renderer );
      return;
    }

    vector<string> lines;
    auto const     log = line_logger( lines );

    CHECK( resolution.has_value() );
    auto const monitor = monitor_properties(
        main_window_physical_size(), monitor_dpi() );
    UNWRAP_CHECK_T( auto const scores,
                    get_global_resolution_scores() );
    // Although this is a tolerance, it is not a rating option,
    // since it is not used to filter results, just to describe
    // them.
    double const RATIO_TOLERANCE = 0.04;
    auto const   logical_aspect =
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
    log( " viewport:       {}", resolution->viewport );
    log( "Scores:" );
    log( " fit score:      {}", scores.fitting );
    log( " size score:     {}", scores.pixel_size );
    log( " overall score:  {}", scores.overall );

    gfx::point const info_region_anchor =
        gfx::point{ .x = 32, .y = 32 };

    render_text_overlay_with_anchor(
        renderer, lines, info_region_anchor, e_cdirection::nw,
        gfx::pixel::white(), gfx::pixel::black() );
  }

  void render_debug_overlay( rr::Renderer& renderer ) const {
    render_aspect_info( renderer );
  }

  void draw( rr::Renderer& renderer ) const override {
    render_framerate( renderer );
    if( g_debug_omni_overlay ) render_debug_overlay( renderer );
    if( show_game_cursor_ )
      render_sprite(
          renderer,
          input::current_mouse_position() - Delta{ .w = 16 },
          e_tile::mouse_arrow1 );
  }

  void update_system_cursor() {
    auto const viewport = get_global_resolution().member(
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
      CASE( quit_event ) { throw exception_exit{}; }
      CASE( key_event ) {
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        switch( key_event.keycode ) {
          case ::SDLK_F12:
            // if( !screenshot() )
            //   lg.warn( "failed to take screenshot." );
            break;
          case ::SDLK_F11:
            this->toggle_fullscreen();
            break;
          case ::SDLK_MINUS:
            if( key_event.mod.ctrl_down ) {
              if( can_cycle_resolution_down() )
                cycle_resolution( -1 );
            } else {
              handled = e_input_handled::no;
            }
            break;
          case ::SDLK_EQUALS:
            if( key_event.mod.ctrl_down ) {
              if( can_cycle_resolution_up() )
                cycle_resolution( 1 );
            } else {
              handled = e_input_handled::no;
            }
            break;
          case ::SDLK_q:
            if( key_event.mod.ctrl_down ) throw exception_exit{};
            handled = e_input_handled::no;
            break;
          default: //
            handled = e_input_handled::no;
            break;
        }
        break;
      }
      default:
        break;
    }
    return handled;
  }

  void on_logical_resolution_changed( e_resolution ) override {}
};

/****************************************************************
** OmniPlane
*****************************************************************/
IPlane& OmniPlane::impl() { return *impl_; }

OmniPlane::~OmniPlane() = default;

OmniPlane::OmniPlane( MenuPlane& menu_plane )
  : impl_( new Impl( menu_plane ) ) {}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_AUTO_FN( toggle_omni_overlay );

}

} // namespace rn
