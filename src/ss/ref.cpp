/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds the serializable state of a game.
*
*****************************************************************/
#include "ref.hpp"

// ss
#include "root.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;

}

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

SS::SS()
  : impl_( new Impl ),
    version( impl_->top.version ),
    meta( impl_->top.meta ),
    settings( impl_->top.settings ),
    events( impl_->top.events ),
    units( impl_->top.units ),
    players( impl_->top.players ),
    turn( impl_->top.turn ),
    colonies( impl_->top.colonies ),
    natives( impl_->top.natives ),
    land_view( impl_->top.land_view ),
    map( impl_->top.map ),
    trade_routes( impl_->top.trade_routes ),
    terrain( impl_->top.zzz_terrain ),
    mutable_terrain_use_with_care( impl_->top.zzz_terrain ),
    root( impl_->top ),
    as_const( *this ) {
  // Initialize these with default values. This is very important
  // to do here since they need to be set even if the game is not
  // customized because they affect game rules, and some of the
  // default values are not simply default constructed values.
  settings.game_setup_options.customized_rules =
      config_options.default_values;
}

void to_str( SS const& o, string& out, base::tag<SS> ) {
  out += "SS@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

/****************************************************************
** SSConst
*****************************************************************/
SSConst::SSConst( SS& ss )
  : ss_( ss ),
    version( ss_.version ),
    meta( ss_.meta ),
    settings( ss_.settings ),
    events( ss_.events ),
    units( ss_.units ),
    players( ss_.players ),
    turn( ss_.turn ),
    colonies( ss_.colonies ),
    natives( ss_.natives ),
    land_view( ss_.land_view ),
    map( ss_.map ),
    trade_routes( ss_.trade_routes ),
    terrain( ss_.terrain ),
    root( ss_.root ) {}

SSConst::SSConst( SS const& ss )
  : ss_( ss ),
    version( ss_.version ),
    meta( ss_.meta ),
    settings( ss_.settings ),
    events( ss_.events ),
    units( ss_.units ),
    players( ss_.players ),
    turn( ss_.turn ),
    colonies( ss_.colonies ),
    natives( ss_.natives ),
    land_view( ss_.land_view ),
    map( ss_.map ),
    trade_routes( ss_.trade_routes ),
    terrain( ss_.terrain ),
    root( ss_.root ) {}

valid_or<string> SSConst::validate_full_game_state() const {
  ScopedTimer const timer( "full game state validation" );
  return validate_recursive( root );
}

valid_or<string> SSConst::validate_non_terrain_game_state()
    const {
  ScopedTimer const timer( "quick game state validation" );
  return validate_recursive_non_terrain( root );
}

/****************************************************************
** Root reference helpers.
*****************************************************************/
bool root_states_equal( RootState const& l,
                        RootState const& r ) {
  return l == r;
}

void assign_src_to_dst( RootState const& src, RootState& dst ) {
  dst = src;
}

} // namespace rn
