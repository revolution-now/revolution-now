/****************************************************************
**custom-house.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-22.
*
* Description: All things related to the custom house.
*
*****************************************************************/
#include "custom-house.hpp"

using namespace std;

// Revolution Now
#include "commodity.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "views.hpp"
#include "window.hpp"

// config
#include "config/colony.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/player.rds.hpp"

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> open_custom_house_menu( IEngine& engine, TS& ts,
                               Colony& colony ) {
  using namespace ::rn::ui;
  auto const& textometer = engine.textometer();
  auto top_array         = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  // Add text.
  auto text_view = make_unique<TextView>(
      textometer,
      "What cargos shall our [Custom House] export?" );
  top_array->add_view( std::move( text_view ) );

  // Add vertical split, since there are too many fathers to put
  // them all vertically.
  auto vsplit_array = make_unique<HorizontalArrayView>(
      HorizontalArrayView::align::up );

  // Add check boxes into four columns.
  vector<unique_ptr<VerticalArrayView>> boxes_arrays;
  int constexpr kNumColumns = 4;
  for( int i = 0; i < kNumColumns; ++i )
    boxes_arrays.push_back( make_unique<VerticalArrayView>(
        VerticalArrayView::align::left ) );

  refl::enum_map<e_commodity, LabeledCheckBoxView const*> boxes;
  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    auto labeled_box = make_unique<TextLabeledCheckBoxView>(
        textometer,
        string( uppercase_commodity_display_name( comm ) ),
        colony.custom_house[comm] );
    boxes[comm]      = labeled_box.get();
    int const column = static_cast<int>( comm ) / 4;
    CHECK_LT( column, kNumColumns );
    boxes_arrays[column]->add_view( std::move( labeled_box ) );
  }
  for( int i = 0; i < kNumColumns; ++i ) {
    boxes_arrays[i]->recompute_child_positions();
    vsplit_array->add_view( std::move( boxes_arrays[i] ) );
  }

  vsplit_array->recompute_child_positions();
  top_array->add_view( std::move( vsplit_array ) );

  // Add buttons.
  auto buttons_view =
      make_unique<ui::OkCancelView2>( textometer );
  ui::OkCancelView2* buttons = buttons_view.get();
  top_array->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top_array->recompute_child_positions();

  // Create window.
  WindowManager& wm = ts.planes.get().window.typed().manager();
  Window window( wm );
  window.set_view( std::move( top_array ) );
  window.autopad_me();
  // Must be done after auto-padding.
  window.center_me();

  ui::e_ok_cancel const finished = co_await buttons->next();
  if( finished == ui::e_ok_cancel::cancel ) co_return;

  for( auto [comm, box] : boxes )
    colony.custom_house[comm] = box->on();
}

void set_default_custom_house_state( Colony& colony ) {
  colony.custom_house =
      config_colony.custom_house.initial_commodities;
}

CustomHouseSales compute_custom_house_sales(
    SSConst const& ss, Player const& player,
    Colony const& colony ) {
  CustomHouseSales sales;
  if( !colony.buildings[e_colony_building::custom_house] )
    return sales;
  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    if( !colony.custom_house[comm] ) continue;
    // The custom house is selling this commodity.
    int const quantity = colony.commodities[comm];
    if( quantity <
        config_colony.custom_house.threshold_for_sell )
      continue;
    int const amount_to_sell =
        colony.commodities[comm] -
        config_colony.custom_house.amount_to_retain;
    CHECK_GE( amount_to_sell, 0 );
    Commodity const to_sell{ .type     = comm,
                             .quantity = amount_to_sell };
    // Now we need to get the player some money and let the sales
    // affect market prices.
    //
    // Note that we suppress immediate price changes here, which
    // is not typical when calling this function. This is impor-
    // tant because, although we want the custom house sales to
    // affect the market, we don't want the current price to be
    // updated immediately upon making the transaction; we will
    // suppress that and just let the changed market state cause
    // any price changes at the start of the next turn when price
    // changes are assessed. That way we can apply the market
    // changes here (for each colony) and not worry about
    // breaking the rule that the price can only move by at most
    // one unit per turn.
    Invoice invoice = transaction_invoice(
        ss, player, to_sell, e_transaction::sell,
        e_immediate_price_change_allowed::suppressed );
    // In the OG, after independence is declared, there is a 50%
    // charge on all goods sold via custom house ("smuggling
    // fee") instead of the tax rate. Or at least there is sup-
    // posed to be... it may not be working right in the OG.
    if( player.revolution_status >
        e_revolution_status::not_declared ) {
      invoice.tax_rate =
          config_colony.custom_house.charge_during_revolution;
      invoice.tax_amount =
          int( invoice.tax_rate *
               ( invoice.money_delta_before_taxes / 100.0 ) );
      invoice.money_delta_final =
          invoice.money_delta_before_taxes - invoice.tax_amount;
    }
    if( config_colony.custom_house.respect_boycotts &&
        player.old_world.market.commodities[comm].boycott ) {
      // In this game the custom house cannot by default sell
      // goods that are boycotted. The OG seems to ignore boy-
      // cotts when selling via custom house, but this is prob-
      // ably a bug, since the strategy guide does not mention
      // boycott-resistance as being among the abilities of the
      // custom house (FreeCol suggests its a bug as well). So in
      // this game we have this config flag that controls it, and
      // it defaults to making the custom house respect boycotts.
      // That means that the custom house will simply not sell
      // goods that are boycotted.
      //
      // Here we only push boycotted items that we were otherwise
      // going to sell.
      sales.boycotted.push_back( comm );
      continue;
    }
    sales.invoices.push_back( invoice );
  }
  return sales;
}

void apply_custom_house_sales( SS& ss, Player& player,
                               Colony& colony,
                               CustomHouseSales const& sales ) {
  for( Invoice const& invoice : sales.invoices ) {
    // The custom house is selling this commodity.
    int& quantity = colony.commodities[invoice.what.type];
    // The custom house will only sell when the quantity becomes
    // larger than a threshold, which in turn is larger than the
    // amount that the colony retains, and it will sell as much
    // as it can to leave only the minimum retaining amount.
    // Therefore, we can just set the quantity to the minimum re-
    // taining amount.
    quantity = config_colony.custom_house.amount_to_retain;
    apply_invoice( ss, player, invoice );
  }
}

} // namespace rn
