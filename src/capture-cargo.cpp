/****************************************************************
**capture-cargo.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-03.
*
* Description: Implements the game mechanic where a ship captures
*              the cargo of a foreign ship defeated in battle.
*
*****************************************************************/
#include "capture-cargo.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "commodity.hpp"
#include "igui.hpp"
#include "revolution-status.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/cargo.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
CapturableCargo capturable_cargo_items( SSConst const& ss,
                                        CargoHold const& src,
                                        CargoHold const& dst ) {
  auto const compactified = [&]( CargoHold const& ch ) {
    CargoHold res = ch;
    res.compactify( ss.units );
    return res;
  };
  auto const from_compact = compactified( src );
  auto const to_compact   = compactified( dst );

  CapturableCargoItems items;
  for( auto const& [comm, slot] : from_compact.commodities() )
    items.commodities.push_back( comm );

  return CapturableCargo{
    .items    = std::move( items ),
    .max_take = to_compact.slots_remaining() };
}

wait<CapturableCargoItems> select_items_to_capture_ui(
    SSConst const& ss, IGui& gui, UnitId const src,
    UnitId const dst, CapturableCargo const& capturable ) {
  CapturableCargoItems res;
  auto const& src_unit      = ss.units.unit_for( src );
  auto const& dst_unit      = ss.units.unit_for( dst );
  e_nation const src_nation = src_unit.nation();
  e_nation const dst_nation = dst_unit.nation();
  Player const& src_player =
      player_for_nation_or_die( ss.players, src_nation );
  Player const& dst_player =
      player_for_nation_or_die( ss.players, dst_nation );
  if( capturable.items.commodities.empty() ) co_return res;
  if( capturable.max_take >=
      ssize( capturable.items.commodities ) ) {
    for( auto const& [type, q] : capturable.items.commodities ) {
      string const text = fmt::format(
          "[{} {}] has captured [{} {}] from [{}] cargo!",
          nation_possessive( dst_player ), dst_unit.desc().name,
          q, lowercase_commodity_display_name( type ),
          nation_possessive( src_player ) );
      // NOTE: we use IGui here and not IEuroMind because this
      // function is only called via IEuroMind in the case that
      // the mind is a human.
      co_await gui.message_box( "{}", text );
    }
    co_return capturable.items;
  }
  auto remaining = capturable;

  while( remaining.max_take > 0 ) {
    ChoiceConfig config{
      .msg = fmt::format(
          "Select a cargo item to capture from [{} {}]:",
          nation_possessive( src_player ),
          dst_unit.desc().name ),
      .options = {},
    };

    auto& commodities = remaining.items.commodities;

    for( int idx = 0; auto const& [type, q] : commodities ) {
      string const text = fmt::format(
          "{} {}", q, lowercase_commodity_display_name( type ) );
      ChoiceConfigOption option{
        .key          = fmt::to_string( idx ),
        .display_name = text,
      };
      config.options.push_back( option );
      ++idx;
    }

    maybe<int> const selection =
        co_await gui.optional_choice_idx( config );
    if( !selection.has_value() ) break;
    --remaining.max_take;
    res.commodities.push_back( commodities[*selection] );
    remaining.items.commodities.erase(
        remaining.items.commodities.begin() + *selection );
  }

  co_return res;
}

void transfer_capturable_cargo_items(
    SSConst const& ss, CapturableCargoItems const& capturable,
    CargoHold& src, CargoHold& dst ) {
  for( Commodity const& cm : capturable.commodities ) {
    src.compactify( ss.units );
    maybe<int> src_slot;
    for( auto const& [comm, slot] : src.commodities() ) {
      if( comm.type == cm.type &&
          comm.quantity >= cm.quantity ) {
        src_slot = slot;
        break;
      }
    }
    CHECK( src_slot.has_value(),
           "failed to find slot from which to take {} {} from "
           "capturable cargo.",
           cm.quantity, cm.type );
    int const moved = move_commodity_as_much_as_possible(
        ss.units, src, *src_slot, dst, /*dst_slot=*/0,
        cm.quantity,
        /*try_other_dst_slots=*/true );
    CHECK( moved == cm.quantity,
           "failed to move {} units of {} from capturable "
           "cargo; instead moved {}",
           cm.quantity, cm.type, moved );
  }
}

} // namespace rn
