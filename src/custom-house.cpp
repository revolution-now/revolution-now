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
#include "igui.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "views.hpp"
#include "window.hpp"

// config
#include "config/colony.rds.hpp"

// ss
#include "ss/colony.rds.hpp"

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> open_custom_house_menu( Planes& planes, TS& ts,
                               Colony& colony ) {
  using namespace ::rn::ui;
  auto top_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  // Add text.
  auto text_view = make_unique<TextView>(
      "What cargos shall our @[H]Custom House@[] export?" );
  top_array->add_view( std::move( text_view ) );

  // Add vertical split, since there are too many fathers to put
  // them all vertically.
  auto vsplit_array = make_unique<HorizontalArrayView>(
      HorizontalArrayView::align::up );

  // Add check boxes.
  auto l_boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );
  auto r_boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  refl::enum_map<e_commodity, LabeledCheckBoxView const*> boxes;
  for( e_commodity comm : refl::enum_values<e_commodity> ) {
    auto labeled_box = make_unique<LabeledCheckBoxView>(
        string( uppercase_commodity_display_name( comm ) ),
        colony.custom_house[comm] );
    boxes[comm] = labeled_box.get();
    if( static_cast<int>( comm ) < 8 )
      l_boxes_array->add_view( std::move( labeled_box ) );
    else
      r_boxes_array->add_view( std::move( labeled_box ) );
  }
  l_boxes_array->recompute_child_positions();
  r_boxes_array->recompute_child_positions();

  vsplit_array->add_view( std::move( l_boxes_array ) );
  vsplit_array->add_view( std::move( r_boxes_array ) );
  vsplit_array->recompute_child_positions();
  top_array->add_view( std::move( vsplit_array ) );

  // Add buttons.
  auto buttons_view          = make_unique<ui::OkCancelView2>();
  ui::OkCancelView2* buttons = buttons_view.get();
  top_array->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top_array->recompute_child_positions();

  // Create window.
  WindowManager& wm = planes.window().manager();
  Window         window( wm );
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

} // namespace rn
