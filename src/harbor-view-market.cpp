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
#include "input.hpp"
#include "market.hpp"
#include "renderer.hpp"
#include "tax.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"

// render
#include "render/typer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

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
** HarborMarketCommodities
*****************************************************************/
Delta HarborMarketCommodities::delta() const {
  size sz;
  sz.w = layout_.left_sign.size.w;
  sz.w += layout_.exit_sign.size.w;
  sz.w += 16 * layout_.plates[e_commodity::food].size.w;
  sz.h = layout_.left_sign.size.h;
  return sz;
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
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  if( event.pos.is_inside( layout_.exit_sign ) )
    throw harbor_view_exit_interrupt{};
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
    if( cheat_mode_enabled( ss_ ) )
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
  auto const comm_type = [&]() -> maybe<e_commodity> {
    for( auto const comm : enum_values<e_commodity> ) {
      if( point( where ).is_inside( layout_.plates[comm] ) )
        return comm;
    }
    return nothing;
  }();
  if( !comm_type.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj =
        HarborDraggableObject::market_commodity{
          .comm =
              Commodity{ .type = *comm_type, .quantity = 100 } },
    .bounds = layout_.comm_icon[*comm_type] };
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
      "What quantity of [{}] would you like to buy? (0-100):",
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
          "We do not have enough in our treasury to purchase "
          "[{} {}]. Try holding down the [shift] key to "
          "reduce the quantity of your purchase.",
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

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborMarketCommodities::can_receive(
    HarborDraggableObject const& o, int /*from_entity*/,
    Coord const& /*where*/ ) const {
  if( o.holds<HarborDraggableObject::cargo_commodity>() )
    return CanReceiveDraggable<HarborDraggableObject>::yes{
      .draggable = o };
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
                                    Coord const coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation,
      point( coord ).distance_from_origin().to_double() );

  render_sprite( renderer, layout_.left_sign.nw(),
                 layout_.left_sign_tile );
  render_sprite( renderer, layout_.exit_sign.nw(),
                 layout_.exit_sign_tile );

  point const mouse_pos = input::current_mouse_position()
                              .to_gfx()
                              .point_becomes_origin( coord );

  bool const hover_over_exit =
      mouse_pos.is_inside( layout_.exit_sign );
  render_sprite( renderer, layout_.exit_text.nw(),
                 hover_over_exit ? e_tile::harbor_exit_text_hover
                                 : e_tile::harbor_exit_text );

  for( auto const comm : enum_values<e_commodity> ) {
    auto const& r = layout_.plates[comm];
    render_sprite( renderer, r.nw(), layout_.comm_plate_tile );
    if( mouse_pos.is_inside( layout_.panel_inner_rect[comm] ) ) {
      // Glow behind commodity.
      render_sprite( renderer, layout_.comm_icon[comm].origin,
                     e_tile::commodity_glow_20 );
      render_commodity_20_outline(
          renderer, layout_.comm_icon[comm].origin, comm,
          pixel::from_hex_rgb( 0x7ba9c9 ) );
    } else {
      render_commodity_20(
          renderer, layout_.comm_icon[comm].origin, comm );
    }

#if 0
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( layout_.bid_ask[comm],
                             gfx::pixel::red() );
#endif
    CommodityPrice const price = market_price( player_, comm );
    string const bid_str       = to_string( price.bid );
    string const ask_str       = to_string( price.ask );
    size const bid_text_size =
        rr::rendered_text_line_size_pixels( bid_str );
    size const ask_text_size =
        rr::rendered_text_line_size_pixels( ask_str );
    size const total_size{
      .w = bid_text_size.w + layout_.bid_ask_padding +
           layout_.slash_size.w + +layout_.bid_ask_padding +
           ask_text_size.w,
      .h =
          std::max( std::max( bid_text_size.h, ask_text_size.h ),
                    layout_.slash_size.h ) };
    point const label_origin =
        gfx::centered_in( total_size, layout_.bid_ask[comm] );

    point const bid_text_origin = label_origin;
    renderer.typer( bid_text_origin, gfx::pixel::black() )
        .write( bid_str );
    point const slash_origin =
        bid_text_origin + size{ .w = layout_.bid_ask_padding } +
        size{ .w = bid_text_size.w };
    render_sprite( renderer, slash_origin, layout_.slash_tile );
    point const ask_text_origin =
        slash_origin + size{ .w = layout_.bid_ask_padding } +
        size{ .w = layout_.slash_size.w };
    renderer.typer( ask_text_origin, gfx::pixel::black() )
        .write( ask_str );

    if( player_.old_world.market.commodities[comm].boycott )
      render_sprite( renderer,
                     layout_.boycott_render_rect[comm].origin,
                     e_tile::boycott );

    // Tooltip.
    // TODO: need to move this to the window plane and make sure
    // that it reflows the text so that it fits on-screen.
    if( mouse_pos.is_inside( layout_.panel_inner_rect[comm] ) ) {
      string const tooltip =
          fmt::format( "{} (Bidding {}, Asking {})",
                       uppercase_commodity_display_name( comm ),
                       price.bid, price.ask );
      size const tooltip_size =
          rr::rendered_text_line_size_pixels( tooltip );
      point const tooltip_origin = gfx::centered_at_bottom(
          tooltip_size,
          layout_.plates[comm].with_new_bottom_edge(
              layout_.plates[comm].top() ) );
      renderer.typer( tooltip_origin, gfx::pixel::black() )
          .write( tooltip );
    }
  }
}

PositionedHarborSubView<HarborMarketCommodities>
HarborMarketCommodities::create( SS& ss, TS& ts, Player& player,
                                 Rect canvas ) {
  size const sz = canvas.delta();

  Layout layout;

  // Scale.
  e_harbor_market_scale const scale = [&] {
    if( sz.w < 640 ) return e_harbor_market_scale::slim;
    if( sz.w >= 768 ) return e_harbor_market_scale::wide;
    return e_harbor_market_scale::medium;
  }();
  layout.scale = scale;

  size const boycott_render_offset   = { .w = 2, .h = 2 };
  size const panel_inner_rect_offset = { .w = 3, .h = 4 };

  switch( scale ) {
    case e_harbor_market_scale::slim:
      layout.left_sign_tile =
          e_tile::harbor_market_sign_left_slim;
      layout.exit_sign_tile =
          e_tile::harbor_market_sign_exit_slim;
      layout.comm_plate_tile = e_tile::harbor_market_plate_slim;
      for( auto const comm : enum_values<e_commodity> )
        layout.panel_inner_rect[comm].size = { .w = 26,
                                               .h = 21 };
      for( auto const comm : enum_values<e_commodity> )
        layout.bid_ask[comm] = { .origin = { .x = 4, .y = 29 },
                                 .size   = { .w = 24, .h = 6 } };
      layout.slash_tile      = e_tile::harbor_market_slash_slim;
      layout.slash_size      = { .w = 3, .h = 8 };
      layout.bid_ask_padding = 0;
      break;
    case e_harbor_market_scale::medium:
      layout.left_sign_tile =
          e_tile::harbor_market_sign_left_slim;
      layout.exit_sign_tile =
          e_tile::harbor_market_sign_exit_slim;
      layout.comm_plate_tile =
          e_tile::harbor_market_plate_medium;
      for( auto const comm : enum_values<e_commodity> )
        layout.panel_inner_rect[comm].size = { .w = 30,
                                               .h = 21 };
      for( auto const comm : enum_values<e_commodity> )
        layout.bid_ask[comm] = { .origin = { .x = 5, .y = 29 },
                                 .size   = { .w = 26, .h = 6 } };
      layout.slash_tile = e_tile::harbor_market_slash_medium;
      layout.slash_size = { .w = 4, .h = 8 };
      layout.bid_ask_padding = 0;
      break;
    case e_harbor_market_scale::wide:
      layout.left_sign_tile =
          e_tile::harbor_market_sign_left_wide;
      layout.exit_sign_tile =
          e_tile::harbor_market_sign_exit_wide;
      layout.comm_plate_tile = e_tile::harbor_market_plate_wide;
      for( auto const comm : enum_values<e_commodity> )
        layout.panel_inner_rect[comm].size = { .w = 34,
                                               .h = 25 };
      for( auto const comm : enum_values<e_commodity> )
        layout.bid_ask[comm] = { .origin = { .x = 5, .y = 33 },
                                 .size   = { .w = 30, .h = 6 } };
      layout.slash_tile      = e_tile::harbor_market_slash_wide;
      layout.slash_size      = { .w = 4, .h = 8 };
      layout.bid_ask_padding = 1;
      break;
  }

  size const panel_tile_size =
      sprite_size( layout.comm_plate_tile );
  size const comm_panels_size = { .w = panel_tile_size.w * 16,
                                  .h = panel_tile_size.h };
  point const comm_panels_nw =
      centered_at_bottom( comm_panels_size, canvas );
  rn::rect const comm_panels_rect{ .origin = comm_panels_nw,
                                   .size   = comm_panels_size };
  for( int i = 0; auto const comm : enum_values<e_commodity> ) {
    layout.plates[comm].origin.x =
        comm_panels_nw.x + panel_tile_size.w * i;
    layout.plates[comm].origin.y = comm_panels_nw.y;
    layout.plates[comm].size     = panel_tile_size;
    ++i;
  }

  // Make the bid/ask text rects relative to the plate nw.
  for( auto const comm : enum_values<e_commodity> )
    layout.bid_ask[comm].origin +=
        layout.plates[comm].origin.distance_from_origin();

  size const left_sign_size =
      sprite_size( layout.left_sign_tile );
  point const left_sign_nw = {
    .x = comm_panels_nw.x - left_sign_size.w,
    .y = comm_panels_nw.y };
  point const view_origin = left_sign_nw;

  size const exit_sign_size =
      sprite_size( layout.exit_sign_tile );
  point const exit_sign_nw = comm_panels_rect.ne();
  size const exit_text_size =
      sprite_size( e_tile::harbor_exit_text );
  layout.exit_text.origin = gfx::centered_in(
      exit_text_size,
      rect{ .origin = exit_sign_nw, .size = exit_sign_size } );
  layout.exit_text.origin.y += 2; // looks better.

  size const comm_tile_size =
      sprite_size( e_tile::commodity_food_20 );
  size const boycott_tile_size = sprite_size( e_tile::boycott );
  for( auto const comm : enum_values<e_commodity> ) {
    layout.panel_inner_rect[comm].origin =
        layout.plates[comm].origin + panel_inner_rect_offset;
    layout.comm_icon[comm].origin = gfx::centered_in(
        comm_tile_size, layout.panel_inner_rect[comm] );
    layout.comm_icon[comm].size = comm_tile_size;
    layout.boycott_render_rect[comm].origin =
        layout.comm_icon[comm].origin + boycott_render_offset;
    layout.boycott_render_rect[comm].size = boycott_tile_size;
  }

  // Now make all points relative to nw of view (left sign).

  for( auto const comm : enum_values<e_commodity> )
    layout.plates[comm].origin =
        layout.plates[comm].origin.point_becomes_origin(
            view_origin );

  for( auto const comm : enum_values<e_commodity> )
    layout.panel_inner_rect[comm].origin =
        layout.panel_inner_rect[comm]
            .origin.point_becomes_origin( view_origin );

  layout.left_sign = {
    .origin = left_sign_nw.point_becomes_origin( view_origin ),
    .size   = left_sign_size };

  layout.exit_sign = {
    .origin = exit_sign_nw.point_becomes_origin( view_origin ),
    .size   = exit_sign_size };
  layout.exit_text.origin =
      layout.exit_text.origin.point_becomes_origin(
          view_origin );

  for( auto const comm : enum_values<e_commodity> ) {
    layout.comm_icon[comm].origin =
        layout.comm_icon[comm].origin.point_becomes_origin(
            view_origin );
    layout.boycott_render_rect[comm].origin =
        layout.boycott_render_rect[comm]
            .origin.point_becomes_origin( view_origin );
  }

  for( auto const comm : enum_values<e_commodity> )
    layout.bid_ask[comm].origin =
        layout.bid_ask[comm].origin.point_becomes_origin(
            view_origin );

  unique_ptr<HarborMarketCommodities> view;
  HarborSubView* harbor_sub_view = nullptr;
  view = make_unique<HarborMarketCommodities>( ss, ts, player,
                                               layout );
  harbor_sub_view                   = view.get();
  HarborMarketCommodities* p_actual = view.get();
  return PositionedHarborSubView<HarborMarketCommodities>{
    .owned = { .view = std::move( view ), .coord = view_origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborMarketCommodities::HarborMarketCommodities(
    SS& ss, TS& ts, Player& player, Layout const& layout )
  : HarborSubView( ss, ts, player ), layout_( layout ) {}

} // namespace rn
