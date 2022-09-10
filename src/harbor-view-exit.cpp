/****************************************************************
**harbor-view-exit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Exit button UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-exit.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborExitButton
*****************************************************************/
Delta HarborExitButton::delta() const {
  Delta res = kExitBlockPixels;
  // +1 for bottom/right border.
  ++res.w;
  ++res.h;
  return res;
}

maybe<int> HarborExitButton::entity() const {
  return static_cast<int>( e_harbor_view_entity::exit );
}

ui::View& HarborExitButton::view() noexcept { return *this; }

ui::View const& HarborExitButton::view() const noexcept {
  return *this;
}

wait<> HarborExitButton::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( rect( {} ) ) );
  throw harbor_view_exit_interrupt{};
}

void HarborExitButton::draw( rr::Renderer& renderer,
                             Coord         coord ) const {
  rr::Painter   painter   = renderer.painter();
  auto          r         = rect( coord );
  static string text      = "Exit";
  Delta         text_size = Delta::from_gfx(
      rr::rendered_text_line_size_pixels( text ) );
  rr::Typer typer = renderer.typer(
      centered( text_size, r + Delta{ .w = 1, .h = 1 } ),
      gfx::pixel::red() );
  typer.write( text );
  painter.draw_empty_rect( r, rr::Painter::e_border_mode::inside,
                           gfx::pixel::white() );
}

PositionedHarborSubView HarborExitButton::create(
    SS& ss, TS& ts, Player& player, Rect canvas,
    Coord market_upper_right, Coord market_lower_right ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborExitButton> view;
  HarborSubView*               harbor_sub_view = nullptr;

  Coord origin =
      market_lower_right - Delta{ .h = kExitBlockPixels.h };
  if( origin.x + kExitBlockPixels.w >= canvas.lower_right().x )
    // Move it up on top of the market commodities.
    origin =
        market_upper_right - kExitBlockPixels + Delta{ .h = 1 };
  origin -= Delta{ .w = 1, .h = 1 };

  view = make_unique<HarborExitButton>( ss, ts, player );
  harbor_sub_view = view.get();
  return PositionedHarborSubView{
      .owned  = { .view = std::move( view ), .coord = origin },
      .harbor = harbor_sub_view };
}

HarborExitButton::HarborExitButton( SS& ss, TS& ts,
                                    Player& player )
  : HarborSubView( ss, ts, player ) {}

} // namespace rn
