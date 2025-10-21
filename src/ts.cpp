/****************************************************************
**ts.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Non-serialized (transient) game state.
*
*****************************************************************/
#include "ts.hpp"

// Revolution Now
#include "imap-updater.hpp"
#include "irand.hpp"
#include "map-updater-lua.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TS::LuaRefSetAndRestore
*****************************************************************/
struct TS::LuaRefSetAndRestore {
  LuaRefSetAndRestore( lua::state& st, TS& new_ts )
    : st_( st ),
      previous_ts_( st["TS"] ),
      previous_root_ts_( st["ROOT_TS"] ) {
    // FIXME: this really needs to be improved.
    st["TS"]                     = new_ts;
    st["ROOT_TS"]                = st.table.create();
    st["ROOT_TS"]["map_updater"] = [&]() -> IMapUpdater& {
      return new_ts.map_updater();
    };
    st["ROOT_TS"]["rand"] = new_ts.rand;
  }

  ~LuaRefSetAndRestore() {
    st_["TS"]      = previous_ts_;
    st_["ROOT_TS"] = previous_root_ts_;
  }

  lua::state& st_;

  // This will allow us to restore the global TS using RAII. This
  // is needed because sometimes nested functions or scopes can
  // create their own TS object with slightly modified members,
  // and we always want the lua version to reflect the "active"
  // one.
  lua::any previous_ts_;
  lua::any previous_root_ts_;
};

/****************************************************************
** TS
*****************************************************************/
TS::TS( Planes& planes_, lua::state& lua_, IGui& gui_,
        IRand& rand_, ICombat& combat_,
        IColonyViewer& colony_viewer_, RootState& saved )
  : planes( planes_ ),
    lua( lua_ ),
    gui( gui_ ),
    rand( rand_ ),
    combat( combat_ ),
    colony_viewer( colony_viewer_ ),
    saved( saved ),
    pimpl_( new LuaRefSetAndRestore( lua, *this ) ) {}

// These are here because we are using the pimpl idiom.
TS::~TS() = default;

void to_str( TS const& o, string& out, base::tag<TS> ) {
  out += "TS@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = TS;
  st.usertype.create<U>();
  // We don't register any members here since we can't form
  // member pointers to references. The TS object is access by
  // Lua from the top-level "TS" name, and that needs to be manu-
  // ally set up by the function above. We just need to expose
  // the TS type so that we can extract it from Lua to C++ and
  // pass it to a C++ function.
};

} // namespace
} // namespace rn
