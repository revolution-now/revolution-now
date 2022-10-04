/****************************************************************
**imap-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Handlers when a map square needs to be modified.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "map-square.hpp"
#include "matrix.hpp"
#include "maybe.hpp"

// ss
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/function-ref.hpp"
#include "base/macros.hpp"
#include "base/to-str.hpp"

// C++ standard library
#include <stack>

namespace rr {
struct Renderer;
}

namespace rn {

struct IMapUpdater;
struct SS;
struct TerrainRenderOptions;
struct Visibility;

/****************************************************************
** MapUpdaterOptions
*****************************************************************/
struct MapUpdaterOptions {
  maybe<e_nation> nation           = nothing;
  bool            render_forests   = true;
  bool            render_resources = true;
  bool            render_lcrs      = true;
  bool            grid             = false;

  bool operator==( MapUpdaterOptions const& ) const = default;
};

namespace detail {

struct [[nodiscard]] MapUpdaterOptionsPopper {
  MapUpdaterOptionsPopper( IMapUpdater& map_updater )
    : map_updater_( map_updater ) {}
  ~MapUpdaterOptionsPopper() noexcept;
  NO_COPY_NO_MOVE( MapUpdaterOptionsPopper );

 private:
  IMapUpdater& map_updater_;
};

} // namespace detail

/****************************************************************
** IMapUpdater
*****************************************************************/
// This is an abstract class so that it can be injected and
// mocked, which is useful because there are places in the code
// that we want to test but that happen to also need to update
// map squares, and we want to be able to test those without
// having to worry about a dependency on the renderer.
struct IMapUpdater {
  using SquareUpdateFunc =
      base::function_ref<void( MapSquare& )>;
  using MapUpdateFunc =
      base::function_ref<void( Matrix<MapSquare>& )>;
  using OptionsUpdateFunc =
      base::function_ref<void( MapUpdaterOptions& )>;
  using Popper = detail::MapUpdaterOptionsPopper;

  IMapUpdater();

  virtual ~IMapUpdater() = default;

  // This function should be used whenever a map square (specifi-
  // cally, a MapSquare object) must be updated as it will han-
  // dler re-rendering the surrounding squares. Returns true if
  // the new square is different from the old one.
  virtual bool modify_map_square( Coord            tile,
                                  SquareUpdateFunc mutator ) = 0;

  // This function should be used when generating the map. It
  // will not (re)draw the map or update player maps, since it is
  // only expected to be called when setting up the world, and in
  // that process the player maps and drawing happen subse-
  // quently.
  virtual void modify_entire_map( MapUpdateFunc mutator ) = 0;

  // If the given nation cannot see the square it will be made
  // visible, and if it was already visible then it will be up-
  // dated in case it was stale. In the case that it was updated
  // (and changed) it will be redrawn. Returns true if the play-
  // er's map square was changed as a result.
  virtual bool make_square_visible( Coord    tile,
                                    e_nation nation ) = 0;

  // Will redraw the entire map.
  virtual void redraw() = 0;

  // Will call the function with the existing set of options and
  // allow modifying them, then will push a new (modified) copy
  // onto the stack, perform a full redraw if the options have
  // changed, and return a popper. Note that since this does per-
  // form a full redraw, you should modify multiple options in
  // one shot.
  Popper push_options_and_redraw( OptionsUpdateFunc mutator );

  MapUpdaterOptions const& options() const;

  // Before using this consider if it would be better to use the
  // push/pop method. This will allow mutating the current op-
  // tions and will redraw only they've actually changed.
  void mutate_options_and_redraw( OptionsUpdateFunc mutator );

  friend void to_str( IMapUpdater const& o, std::string& out,
                      base::ADL_t );

 private:
  friend struct detail::MapUpdaterOptionsPopper;

  std::stack<MapUpdaterOptions> options_;
};

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
struct NonRenderingMapUpdater : IMapUpdater {
  NonRenderingMapUpdater( SS& ss ) : ss_( ss ) {}

  // Implement IMapUpdater.
  bool modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;

 protected:
  SS& ss_;
};

/****************************************************************
** MapUpdater
*****************************************************************/
// TODO:
//
//   1. Move this to its own file.
//   2. Rename it to RenderingMapUpdater.
//   3. Consider changing to pimpl, though it may not be neces-
//      sary as only one place should include it.
//   4. Implement the vertex zapper, and let it be able to zap
//      vertices in either the landscape or annex buffer.
//   5. Give it the ability to perform a full re-render in a
//      background thread (use jthread and re-watch the video on
//      C++20 threading enhancements).
//   6. At that point we should be able to support arbitrarily
//      large maps seemlessly.
//
// The real map updater used by the game. This one delegates to
// the non-rendering version to make changes to the maps, then
// proceeds (if necessary) to do rendering.
struct MapUpdater : NonRenderingMapUpdater {
  using Base = NonRenderingMapUpdater;

  MapUpdater( SS& ss, rr::Renderer& renderer )
    : NonRenderingMapUpdater( ss ),
      renderer_( renderer ),
      tiles_redrawn_( 0 ) {}

  // Implement IMapUpdater.
  bool modify_map_square( Coord            tile,
                          SquareUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;

 private:
  void redraw_square(
      Visibility const&           viz,
      TerrainRenderOptions const& terrain_options, Coord tile );

  rr::Renderer& renderer_;
  int           tiles_redrawn_;
};

/****************************************************************
** TrappingMapUpdater
*****************************************************************/
// This one literally does nothing except for check-fail if any
// of its methods are called that attempt to modify the map. It's
// for when you know that the map updater will not be called, but
// need one to pass in anyway.
struct TrappingMapUpdater : IMapUpdater {
  TrappingMapUpdater() = default;

  // Implement IMapUpdater.
  bool modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;
};

} // namespace rn
