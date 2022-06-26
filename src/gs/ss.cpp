/****************************************************************
**ss.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds the serializable state of a game.
*
*****************************************************************/
#include "ss.hpp"

// ss
#include "root.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** SS::Impl
*****************************************************************/
struct SS::Impl {
  RootState top;
};

/****************************************************************
** SS
*****************************************************************/
SS::~SS() = default;

SS::SS() : impl_( new Impl ) {}

FormatVersion& SS::version() { return impl_->top.version; }

EventsState& SS::events() { return impl_->top.events; }

SettingsState& SS::settings() { return impl_->top.settings; }

UnitsState& SS::units() { return impl_->top.units; }

PlayersState& SS::players() { return impl_->top.players; }

TurnState& SS::turn() { return impl_->top.turn; }

ColoniesState& SS::colonies() { return impl_->top.colonies; }

LandViewState& SS::land_view() { return impl_->top.land_view; }

TerrainState const& SS::terrain() {
  return impl_->top.zzz_terrain;
}

TerrainState& SS::mutable_terrain_use_with_care() {
  return impl_->top.zzz_terrain;
}

RootState& SS::root() { return impl_->top; }

/****************************************************************
** SSConst
*****************************************************************/
FormatVersion const& SSConst::version() { return ss_.version(); }

EventsState const& SSConst::events() { return ss_.events(); }

SettingsState const& SSConst::settings() {
  return ss_.settings();
}

UnitsState const& SSConst::units() { return ss_.units(); }

PlayersState const& SSConst::players() { return ss_.players(); }

TurnState const& SSConst::turn() { return ss_.turn(); }

ColoniesState const& SSConst::colonies() {
  return ss_.colonies();
}

LandViewState const& SSConst::land_view() {
  return ss_.land_view();
}

TerrainState const& SSConst::terrain() { return ss_.terrain(); }

RootState const& SSConst::root() { return ss_.root(); }

base::valid_or<std::string> SSConst::validate_game_state()
    const {
  // TODO: just do a quick-and-dirty recursive approach within
  // this file.
  NOT_IMPLEMENTED;
}

} // namespace rn
