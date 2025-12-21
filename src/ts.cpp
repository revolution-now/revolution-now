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

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TS
*****************************************************************/
TS::TS( Planes& planes_, IGui& gui_, ICombat& combat_,
        IColonyViewer& colony_viewer_, RootState& saved )
  : planes( planes_ ),
    gui( gui_ ),
    combat( combat_ ),
    colony_viewer( colony_viewer_ ),
    saved( saved ) {}

void to_str( TS const& o, string& out, base::tag<TS> ) {
  out += "TS@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

} // namespace rn
