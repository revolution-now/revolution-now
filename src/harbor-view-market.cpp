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
#include "co-time.hpp"
#include "commodity.hpp"
#include "harbor-extra.hpp"
#include "harbor-units.hpp"
#include "harbor-view-status.hpp"
#include "igui.hpp"
#include "input.hpp"
#include "market.hpp"
#include "player-mgr.hpp"
#include "renderer.hpp"
#include "tax.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/text.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/old-world-state.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// render
#include "render/extra.hpp"
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

maybe<UnitId> HarborMarketCommodities::get_active_unit() const {
  return old_world_state( ss_, player_.type )
      .harbor_state.selected_unit;
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
      cheat_toggle_boycott( ss_, player_, type );
    co_return;
  }

  // If the commodity is boycotted this will allow the player to
  // pay the back taxes.
  (void)co_await check_boycott( type );
}

wait<bool> HarborMarketCommodities::perform_key(
    input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down )
    co_return false;
  switch( event.keycode ) {
    case ::SDLK_u:
      co_await unload_one();
      co_return true;
    default:
      break;
  }
  co_return false;
}

wait<> HarborMarketCommodities::unload_impl(
    UnitId const unit_id, Commodity const comm,
    int const slot ) {
  Unit& ship = ss_.units.unit_for( unit_id );
  rm_commodity_from_cargo( ss_.units, ship.cargo(), slot );
  co_await sell( comm );
}

wait<> HarborMarketCommodities::unload_one() {
  auto const unit_id = get_active_unit();
  if( !unit_id.has_value() ) co_return;
  if( !is_unit_in_port( ss_.units, *unit_id ) ) co_return;
  auto const unloadables =
      find_unloadable_slots_in_harbor( ss_, *unit_id );
  if( unloadables.items.empty() ) co_return;
  auto unloadable = unloadables.items[0];
  if( unloadable.boycott ) {
    // Give the player the opportunity to lift the boycott.
    int const back_tax = back_tax_for_boycotted_commodity(
        ss_, player_, unloadable.comm.type );
    unloadable.boycott = co_await try_trade_boycotted_commodity(
        ss_, ts_, player_, unloadable.comm.type, back_tax );
  }
  if( !unloadable.boycott )
    co_await unload_impl( *unit_id, unloadable.comm,
                          unloadable.slot );
}

wait<> HarborMarketCommodities::unload_all() {
  auto const unit_id = get_active_unit();
  if( !unit_id.has_value() ) co_return;
  if( !is_unit_in_port( ss_.units, *unit_id ) ) co_return;
  HarborUnloadables const unloadables =
      find_unloadable_slots_in_harbor( ss_, *unit_id );
  if( unloadables.items.empty() ) co_return;
  bool const has_boycotted_item = [&] {
    for( auto const& [slot, comm, boycott] : unloadables.items )
      if( boycott ) return true;
    return false;
  }();
  using namespace std::chrono_literals;
  chrono::milliseconds delay = 0ms;
  for( auto const& [slot, comm, boycott] : unloadables.items ) {
    if( boycott ) continue;
    co_await delay;
    delay = 500ms;
    bool const suspended =
        ( co_await co::detect_suspend(
              unload_impl( *unit_id, comm, slot ) ) )
            .suspended;
    if( suspended )
      // This typically means that a window popped up notifying
      // the player of a price change as a result of the sell. In
      // that case, we can skip the delay on the next one.
      delay = 0ms;
  }
  if( has_boycotted_item )
    co_await ts_.gui.message_box(
        "We were unable to unload all commodities because some "
        "of them are under boycott. Click on those boycotted "
        "commodities in the market to lift the boycott." );
  co_return;
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

void HarborMarketCommodities::clear_status_bar_msg() const {
  // This will clear the status bar but only if there is not a
  // transient message visible. That way, at the completion of a
  // drag where there is a transient message, it won't automati-
  // cally get removed by the fact that the cancel_drag method
  // (which always gets called at the end of a drag) will call
  // this. The only time we want to actually clear the status bar
  // is when the drag did not complete and still holds the
  // "sticky" status bar message that it shows while the drag is
  // in progress.
  harbor_status_bar_.inject_message(
      HarborStatusMsg::default_msg_ignore_when_transient{} );
}

void HarborMarketCommodities::send_purchase_info_to_status_bar(
    e_commodity const comm ) const {
  CommodityPrice const price =
      market_price( ss_, player_, comm );
  string const comm_name =
      uppercase_commodity_display_name( comm );
  string const msg = fmt::format(
      "Purchasing {} at {}{}", comm_name, price.ask * 100,
      config_text.special_chars.currency );
  harbor_status_bar_.inject_message(
      HarborStatusMsg::sticky_override{ .msg = msg } );
}

void HarborMarketCommodities::send_invoice_msg_to_status_bar(
    Invoice const& invoice ) const {
  e_transaction const buy_sell =
      invoice.money_delta_before_taxes < 0 ? e_transaction::buy
                                           : e_transaction::sell;
  string const comm_name =
      uppercase_commodity_display_name( invoice.what.type );
  string const msg = [&] {
    switch( buy_sell ) {
      case e_transaction::sell:
        return fmt::format(
            "{} {} sold for {}. {}% Tax: {}. Net: {}",
            invoice.what.quantity, comm_name,
            invoice.money_delta_before_taxes, invoice.tax_rate,
            invoice.tax_amount, invoice.money_delta_final );
      case e_transaction::buy:
        return fmt::format( "{} {} purchased for {}",
                            invoice.what.quantity, comm_name,
                            -invoice.money_delta_before_taxes );
    }
  }();
  harbor_status_bar_.inject_message(
      HarborStatusMsg::transient_override{ .msg = msg } );
}

void HarborMarketCommodities::send_no_afford_msg_to_status_bar(
    Commodity const& comm ) const {
  string const comm_name =
      uppercase_commodity_display_name( comm.type );
  string const msg =
      fmt::format( "{} tons of {} is too expensive!",
                   comm.quantity, comm_name );
  send_error_to_status_bar( msg );
}

void HarborMarketCommodities::send_error_to_status_bar(
    string const& err ) const {
  harbor_status_bar_.inject_message(
      HarborStatusMsg::transient_override{ .msg   = err,
                                           .error = true } );
}

wait<> HarborMarketCommodities::sell(
    Commodity const& comm ) const {
  // The player is selling. Here the market is officially ac-
  // cepting the goods from the player, and so we must pay the
  // player now.
  Invoice const invoice = transaction_invoice(
      ss_, player_, comm, e_transaction::sell,
      e_immediate_price_change_allowed::allowed );
  apply_invoice( ss_, player_, invoice );
  send_invoice_msg_to_status_bar( invoice );
  if( invoice.price_change.delta != 0 )
    co_await display_price_change_notification(
        ts_, player_, invoice.price_change );
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
  send_purchase_info_to_status_bar( comm.comm.type );
  return true;
}

void HarborMarketCommodities::cancel_drag() {
  dragging_ = nothing;
  clear_status_bar_msg();
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
  bool const boycott = old_world_state( ss_, player_.type )
                           .market.commodities[type]
                           .boycott;
  if( !boycott ) co_return false;
  int const back_tax =
      back_tax_for_boycotted_commodity( ss_, player_, type );
  co_return co_await try_trade_boycotted_commodity(
      ss_, ts_, player_, type, back_tax );
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
  if( -invoice.money_delta_final > player_.money ) {
    send_no_afford_msg_to_status_bar( comm );
    string reason = fmt::format(
        "We do not have enough in our treasury to purchase "
        "[{} {}].",
        comm.quantity,
        lowercase_commodity_display_name( comm.type ) );
    if( comm.quantity < 100 )
      reason += fmt::format(
          " Try further reducing the quantity of your purchase.",
          comm.quantity,
          lowercase_commodity_display_name( comm.type ) );
    else
      reason += fmt::format(
          " Try holding down the [shift] key while draggin to "
          "reduce the quantity of your purchase.",
          comm.quantity,
          lowercase_commodity_display_name( comm.type ) );
    co_return DragRejection{ .reason = std::move( reason ) };
  }
  co_return base::valid;
}

wait<> HarborMarketCommodities::disown_dragged_object() {
  UNWRAP_CHECK( comm, dragging_.member( &Draggable::comm ) );
  // The player is buying. Here we are officially releasing the
  // goods from the market, and so we must charge the player now.
  Invoice const invoice = transaction_invoice(
      ss_, player_, comm, e_transaction::buy,
      e_immediate_price_change_allowed::allowed );
  dragging_->invoice = invoice;
  apply_invoice( ss_, player_, invoice );
  co_return;
}

wait<> HarborMarketCommodities::post_successful_source(
    HarborDraggableObject const&, Coord const& ) {
  CHECK( dragging_.has_value() );
  send_invoice_msg_to_status_bar( dragging_->invoice );
  if( dragging_->invoice.price_change.delta != 0 )
    co_await display_price_change_notification(
        ts_, player_, dragging_->invoice.price_change );
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
  co_await sell( cargo_comm.comm );
}

void HarborMarketCommodities::draw( rr::Renderer& renderer,
                                    Coord const coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
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
    CommodityPrice const price =
        market_price( ss_, player_, comm );
    string const bid_str = to_string( price.bid );
    string const ask_str = to_string( price.ask );
    rr::Typer typer      = renderer.typer();
    typer.set_color( gfx::pixel::black() );
    size const bid_text_size =
        typer.dimensions_for_line( bid_str );
    size const ask_text_size =
        typer.dimensions_for_line( ask_str );
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
    typer.set_position( bid_text_origin );
    typer.write( bid_str );
    point const slash_origin =
        bid_text_origin + size{ .w = layout_.bid_ask_padding } +
        size{ .w = bid_text_size.w };
    render_sprite( renderer, slash_origin, layout_.slash_tile );
    point const ask_text_origin =
        slash_origin + size{ .w = layout_.bid_ask_padding } +
        size{ .w = layout_.slash_size.w };
    typer.set_position( ask_text_origin );
    typer.write( ask_str );

    if( old_world_state( ss_, player_.type )
            .market.commodities[comm]
            .boycott )
      render_sprite( renderer,
                     layout_.boycott_render_rect[comm].origin,
                     e_tile::red_x_12 );

    // Tooltip.
    // TODO: need to move this to the window plane and make sure
    // that it reflows the text so that it fits on screen. Maybe
    // we want to make some kind of general tooltip framework
    // that handles the delay and showing of the tooltip.
    if( mouse_pos.is_inside( layout_.panel_inner_rect[comm] ) ) {
      string const tooltip =
          fmt::format( "{}: Bidding {}, Asking {}",
                       uppercase_commodity_display_name( comm ),
                       price.bid, price.ask );
      size const tooltip_size =
          renderer.typer().dimensions_for_line( tooltip );
      point anchor = layout_.plates[comm].center().with_y(
          layout_.plates[comm].top() - 1 );
      e_cdirection placement = e_cdirection::s;
      if( anchor.x - tooltip_size.w / 2 < 0 ) {
        anchor.x  = 0;
        placement = e_cdirection::sw;
      }
      if( anchor.x + tooltip_size.w / 2 >=
          renderer.logical_screen_size().w ) {
        anchor.x  = renderer.logical_screen_size().w;
        placement = e_cdirection::se;
      }
      rr::render_text_line_with_background(
          renderer, rr::TextLayout{}, tooltip,
          gfx::oriented_point{ .anchor    = anchor,
                               .placement = placement },
          config_ui.tooltips.default_fg_color,
          config_ui.tooltips.default_bg_color, /*padding=*/1,
          /*draw_corners=*/false );
    }
  }
}

PositionedHarborSubView<HarborMarketCommodities>
HarborMarketCommodities::create(
    SS& ss, TS& ts, Player& player, Rect canvas,
    HarborStatusBar& harbor_status_bar ) {
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
  size const boycott_tile_size = sprite_size( e_tile::red_x_12 );
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
  view = make_unique<HarborMarketCommodities>(
      ss, ts, player, harbor_status_bar, layout );
  harbor_sub_view                   = view.get();
  HarborMarketCommodities* p_actual = view.get();
  return PositionedHarborSubView<HarborMarketCommodities>{
    .owned = { .view = std::move( view ), .coord = view_origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborMarketCommodities::HarborMarketCommodities(
    SS& ss, TS& ts, Player& player,
    HarborStatusBar& harbor_status_bar, Layout const& layout )
  : HarborSubView( ss, ts, player ),
    harbor_status_bar_( harbor_status_bar ),
    layout_( layout ) {}

} // namespace rn
