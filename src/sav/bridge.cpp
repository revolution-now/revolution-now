/****************************************************************
**bridge.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: TODO [FILL ME IN]
*
*****************************************************************/
#include "bridge.hpp"

// sav
#include "sav-struct.hpp"

// ss
#include "ss/root.hpp"

using namespace std;

namespace bridge {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<std::string> convert_to_rn( sav::ColonySAV const& in,
                                     rn::RootState& out ) {
  // Terrain.
  rn::wrapped::TerrainState terrain_o;

  if( in.header.map_size_x <= 2 || in.header.map_size_y <= 2 )
    return "map size too small";
  auto& map = terrain_o.real_terrain.map;
  map       = gfx::Matrix<rn::MapSquare>(
      rn::Delta{ .w = in.header.map_size_x - 2,
                       .h = in.header.map_size_y - 2 } );
  // ...
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // TODO: add more here.

  return valid;
}

valid_or<std::string> convert_to_og( rn::RootState const& in,
                                     sav::ColonySAV&      out ) {
  out.header.colonize = { 'C', 'O', 'L', 'O', 'N',
                          'I', 'Z', 'E', 0 };

  out.header.map_size_x =
      in.zzz_terrain.world_map().size().w + 2;
  out.header.map_size_y =
      in.zzz_terrain.world_map().size().h + 2;

  // TODO: add more here.

  return valid;
}

valid_or<std::string> convert_to_rn( sav::MapFile const&,
                                     rn::RealTerrain& ) {
  // TODO
  return valid;
}

valid_or<std::string> convert_to_og( rn::RealTerrain const&,
                                     sav::ColonySAV& ) {
  // TODO
  return valid;
}

} // namespace bridge
