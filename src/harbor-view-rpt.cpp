/****************************************************************
**harbor-view-rpt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Recruit/Purchase/Train button UI elements within
*              the harbor view.
*
*****************************************************************/
#include "harbor-view-rpt.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "harbor-view-backdrop.hpp"
#include "harbor-view-dock.hpp"
#include "harbor-view-inport.hpp"
#include "input.hpp"
#include "markup.hpp"
#include "rpt.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// render
#include "render/renderer.hpp"
#include "render/typer.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_values;

} // namespace

/****************************************************************
** HarborRptButtons
*****************************************************************/
maybe<e_rpt_button> HarborRptButtons::button_for_coord(
    point const where ) const {
  for( auto const button : enum_values<e_rpt_button> )
    if( where.is_inside( layout_.buttons[button].bounds ) )
      return button;
  return nothing;
}

Delta HarborRptButtons::delta() const {
  return layout_.view.size;
}

maybe<int> HarborRptButtons::entity() const {
  return static_cast<int>( e_harbor_view_entity::dock );
}

ui::View& HarborRptButtons::view() noexcept { return *this; }

ui::View const& HarborRptButtons::view() const noexcept {
  return *this;
}

wait<> HarborRptButtons::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  maybe<e_rpt_button> const button =
      button_for_coord( event.pos );
  if( !button.has_value() ) co_return;
  // This is so that the button doesn't stay highlighted while
  // the window is open, since once the window pops open this
  // view won't get "mouse leave" events.
  mouse_hover_.reset();
  // But then if the user closes the window while the mouse is
  // still over a button then we want to restart the hover state.
  SCOPE_EXIT {
    update_mouse_hover(
        input::current_mouse_position()
            .to_gfx()
            .point_becomes_origin( layout_.view.origin ) );
  };
  switch( *button ) {
    case e_rpt_button::recruit:
      co_await click_recruit( ss_, ts_, player_ );
      break;
    case e_rpt_button::purchase:
      co_await click_purchase( ss_, ts_, player_ );
      break;
    case e_rpt_button::train:
      co_await click_train( ss_, ts_, player_ );
      break;
  }
}

wait<bool> HarborRptButtons::perform_key(
    input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down )
    co_return false;
  if( event.mod.shf_down ) co_return false;
  SCOPE_EXIT {
    update_mouse_hover(
        input::current_mouse_position()
            .to_gfx()
            .point_becomes_origin( layout_.view.origin ) );
  };
  switch( event.keycode ) {
    case ::SDLK_r:
      mouse_hover_.reset();
      co_await click_recruit( ss_, ts_, player_ );
      co_return true;
    case ::SDLK_p:
      mouse_hover_.reset();
      co_await click_purchase( ss_, ts_, player_ );
      co_return true;
    case ::SDLK_t:
      mouse_hover_.reset();
      co_await click_train( ss_, ts_, player_ );
      co_return true;
  }
  co_return false; // not handled.
}

bool HarborRptButtons::on_mouse_move(
    input::mouse_move_event_t const& event ) {
  update_mouse_hover( event.pos );
  return true;
}

void HarborRptButtons::on_mouse_enter( Coord const from ) {
  update_mouse_hover( from );
}

void HarborRptButtons::on_mouse_leave( Coord const ) {
  mouse_hover_.reset();
}

void HarborRptButtons::update_mouse_hover(
    point const mouse_pos ) {
  mouse_hover_ = button_for_coord( mouse_pos );
}

void HarborRptButtons::draw( rr::Renderer& renderer,
                             Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      point( coord ).distance_from_origin().to_double() );
  bool const l_mouse_down =
      input::get_mouse_buttons_state().l_down;

  render_sprite( renderer, layout_.post_sprite_origin,
                 e_tile::harbor_post );

  for( e_rpt_button const button : enum_values<e_rpt_button> ) {
    e_tile button_tile     = e_tile::harbor_rpt_button_normal;
    e_tile const text_tile = layout_.buttons[button].text_tile;
    point const button_origin =
        layout_.buttons[button].bounds.origin;
    point text_origin = button_origin;
    if( mouse_hover_ == button ) {
      if( l_mouse_down ) {
        button_tile = e_tile::harbor_rpt_button_press;
        text_origin += size{ .w = 1, .h = 1 };
      } else {
        button_tile = e_tile::harbor_rpt_button_hover;
      }
    }
    render_sprite( renderer, button_origin - size{ .h = 2 },
                   e_tile::harbor_rpt_hangers );
    render_sprite( renderer, button_origin, button_tile );
    render_sprite( renderer, text_origin, text_tile );
  }
}

HarborRptButtons::Layout HarborRptButtons::create_layout(
    rect const canvas, HarborBackdrop const& backdrop ) {
  Layout l;
  size const signpost_size = sprite_size( e_tile::harbor_post );

  // The view does not include the entire sign; it includes the
  // buttons and one layer of pixels around them.
  l.view.origin.y = backdrop.horizon_center().y - 79;
  l.view.origin.x = canvas.right() - signpost_size.w + 2;
  l.view.size     = size{ .w = 51, .h = 42 };

  l.post_left_point    = { .x = -2, .y = -3 };
  l.post_sprite_origin = l.post_left_point - size{ .h = 31 };

  size const button_size =
      sprite_size( e_tile::harbor_rpt_recruit );
  point button_origin = { .x = 1, .y = 1 };
  e_rpt_button button = {};

  button            = e_rpt_button::recruit;
  l.buttons[button] = {
    .bounds = { .origin = button_origin, .size = button_size },
    .text_tile = e_tile::harbor_rpt_recruit };
  button_origin.y += button_size.h + 2;

  button            = e_rpt_button::purchase;
  l.buttons[button] = {
    .bounds = { .origin = button_origin, .size = button_size },
    .text_tile = e_tile::harbor_rpt_purchase };
  button_origin.y += button_size.h + 2;

  button            = e_rpt_button::train;
  l.buttons[button] = {
    .bounds = { .origin = button_origin, .size = button_size },
    .text_tile = e_tile::harbor_rpt_train };
  button_origin.y += button_size.h + 2;
  return l;
}

PositionedHarborSubView<HarborRptButtons>
HarborRptButtons::create( SS& ss, TS& ts, Player& player,
                          Rect const canvas,
                          HarborBackdrop const& backdrop ) {
  Layout layout = create_layout( canvas, backdrop );
  auto view     = make_unique<HarborRptButtons>(
      ss, ts, player, std::move( layout ) );
  HarborSubView* const harbor_sub_view = view.get();
  HarborRptButtons* p_actual           = view.get();
  return PositionedHarborSubView<HarborRptButtons>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.nw() },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborRptButtons::HarborRptButtons( SS& ss, TS& ts,
                                    Player& player,
                                    Layout layout )
  : HarborSubView( ss, ts, player ), layout_( layout ) {}

} // namespace rn
