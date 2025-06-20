/****************************************************************
**game-ui-views.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-13.
*
* Description: Contains high-level game-specific UI Views.
*
*****************************************************************/
#include "game-ui-views.hpp"

// Revolution Now
#include "unit-mgr.hpp"
#include "unit-stack.hpp"

// config
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

/****************************************************************
** UnitActivationView
*****************************************************************/
UnitActivationView::UnitActivationView(
    UnitActivationOptions const& opts )
  : opts_( opts ), info_map_{} {}

void UnitActivationView::on_click_unit( UnitId id ) {
  auto& infos = info_map();
  CHECK( infos.contains( id ) );
  UnitActivationInfo& info = infos[id];
  if( info.original_orders.to_enum() != unit_orders::e::none ) {
    // Orders --> No Orders --> No Orders+Prio --> ...
    if( info.current_orders.to_enum() != unit_orders::e::none ) {
      CHECK( !info.is_activated );
      info.current_orders = unit_orders::none{};
    } else if( info.is_activated ) {
      CHECK( info.current_orders.to_enum() ==
             unit_orders::e::none );
      info.current_orders = info.original_orders;
      info.is_activated   = false;
    } else {
      CHECK( !info.is_activated );
      if( !opts_.allow_prioritizing_multiple )
        for( auto& [_, other] : infos )
          other.is_activated = false;
      info.is_activated = true;
    }
  } else {
    // No Orders --> No Orders+Prioritized --> ...
    CHECK( info.original_orders.to_enum() ==
           unit_orders::e::none );
    CHECK( info.current_orders.to_enum() ==
           unit_orders::e::none );
    bool const will_activate = !info.is_activated;
    if( will_activate && !opts_.allow_prioritizing_multiple )
      for( auto& [_, other] : infos ) other.is_activated = false;
    info.is_activated = will_activate;
  }
}

/*
 * This function assembles the following view structure:
 *
 * OkCancelAdapter
 * +-VerticalScrollView // TODO
 *   +-VerticalArrayView
 *     |-...
 *     |-HorizontalArrayView
 *     | |-OneLineTextView
 *     | +-AddSelectBorderView
 *     |   +-ClickableView
 *     |     +-FakeUnitView
 *     |       +-SpriteView
 *     +-...
 */
unique_ptr<UnitActivationView> UnitActivationView::Create(
    rr::ITextometer const& textometer, SSConst const& ss,
    vector<UnitId> const& ids_,
    UnitActivationOptions const& opts ) {
  auto unit_activation_view =
      std::make_unique<UnitActivationView>( opts );
  auto* p_unit_activation_view = unit_activation_view.get();

  auto ids = ids_;
  CHECK( !ids.empty() );
  sort_euro_unit_stack( ss, ids );

  vector<unique_ptr<View>> units_vec;

  auto& infos = unit_activation_view->info_map();
  for( auto id : ids ) {
    auto const& unit = ss.units.unit_for( id );
    infos[id] =
        UnitActivationInfo{ /*original_orders=*/unit.orders(),
                            /*current_orders=*/unit.orders(),
                            /*is_activated=*/false };
  }
  CHECK( ids.size() == infos.size() );

  /* Each click on a unit cycles it through one the following cy-
   * cles depending on whether it initially had orders and
   * whether or not it is the end of turn:
   *
   * Orders=Fortified:
   *   Not End-of-turn:
   *     Fortified --> No Orders --> No Orders+Prio --> ...
   *   End-of-turn:
   *     Fortified --> No Orders --> ...
   * Orders=None:
   *   Not End-of-turn:
   *     No Orders --> No Orders+Prioritized --> ...
   *   End-of-turn:
   *     No Orders --> ...
   */

  // FIXME: this shared_ptr is a hack because this code was
  // written before coroutines, which would have kept this vari-
  // able in scope so that it could have just been referenced
  // normally below.
  auto const update_border_fns =
      make_shared<vector<function<void()>>>();
  update_border_fns->reserve( ids.size() );

  for( auto id : ids ) {
    auto const& unit = ss.units.unit_for( id );

    auto fake_unit_view = make_unique<ui::FakeUnitView>(
        unit.desc().type, unit.player_type(), unit.orders() );
    auto* p_fake_unit_view = fake_unit_view.get();

    auto border_view = make_unique<ui::BorderView>(
        std::move( fake_unit_view ), gfx::pixel::white(),
        /*padding=*/1,
        /*on_initially=*/false );
    auto* p_border_view = border_view.get();

    auto clickable = make_unique<ui::ClickableView>(
        // Capture local variables by value.
        std::move( border_view ),
        [update_border_fns, p_unit_activation_view, id,
         p_fake_unit_view] {
          auto& infos = p_unit_activation_view->info_map();
          p_unit_activation_view->on_click_unit( id );
          CHECK( infos.contains( id ) );
          auto& info = infos[id];
          p_fake_unit_view->set_orders( info.current_orders );
          // Need to update all of the borders in case we are in
          // the mode (set in the options) where we only allow
          // one unit to be prioritized at a time; in that case,
          // prioritizing one unit will deselect any others.
          for( auto const& fn : *update_border_fns ) fn();
        } );

    update_border_fns->push_back(
        [p_border_view, &info = infos[id]] {
          p_border_view->on( info.is_activated );
        } );

    auto unit_label = make_unique<ui::OneLineStringView>(
        textometer, unit.desc().name,
        config_ui.dialog_text.normal );

    vector<unique_ptr<View>> horizontal_vec;
    horizontal_vec.emplace_back( std::move( clickable ) );
    horizontal_vec.emplace_back( std::move( unit_label ) );
    auto horizontal_view = make_unique<ui::HorizontalArrayView>(
        std::move( horizontal_vec ),
        ui::HorizontalArrayView::align::middle );

    units_vec.emplace_back( std::move( horizontal_view ) );
  }
  auto arr_view = make_unique<ui::VerticalArrayView>(
      std::move( units_vec ),
      ui::VerticalArrayView::align::left );

  unit_activation_view->set_view( std::move( arr_view ),
                                  Coord{} );
  return unit_activation_view;
}

} // namespace rn
