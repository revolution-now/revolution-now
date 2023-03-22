/****************************************************************
**imap-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Interface for modifying the map.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "imap-updater.rds.hpp"

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

namespace rn {

struct IMapUpdater;

/****************************************************************
** MapUpdaterOptions
*****************************************************************/
struct MapUpdaterOptions {
  // This desired value of this `nation` field at any given time
  // can be derived from other state in the game, but it is here
  // to represent the current state of rendering (with respect to
  // nation perspective) on the GPU, so that if that desired
  // value changes, we know when we need to redraw.
  maybe<e_nation> nation            = nothing;
  bool            render_forests    = true;
  bool            render_resources  = true;
  bool            render_lcrs       = true;
  bool            grid              = false;
  bool            render_fog_of_war = true;

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
      base::function_ref<void( gfx::Matrix<MapSquare>& )>;
  using OptionsUpdateFunc =
      base::function_ref<void( MapUpdaterOptions& )>;
  using Popper = detail::MapUpdaterOptionsPopper;

  IMapUpdater();

  IMapUpdater( MapUpdaterOptions const& initial_options );

  virtual ~IMapUpdater() = default;

  // This function should be used whenever a map square (specifi-
  // cally, a MapSquare object) must be updated as it will han-
  // dler re-rendering the surrounding squares. Returns which
  // buffers needed to be redrawn (in practice, just the land-
  // scape buffer).
  virtual BuffersUpdated modify_map_square(
      Coord tile, SquareUpdateFunc mutator ) = 0;

  // This function should be used when generating the map. It
  // will not (re)draw the map or update player maps, since it is
  // only expected to be called when setting up the world, and in
  // that process the player maps and drawing happen subse-
  // quently.
  virtual void modify_entire_map( MapUpdateFunc mutator ) = 0;

  // If the given nation cannot see the square it will be made
  // visible, and if it was already visible then it will be up-
  // dated in case it was stale. In either case, it will also
  // remove the fog from the square if there is any.
  //
  // The return object will specify which buffers needed a re-
  // draw. and will have been redrawn.
  virtual BuffersUpdated make_square_visible(
      Coord tile, e_nation nation ) = 0;

  // If the square is not fogged from the perspective of the
  // player then it is made so and any redrawing is done if nec-
  // essary, if fog rendering is enabled. If the square is not
  // visible then no changes are made. Returns which buffers
  // needed a redraw, if any (in practice, this will just be the
  // obfuscation buffer).
  virtual BuffersUpdated make_square_fogged(
      Coord tile, e_nation nation ) = 0;

  // Will redraw the entire map.
  virtual void redraw() = 0;

  // This will clear all of the buffers.
  virtual void unrender() = 0;

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

} // namespace rn
