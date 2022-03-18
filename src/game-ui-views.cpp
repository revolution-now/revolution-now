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
#include "logger.hpp"
#include "unit.hpp"
#include "ustate.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn::ui {

/****************************************************************
** UnitActivationView
*****************************************************************/
UnitActivationView::UnitActivationView( bool allow_activation )
  : allow_activation_( allow_activation ), info_map_{} {}

void UnitActivationView::on_click_unit( UnitId id ) {
  auto& infos = info_map();
  CHECK( infos.contains( id ) );
  UnitActivationInfo& info = infos[id];
  if( info.original_orders != e_unit_orders::none ) {
    if( allow_activation_ ) {
      // Orders --> No Orders --> No Orders+Prio --> ...
      if( info.current_orders != e_unit_orders::none ) {
        CHECK( !info.is_activated );
        info.current_orders = e_unit_orders::none;
      } else if( info.is_activated ) {
        CHECK( info.current_orders == e_unit_orders::none );
        info.current_orders = info.original_orders;
        info.is_activated   = false;
      } else {
        CHECK( !info.is_activated );
        info.is_activated = true;
      }
    } else {
      // Orders --> No Orders --> ...
      CHECK( !info.is_activated );
      if( info.current_orders != e_unit_orders::none ) {
        info.current_orders = e_unit_orders::none;
      } else {
        info.current_orders = info.original_orders;
      }
    }
  } else {
    if( allow_activation_ ) {
      // No Orders --> No Orders+Prioritized --> ...
      CHECK( info.original_orders == e_unit_orders::none );
      CHECK( info.current_orders == e_unit_orders::none );
      info.is_activated = !info.is_activated;
    } else {
      // No Orders --> ...
      CHECK( info.original_orders == e_unit_orders::none );
      CHECK( info.current_orders == e_unit_orders::none );
      CHECK( !info.is_activated );
    }
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
    vector<UnitId> const& ids_, bool allow_activation ) {
  auto unit_activation_view =
      std::make_unique<UnitActivationView>( allow_activation );
  auto* p_unit_activation_view = unit_activation_view.get();

  auto cmp = []( UnitId l, UnitId r ) {
    auto const& unit1 = unit_from_id( l ).desc();
    auto const& unit2 = unit_from_id( r ).desc();
    if( unit1.ship && !unit2.ship ) return true;
    if( unit1.cargo_slots > unit2.cargo_slots ) return true;
    if( unit1.attack_points > unit2.attack_points ) return true;
    return false;
  };

  auto ids = ids_;
  sort( ids.begin(), ids.end(), cmp );

  vector<unique_ptr<View>> units_vec;

  auto& infos = unit_activation_view->info_map();
  for( auto id : ids ) {
    auto const& unit = unit_from_id( id );
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
    auto const& unit = unit_from_id( id );

    auto fake_unit_view = make_unique<FakeUnitView>(
        unit.desc().type, unit.nation(), unit.orders() );
    auto* p_fake_unit_view = fake_unit_view.get();

    auto border_view = make_unique<BorderView>(
        std::move( fake_unit_view ), gfx::pixel::white(),
        /*padding=*/1,
        /*on_initially=*/false );
    auto* p_border_view = border_view.get();

    auto clickable = make_unique<ClickableView>(
        // Capture local variables by value.
        std::move( border_view ),
        [p_unit_activation_view, id, p_fake_unit_view,
         p_border_view] {
          auto& infos = p_unit_activation_view->info_map();
          lg.debug( "clicked on {}", debug_string( id ) );
          p_unit_activation_view->on_click_unit( id );
          CHECK( infos.contains( id ) );
          auto& info = infos[id];
          p_fake_unit_view->set_orders( info.current_orders );
          p_border_view->on( info.is_activated );
        } );

    auto unit_label = make_unique<OneLineStringView>(
        unit.desc().name, gfx::pixel::banana() );

    vector<unique_ptr<View>> horizontal_vec;
    horizontal_vec.emplace_back( std::move( clickable ) );
    horizontal_vec.emplace_back( std::move( unit_label ) );
    auto horizontal_view = make_unique<HorizontalArrayView>(
        std::move( horizontal_vec ),
        HorizontalArrayView::align::middle );

    units_vec.emplace_back( std::move( horizontal_view ) );
  }
  auto arr_view = make_unique<VerticalArrayView>(
      std::move( units_vec ), VerticalArrayView::align::left );

  unit_activation_view->set_view( std::move( arr_view ),
                                  Coord{} );
  return unit_activation_view;
}

} // namespace rn::ui
