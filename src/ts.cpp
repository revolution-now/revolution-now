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
#include "ieuro-mind.hpp"
#include "imap-updater.hpp"
#include "inative-mind.hpp"
#include "irand.hpp"
#include "map-updater-lua.hpp"

// ss
#include "ss/native-enums.rds.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NativeMinds
*****************************************************************/
NativeMinds::NativeMinds(
    std::unordered_map<e_tribe, unique_ptr<INativeMind>> minds )
  : minds_( std::move( minds ) ) {}

NativeMinds::~NativeMinds() = default;

INativeMind& NativeMinds::operator[]( e_tribe tribe ) const {
  auto iter = minds_.find( tribe );
  CHECK( iter != minds_.end(),
         "no INativeMind object for tribe {}.", tribe );
  unique_ptr<INativeMind> const& p_mind = iter->second;
  CHECK( p_mind != nullptr,
         "null INativeMind object for tribe {}.", tribe );
  return *p_mind;
}

/****************************************************************
** EuroMinds
*****************************************************************/
EuroMinds::EuroMinds(
    std::unordered_map<e_nation, unique_ptr<IEuroMind>> minds )
  : minds_( std::move( minds ) ) {}

EuroMinds::~EuroMinds() = default;

IEuroMind& EuroMinds::operator[]( e_nation nation ) const {
  auto iter = minds_.find( nation );
  CHECK( iter != minds_.end(),
         "no IEuroMind object for nation {}.", nation );
  unique_ptr<IEuroMind> const& p_mind = iter->second;
  CHECK( p_mind != nullptr,
         "null IEuroMind object for nation {}.", nation );
  return *p_mind;
}

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
    st["ROOT_TS"]["map_updater"] = new_ts.map_updater;
    st["ROOT_TS"]["rand"]        = new_ts.rand;
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
TS::TS( Planes& planes_, IMapUpdater& map_updater_,
        lua::state& lua_, IGui& gui_, IRand& rand_,
        ICombat& combat_, IColonyViewer& colony_viewer_,
        RootState& saved, TerrainConnectivity& connectivity_,
        NativeMinds& native_minds_, EuroMinds& euro_minds_ )
  : planes( planes_ ),
    map_updater( map_updater_ ),
    lua( lua_ ),
    gui( gui_ ),
    rand( rand_ ),
    combat( combat_ ),
    colony_viewer( colony_viewer_ ),
    saved( saved ),
    connectivity( connectivity_ ),
    native_minds( native_minds_ ),
    euro_minds( euro_minds_ ),
    pimpl_( new LuaRefSetAndRestore( lua, *this ) ) {}

// These are here because we are using the pimpl idiom.
TS::~TS()      = default;
TS::TS( TS&& ) = default;

void to_str( TS const& o, string& out, base::ADL_t ) {
  out += "TS@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

TS TS::with_gui( IGui& new_gui ) {
  return TS( planes, map_updater, lua, new_gui, rand, combat,
             colony_viewer, saved, connectivity, native_minds,
             euro_minds );
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
