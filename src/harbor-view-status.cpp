/****************************************************************
**harbor-view-status.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-02.
*
* Description: Status bar in harbor view.
*
*****************************************************************/
#include "harbor-view-status.hpp"

// Revolution Now
#include "co-time.hpp"
#include "co-wait.hpp"
#include "player-mgr.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/nation.hpp"
#include "config/text.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/old-world-state.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"

// render
#include "render/extra.hpp"
#include "render/renderer.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_value_name;

string season_name( e_season const season ) {
  return base::capitalize_initials( enum_value_name( season ) );
}

} // namespace

/****************************************************************
** HarborStatusBar
*****************************************************************/
Delta HarborStatusBar::delta() const {
  return layout_.view.size;
}

ui::View& HarborStatusBar::view() noexcept { return *this; }

ui::View const& HarborStatusBar::view() const noexcept {
  return *this;
}

maybe<int> HarborStatusBar::entity() const {
  return static_cast<int>( e_harbor_view_entity::status_bar );
}

wait<> HarborStatusBar::perform_click(
    input::mouse_button_event_t const& ) {
  // Just a hack to tell the status bar to revert back to a de-
  // fault message. This allows us to replicate the OG's behavior
  // which has a good feel in that a click anywhere immediately
  // creates a reponse that clears the status bar and returns the
  // text to the default.
  inject_message( HarborStatusMsg::default_msg{} );
  co_return;
}

void HarborStatusBar::draw_text( rr::Renderer& renderer ) const {
  string const text =
      status_override_.value_or( build_status_normal() );
  pixel const color = text_color_override_.value_or(
      config_ui.dialog_text.normal );
  rr::write_centered( renderer, color, layout_.text_center,
                      text );
}

string HarborStatusBar::build_status_normal() const {
  auto& nation_conf  = nation_obj( player_.nation );
  string const stats = fmt::format(
      "{}, {}. {}, {}. Tax: {}%  Treasury: {}{}",
      nation_conf.harbor_city_name, nation_conf.country_name,
      season_name( ss_.turn.time_point.season ),
      ss_.turn.time_point.year,
      old_world_state( ss_, player_.type ).taxes.tax_rate,
      player_.money, config_text.special_chars.currency );
  return stats;
}

void HarborStatusBar::inject_message(
    HarborStatusMsg const& msg ) {
  injected_msgs_.send( msg );
}

wait<HarborStatusMsg> HarborStatusBar::wait_for_override() {
  HarborStatusMsg msg;
  do {
    msg = co_await injected_msgs_.next();
  } while( msg.holds<HarborStatusMsg::
                         default_msg_ignore_when_transient>() );
  co_return msg;
}

wait<> HarborStatusBar::status_generator() {
  HarborStatusMsg next = HarborStatusMsg::default_msg{};
  do {
    SWITCH( next ) {
      CASE( default_msg ) {
        // Display default text until an override comes in.
        status_override_.reset();
        text_color_override_.reset();
        next = co_await wait_for_override();
        break;
      }
      CASE( default_msg_ignore_when_transient ) {
        // Display default text until an override comes in.
        status_override_.reset();
        text_color_override_.reset();
        next = co_await wait_for_override();
        break;
      }
      CASE( sticky_override ) {
        // Display an override text until any message comes in.
        status_override_     = sticky_override.msg;
        text_color_override_ = config_ui.dialog_text.highlighted;
        next                 = co_await injected_msgs_.next();
        break;
      }
      CASE( transient_override ) {
        // Display an override for up to 3 secs or until another
        // override comes in.
        using namespace std::chrono_literals;
        status_override_ = transient_override.msg;
        text_color_override_ =
            transient_override.error
                ? pixel::red()
                : config_ui.dialog_text.highlighted;
        auto const override =
            co_await co::timeout( 3s, wait_for_override() );
        next =
            override.value_or( HarborStatusMsg::default_msg{} );
        break;
      }
    }
  } while( true );
}

void HarborStatusBar::draw( rr::Renderer& renderer,
                            Coord ) const {
  // Status bar wood background.
  tile_sprite( renderer, e_tile::wood_middle, layout_.view );

  // Status bar border.
  rr::Painter painter = renderer.painter();
  painter.draw_horizontal_line( layout_.view.sw().moved_up( 2 ),
                                layout_.view.size.w,
                                config_ui.window.border_dark );
  painter.draw_horizontal_line( layout_.view.sw().moved_up( 1 ),
                                layout_.view.size.w,
                                config_ui.window.border_darker );

  // Text.
  draw_text( renderer );
}

HarborStatusBar::Layout HarborStatusBar::create_layout(
    rect const canvas ) {
  Layout l;
  l.view = canvas.with_new_bottom_edge(
      config_ui.menus.menu_bar_height );
  l.text_center = l.view.center();
  return l;
}

PositionedHarborSubView<HarborStatusBar> HarborStatusBar::create(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    Rect const canvas ) {
  Layout layout = create_layout( canvas );
  auto view     = make_unique<HarborStatusBar>(
      engine, ss, ts, player, std::move( layout ) );
  HarborSubView* const harbor_sub_view = view.get();
  HarborStatusBar* p_actual            = view.get();
  return PositionedHarborSubView<HarborStatusBar>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.nw() },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborStatusBar::HarborStatusBar( IEngine& engine, SS& ss,
                                  TS& ts, Player& player,
                                  Layout layout )
  : HarborSubView( engine, ss, ts, player ), layout_( layout ) {
  status_generator_thread_ = status_generator();
}

} // namespace rn
