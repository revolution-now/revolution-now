/****************************************************************
**harbor-view-market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: Market commodities UI element within the harbor
*              view.
*
*****************************************************************/
#include "harbor-view-market.hpp"

// Revolution Now
#include "cheat.hpp"
#include "commodity.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "tax.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborMarketCommodities
*****************************************************************/
Delta HarborMarketCommodities::delta() const {
  int x_boxes = 16;
  int y_boxes = 1;
  if( stacked_ ) {
    x_boxes = 8;
    y_boxes = 2;
  }
  // +1 in each dimension for the border.
  return Delta{ .w = 32 * x_boxes + 1, .h = 32 * y_boxes + 1 };
}

maybe<int> HarborMarketCommodities::entity() const {
  return static_cast<int>( e_harbor_view_entity::market );
}

ui::View& HarborMarketCommodities::view() noexcept {
  return *this;
}

ui::View const& HarborMarketCommodities::view() const noexcept {
  return *this;
}

wait<> HarborMarketCommodities::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( rect( {} ) ) );
  auto obj = object_here( event.pos );
  if( !obj.has_value() ) co_return;
  HarborDraggableObject const& harbor_obj = obj->obj;
  UNWRAP_CHECK(
      comm,
      harbor_obj
          .get_if<HarborDraggableObject::market_commodity>() );
  e_commodity const type = comm.comm.type;

  if( event.mod.shf_down ) {
    // Cheat functions.
    cheat_toggle_boycott( player_, type );
    co_return;
  }

  // If the commodity is boycotted this will allow the player to
  // pay the back taxes.
  (void)co_await check_boycott( type );
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborMarketCommodities::object_here(
    Coord const& where ) const {
  maybe<pair<e_commodity, Rect>> res;

  Rect const r     = rect( Coord{} );
  Rect const boxes = r / g_tile_delta;
  UNWRAP_CHECK( comm_idx,
                boxes.rasterize( where / g_tile_delta ) );
  UNWRAP_CHECK( comm_type, commodity_from_index( comm_idx ) );
  Coord const box_origin =
      where.rounded_to_multiple_to_minus_inf( g_tile_delta ) +
      kCommodityInCargoHoldRenderingOffset;
  Rect const box =
      Rect::from( box_origin, Delta{ .w = 1, .h = 1 } *
                                  Delta{ .w = 16, .h = 16 } );
  return DraggableObjectWithBounds<HarborDraggableObject>{
      .obj =
          HarborDraggableObject::market_commodity{
              .comm = Commodity{ .type     = comm_type,
                                 .quantity = 100 } },
      .bounds = box };
}

bool HarborMarketCommodities::try_drag(
    HarborDraggableObject const& o, Coord const& ) {
  UNWRAP_CHECK(
      comm,
      o.get_if<HarborDraggableObject::market_commodity>() );
  dragging_ = Draggable{ .comm = comm.comm };
  CHECK_GT( dragging_->comm.quantity, 0 );
  // Note that we allow this even if there is an active boycott
  // on the commodity, since we will handle that in the interac-
  // tive confirmation process.
  return true;
}

void HarborMarketCommodities::cancel_drag() {
  dragging_ = nothing;
}

wait<maybe<HarborDraggableObject>>
HarborMarketCommodities::user_edit_object() const {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );
  string const text = fmt::format(
      "What quantity of [{}] would you like to buy? "
      "(0-100):",
      lowercase_commodity_display_name( comm.type ) );

  maybe<int> const quantity =
      co_await ts_.gui.optional_int_input(
          { .msg           = text,
            .initial_value = 100,
            .min           = 0,
            .max           = 100 } );
  if( !quantity.has_value() ) co_return nothing;
  if( quantity == 0 ) co_return nothing;
  // We shouldn't have to update the dragging_ member here be-
  // cause the framework should call try_drag again with the mod-
  // ified value.
  Commodity new_comm = comm;
  new_comm.quantity  = *quantity;
  CHECK( new_comm.quantity > 0 );
  co_return HarborDraggableObject::market_commodity{
      .comm = new_comm };
}

wait<base::NoDiscard<bool>>
HarborMarketCommodities::check_boycott( e_commodity type ) {
  bool const& boycott =
      player_.old_world.market.commodities[type].boycott;
  if( !boycott ) co_return false;
  int const back_tax =
      back_tax_for_boycotted_commodity( player_, type );
  co_await try_trade_boycotted_commodity( ts_, player_, type,
                                          back_tax );
  co_return boycott;
}

wait<base::valid_or<DragRejection>>
HarborMarketCommodities::source_check(
    HarborDraggableObject const&, Coord const ) {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );

  // Need to check for boycotts before checking the player money
  // because the player might be charged to lift the boycott.
  if( co_await check_boycott( comm.type ) )
    co_return DragRejection{};

  Invoice const invoice = transaction_invoice(
      ss_, player_, comm, e_transaction::buy,
      e_immediate_price_change_allowed::allowed );
  CHECK_LE( invoice.money_delta_final, 0 );
  if( -invoice.money_delta_final > player_.money )
    co_return DragRejection{
        .reason = fmt::format(
            "You do not have enough gold to purchase [{} {}].  "
            "Try holding down the [shift] key to reduce the "
            "quantity of your purchase.",
            comm.quantity,
            lowercase_commodity_display_name( comm.type ) ) };
  co_return base::valid;
}

wait<> HarborMarketCommodities::disown_dragged_object() {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );
  // The player is buying. Here we are officially releasing the
  // goods from the market, and so we must charge the player now.
  Invoice const invoice = transaction_invoice(
      ss_, player_, comm, e_transaction::buy,
      e_immediate_price_change_allowed::allowed );
  dragging_->price_change = invoice.price_change;
  apply_invoice( ss_, player_, invoice );
  co_return;
}

wait<> HarborMarketCommodities::post_successful_source(
    HarborDraggableObject const&, Coord const& ) {
  CHECK( dragging_.has_value() );
  if( dragging_->price_change.delta != 0 )
    co_await display_price_change_notification(
        ts_, player_, dragging_->price_change );
}

maybe<HarborDraggableObject>
HarborMarketCommodities::can_receive(
    HarborDraggableObject const& o, int /*from_entity*/,
    Coord const& /*where*/ ) const {
  if( o.holds<HarborDraggableObject::cargo_commodity>() )
    return o;
  return nothing;
}

wait<base::valid_or<DragRejection>>
HarborMarketCommodities::sink_check(
    HarborDraggableObject const& o, int /*from_entity*/,
    Coord const ) {
  CHECK( o.holds<HarborDraggableObject::cargo_commodity>() );
  e_commodity const type =
      o.get<HarborDraggableObject::cargo_commodity>().comm.type;

  if( co_await check_boycott( type ) ) co_return DragRejection{};

  co_return base::valid;
}

wait<> HarborMarketCommodities::drop(
    HarborDraggableObject const& o, Coord const& ) {
  UNWRAP_CHECK(
      cargo_comm,
      o.get_if<HarborDraggableObject::cargo_commodity>() );
  Commodity const& comm = cargo_comm.comm;
  // The player is selling. Here the market is officially ac-
  // cepting the goods from the player, and so we must pay the
  // player now.
  Invoice const invoice = transaction_invoice(
      ss_, player_, comm, e_transaction::sell,
      e_immediate_price_change_allowed::allowed );
  apply_invoice( ss_, player_, invoice );
  if( invoice.price_change.delta != 0 )
    co_await display_price_change_notification(
        ts_, player_, invoice.price_change );
}

void HarborMarketCommodities::draw( rr::Renderer& renderer,
                                    Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  auto        bds     = rect( coord );
  // Our delta for this view has one extra pixel added to the
  // width and height to allow for the border, and so we need to
  // remove that otherwise the subrects method below will create
  // too many boxes.
  --bds.w;
  --bds.h;
  auto comm_it = refl::enum_values<e_commodity>.begin();
  auto label   = CommodityLabel::buy_sell{};
  for( Rect const rect : gfx::subrects( bds, g_tile_delta ) ) {
    CHECK( comm_it != refl::enum_values<e_commodity>.end() );
    // FIXME: this color should be deduped with the one in the
    // colony view.
    static gfx::pixel const bg_color =
        gfx::pixel{ .r = 0x90, .g = 0x90, .b = 0xc0, .a = 0xff };
    painter.draw_solid_rect( rect, bg_color );
    painter.draw_empty_rect( rect,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::white() );
    CommodityPrice price = market_price( player_, *comm_it );
    label.bid            = price.bid;
    label.ask            = price.ask;
    render_commodity_annotated(
        renderer,
        rect.upper_left() + kCommodityInCargoHoldRenderingOffset,
        *comm_it, label );
    if( player_.old_world.market.commodities[*comm_it].boycott )
      render_sprite( painter,
                     rect.upper_left() +
                         kCommodityInCargoHoldRenderingOffset,
                     e_tile::boycott );
    ++comm_it;
  }
  CHECK( comm_it == refl::enum_values<e_commodity>.end() );
}

PositionedHarborSubView<HarborMarketCommodities>
HarborMarketCommodities::create( SS& ss, TS& ts, Player& player,
                                 Rect canvas ) {
  Delta const size = canvas.delta();
  W           comm_block_width =
      size.w / SX{ refl::enum_count<e_commodity> };
  comm_block_width =
      clamp( comm_block_width, kCommodityTileSize.w, 32 );
  unique_ptr<HarborMarketCommodities> view;
  HarborSubView*                      harbor_sub_view = nullptr;
  Coord                               pos;
  bool                                stacked = false;
  if( size.w >= HarborMarketCommodities::single_layer_width &&
      size.h >= HarborMarketCommodities::single_layer_height ) {
    stacked = false;
    pos     = Coord{
            .x = canvas.center().x -
             HarborMarketCommodities::single_layer_width / 2,
            .y = canvas.bottom_edge() -
             HarborMarketCommodities::single_layer_height - 1 };
  } else {
    stacked = true;
    pos     = Coord{
            .x = canvas.center().x -
             HarborMarketCommodities::double_layer_width / 2,
            .y = canvas.bottom_edge() -
             HarborMarketCommodities::double_layer_height - 1 };
  };
  view = make_unique<HarborMarketCommodities>( ss, ts, player,
                                               stacked );
  harbor_sub_view                   = view.get();
  HarborMarketCommodities* p_actual = view.get();
  return PositionedHarborSubView<HarborMarketCommodities>{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborMarketCommodities::HarborMarketCommodities( SS& ss, TS& ts,
                                                  Player& player,
                                                  bool stacked )
  : HarborSubView( ss, ts, player ), stacked_( stacked ) {}

} // namespace rn
