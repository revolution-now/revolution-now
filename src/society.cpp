/****************************************************************
**society.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-04.
*
* Description: Abstraction for nations and tribes.
*
*****************************************************************/
#include "society.hpp"

// Revolution Now
#include "tribe-mgr.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

VisibleSociety society_on_square_impl( SSConst const& ss,
                                       IVisibility const& viz,
                                       point const tile ) {
  // Check for European colony.
  if( auto const colony = viz.colony_at( tile );
      colony.has_value() )
    return VisibleSociety::society{
      .value = Society::european{ .player = colony->player } };

  // Check for native dwelling.
  if( auto const dwelling = viz.dwelling_at( tile );
      dwelling.has_value() )
    return VisibleSociety::society{
      .value = Society::native{
        .tribe = tribe_type_for_dwelling( ss, *dwelling ) } };

  switch( viz.visible( tile ) ) {
    case e_tile_visibility::hidden:
      return VisibleSociety::hidden{};
    case e_tile_visibility::fogged:
      // We already know there are no visble colonies or
      // dwellings on the tile, so all that remains to check are
      // units. However, if the tile is fogged that means that
      // there will be no visible units on the tile, thus it will
      // appear empty.
      return VisibleSociety::empty{};
    case e_tile_visibility::clear:
      break;
  }

  // Check units.
  auto& units = ss.units;
  unordered_set<GenericUnitId> const& on_coord =
      units.from_coord( tile );
  if( on_coord.empty() ) return VisibleSociety::empty{};
  GenericUnitId const id = *on_coord.begin();

  switch( units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      e_player const player =
          units.unit_for( units.check_euro_unit( id ) )
              .player_type();
      return VisibleSociety::society{
        .value = Society::european{ .player = player } };
    }
    case e_unit_kind::native: {
      e_tribe const tribe = tribe_type_for_unit(
          ss, units.unit_for( units.check_native_unit( id ) ) );
      return VisibleSociety::society{
        .value = Society::native{ .tribe = tribe } };
    }
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<Society> society_on_real_square( SSConst const& ss,
                                       point const tile ) {
  SWITCH( society_on_square_impl( ss, VisibilityEntire( ss ),
                                  tile ) ) {
    CASE( hidden ) { SHOULD_NOT_BE_HERE; }
    CASE( empty ) { return nothing; }
    CASE( society ) { return society.value; }
  }
}

VisibleSociety society_on_visible_square( SSConst const& ss,
                                          IVisibility const& viz,
                                          point const tile ) {
  return society_on_square_impl( ss, viz, tile );
}

gfx::pixel flag_color_for_society( Society const& society ) {
  switch( society.to_enum() ) {
    case Society::e::european: {
      auto const& o = society.get<Society::european>();
      return config_nation.players[o.player].flag_color;
    }
    case Society::e::native: {
      auto const& o = society.get<Society::native>();
      return config_natives.tribes[o.tribe].flag_color;
    }
  }
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( tribe_on_square, maybe<e_tribe>, Coord square ) {
  SSConst const& ss = st["SS"].as<SS&>();
  return society_on_real_square( ss, square )
      .bind( []( Society const& society ) {
        return society.get_if<Society::native>().member(
            &Society::native::tribe );
      } );
}

} // namespace

} // namespace rn
