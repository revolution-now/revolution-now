/****************************************************************
**terrain-render-options.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-02.
*
* Description: Options for rendering the map.
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** TerrainRenderOptions
*****************************************************************/
struct TerrainRenderOptions {
  bool grid              = false;
  bool render_fog_of_war = true;
};

} // namespace rn
