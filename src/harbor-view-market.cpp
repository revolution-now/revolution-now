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
#include "commodity.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// ss
#include "ss/player.rds.hpp"

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

maybe<DraggableObjectWithBounds<HarborDraggableObject_t>>
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
  return DraggableObjectWithBounds<HarborDraggableObject_t>{
      .obj =
          HarborDraggableObject::market_commodity{
              .comm = Commodity{ .type     = comm_type,
                                 .quantity = 100 } },
      .bounds = box };
}

bool HarborMarketCommodities::try_drag(
    HarborDraggableObject_t const& o, Coord const& ) {
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

wait<maybe<HarborDraggableObject_t>>
HarborMarketCommodities::user_edit_object() const {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );
  string const text = fmt::format(
      "What quantity of @[H]{}@[] would you like to buy? "
      "(0-100):",
      commodity_display_name( comm.type ) );

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

wait<base::valid_or<DragRejection>>
HarborMarketCommodities::source_check(
    HarborDraggableObject_t const&, Coord const ) const {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );

  // TODO: check for boycotts.

  PurchaseInvoice const invoice = cost_to_buy( player_, comm );
  if( invoice.cost > player_.money )
    co_return DragRejection{
        .reason = fmt::format(
            "You do not have enough gold to purchase @[H]{} "
            "{}@[].  Try holding down the @[H]shift@[] key to "
            "reduce the quantity of your purchase.",
            comm.quantity, comm.type ) };
  co_return base::valid;
}

wait<> HarborMarketCommodities::disown_dragged_object() {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );
  // The player is buying. Here we are officially releasing the
  // goods from the market, and so we must charge the player now.
  PurchaseInvoice const invoice = cost_to_buy( player_, comm );
  player_.money -= invoice.cost;
  CHECK_GE( player_.money, 0 );
  co_return;
}

wait<> HarborMarketCommodities::post_successful_source(
    HarborDraggableObject_t const& o, Coord const& ) {
  // TODO
  (void)o;
  co_return;
}

maybe<HarborDraggableObject_t>
HarborMarketCommodities::can_receive(
    HarborDraggableObject_t const& o, int /*from_entity*/,
    Coord const& /*where*/ ) const {
  if( o.holds<HarborDraggableObject::cargo_commodity>() )
    return o;
  return nothing;
}

wait<base::valid_or<DragRejection>>
HarborMarketCommodities::sink_check(
    HarborDraggableObject_t const&, int /*from_entity*/,
    Coord const ) const {
  // TODO: check for boycotts.
  co_return base::valid;
}

wait<> HarborMarketCommodities::drop(
    HarborDraggableObject_t const& o, Coord const& ) {
  UNWRAP_CHECK(
      cargo_comm,
      o.get_if<HarborDraggableObject::cargo_commodity>() );
  Commodity const& comm = cargo_comm.comm;
  // The player is selling. Here the market is officially ac-
  // cepting the goods from the player, and so we must pay the
  // player now.
  SaleInvoice const invoice = sale_transaction( player_, comm );
  player_.money += invoice.received_final;
}

wait<> HarborMarketCommodities::post_successful_sink(
    HarborDraggableObject_t const& o, int /*from_entity*/,
    Coord const& ) {
  // TODO
  (void)o;
  co_return;
}

void HarborMarketCommodities::draw( rr::Renderer& renderer,
                                    Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  auto        bds     = rect( coord );
  // Our delta for this view has one extra pixel added to the
  // width and height to allow for the border, and so we need to
  // remove that otherwise the to-grid method below will create
  // too many grid boxes.
  --bds.w;
  --bds.h;
  auto grid    = bds.to_grid_noalign( g_tile_delta );
  auto comm_it = refl::enum_values<e_commodity>.begin();
  auto label   = CommodityLabel::buy_sell{};
  for( Rect const rect : grid ) {
    CHECK( comm_it != refl::enum_values<e_commodity>.end() );
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
