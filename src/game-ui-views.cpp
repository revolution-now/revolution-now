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
UnitActivationView::UnitActivationView() : info_map_{} {}

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
      info.is_activated = true;
    }
  } else {
    // No Orders --> No Orders+Prioritized --> ...
    CHECK( info.original_orders.to_enum() ==
           unit_orders::e::none );
    CHECK( info.current_orders.to_enum() ==
           unit_orders::e::none );
    info.is_activated = !info.is_activated;
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
    vector<UnitId> const& ids_ ) {
  auto unit_activation_view =
      std::make_unique<UnitActivationView>();
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
        [p_unit_activation_view, id, p_fake_unit_view,
         p_border_view] {
          auto& infos = p_unit_activation_view->info_map();
          p_unit_activation_view->on_click_unit( id );
          CHECK( infos.contains( id ) );
          auto& info = infos[id];
          p_fake_unit_view->set_orders( info.current_orders );
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
