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

// luapp
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"
#include "refl/traverse.hpp"
#include "refl/validate.hpp"

// traverse
#include "traverse/ext-base.hpp"
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;
using ::trv::traverse;

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
    settings( impl_->top.settings ),
    events( impl_->top.events ),
    units( impl_->top.units ),
    players( impl_->top.players ),
    turn( impl_->top.turn ),
    colonies( impl_->top.colonies ),
    natives( impl_->top.natives ),
    land_view( impl_->top.land_view ),
    map( impl_->top.map ),
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
    settings( ss_.settings ),
    events( ss_.events ),
    units( ss_.units ),
    players( ss_.players ),
    turn( ss_.turn ),
    colonies( ss_.colonies ),
    natives( ss_.natives ),
    land_view( ss_.land_view ),
    map( ss_.map ),
    terrain( ss_.terrain ),
    root( ss_.root ) {}

SSConst::SSConst( SS const& ss )
  : ss_( ss ),
    version( ss_.version ),
    settings( ss_.settings ),
    events( ss_.events ),
    units( ss_.units ),
    players( ss_.players ),
    turn( ss_.turn ),
    colonies( ss_.colonies ),
    natives( ss_.natives ),
    land_view( ss_.land_view ),
    map( ss_.map ),
    terrain( ss_.terrain ),
    root( ss_.root ) {}

valid_or<string> SSConst::validate_full_game_state() const {
  ScopedTimer const timer( "full game state validation" );
  return refl::validate_recursive( root, "root" );
}

valid_or<string> SSConst::validate_non_terrain_game_state()
    const {
  valid_or<string> res = valid;
  ScopedTimer const timer( "quick game state validation" );
  traverse( root, [&]( auto const& o, string_view const name ) {
    if( !res.valid() ) return;
    if( name.find( "terrain" ) != string_view::npos ) return;
    string const key = format( "root.{}", name );

    res = refl::validate_recursive( o, key );
  } );
  return res;
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

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = SS;
  st.usertype.create<U>();
  // We don't need to register any members here since lua should
  // access those via the "ROOT" global which will be a RootState
  // and its fields are exposed. We just need to expose the SS
  // type so that we can extract it from Lua to C++ and pass it
  // to a C++ function.
};

} // namespace
} // namespace rn
