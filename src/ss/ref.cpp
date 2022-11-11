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
    terrain( impl_->top.zzz_terrain ),
    mutable_terrain_use_with_care( impl_->top.zzz_terrain ),
    root( impl_->top ) {}

void to_str( SS const& o, string& out, base::ADL_t ) {
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
    terrain( ss_.terrain ),
    root( ss_.root ) {}

base::valid_or<std::string> SSConst::validate_game_state()
    const {
  // TODO: just do a quick-and-dirty recursive approach within
  // this file.
  NOT_IMPLEMENTED;
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
