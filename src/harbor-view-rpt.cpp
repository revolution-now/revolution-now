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
#include "rpt.hpp"
#include "text.hpp"

// render
#include "render/typer.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborRptButtons
*****************************************************************/
Delta HarborRptButtons::button_size() {
  gfx::size const text_size =
      rr::rendered_text_line_size_pixels( "purchase" );
  Delta res = Delta::from_gfx( text_size );
  res.h += 4;
  res.w += 4;
  return res;
}

// This will be relative to the upper left of this view.
Rect HarborRptButtons::button_rect( e_rpt_button button ) {
  Coord   upper_left;
  H const button_height = button_size().h;
  switch( button ) {
    case e_rpt_button::recruit: break;
    case e_rpt_button::purchase:
      upper_left.y += button_height;
      upper_left.y += kVerticalSpacing;
      break;
    case e_rpt_button::train:
      upper_left.y += button_height;
      upper_left.y += kVerticalSpacing;
      upper_left.y += button_height;
      upper_left.y += kVerticalSpacing;
      break;
  }
  return Rect::from( upper_left, button_size() );
}

maybe<e_rpt_button> HarborRptButtons::button_for_coord(
    Coord where ) {
  for( e_rpt_button button : refl::enum_values<e_rpt_button> )
    if( where.is_inside( button_rect( button ) ) ) return button;
  return nothing;
}

string HarborRptButtons::button_text_markup(
    e_rpt_button button ) {
  switch( button ) {
    case e_rpt_button::recruit: return "[R]ecruit";
    case e_rpt_button::purchase: return "[P]urchase";
    case e_rpt_button::train: return "[T]rain";
  }
}

Delta HarborRptButtons::total_size() {
  Delta const single = button_size();
  return Delta{ .w = single.w,
                .h = single.h * 3 + kVerticalSpacing * 2 };
}

Delta HarborRptButtons::delta() const { return total_size(); }

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
  switch( event.keycode ) {
    case ::SDLK_r:
      co_await click_recruit( ss_, ts_, player_ );
      co_return true;
    case ::SDLK_p:
      co_await click_purchase( ss_, ts_, player_ );
      co_return true;
    case ::SDLK_t:
      co_await click_train( ss_, ts_, player_ );
      co_return true;
  }
  co_return false; // not handled.
}

void HarborRptButtons::draw( rr::Renderer& renderer,
                             Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  for( e_rpt_button button : refl::enum_values<e_rpt_button> ) {
    Rect const rect =
        button_rect( button ).as_if_origin_were( coord );
    bool const mouse_inside =
        input::current_mouse_position().is_inside( rect );
    if( mouse_inside )
      painter.draw_solid_rect( rect, gfx::pixel::wood() );
    else
      painter.draw_empty_rect(
          rect, rr::Painter::e_border_mode::in_out,
          gfx::pixel::wood() );
    string const text_markup = button_text_markup( button );
    string const no_markup   = remove_markup( text_markup );
    Delta const  text_size   = Delta::from_gfx(
        rr::rendered_text_line_size_pixels( no_markup ) );
    Coord const    text_upper_left = centered( text_size, rect );
    TextMarkupInfo markup_info;
    if( mouse_inside )
      markup_info = {
          .normal    = gfx::pixel::banana(),
          .highlight = gfx::pixel::banana().shaded( 2 ),
      };
    else
      markup_info = {
          .normal    = gfx::pixel::wood(),
          .highlight = gfx::pixel::wood().highlighted( 2 ),
      };
    render_text_markup( renderer, text_upper_left, {},
                        markup_info, text_markup );
  }
}

PositionedHarborSubView<HarborRptButtons>
HarborRptButtons::create( SS& ss, TS& ts, Player& player,
                          Rect const            canvas,
                          HarborBackdrop const& backdrop ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborRptButtons> view;
  HarborSubView*               harbor_sub_view = nullptr;

  Coord const lower_right{
      .x = canvas.right_edge() - 1 - 4,
      .y = canvas.bottom_edge() - backdrop.top_of_houses() - 8 };
  Coord const upper_left = lower_right - total_size();

  view = make_unique<HarborRptButtons>( ss, ts, player );
  harbor_sub_view            = view.get();
  HarborRptButtons* p_actual = view.get();
  return PositionedHarborSubView<HarborRptButtons>{
      .owned  = { .view  = std::move( view ),
                  .coord = upper_left },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborRptButtons::HarborRptButtons( SS& ss, TS& ts,
                                    Player& player )
  : HarborSubView( ss, ts, player ) {}

} // namespace rn
