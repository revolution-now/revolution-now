/****************************************************************
**painter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: High level drawing commands that emit vertices.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

// base
#include "base/maybe.hpp"

namespace rr {

struct AtlasMap;
struct Emitter;
struct VertexBase;

/****************************************************************
** PainterMods
*****************************************************************/
struct PainterMods {
  base::maybe<double> depixelate;
  base::maybe<double> alpha;
};

/****************************************************************
** Painter
*****************************************************************/
struct Painter {
  Painter( AtlasMap const& atlas, Emitter& emitter )
    : atlas_( atlas ), emitter_( emitter ), mods_{} {}

  base::maybe<PainterMods const&> mods() const { return mods_; }
  Painter with_mods( PainterMods const& mods );
  Painter without_mods();

  // .......................[[ Points ]]...................... //

  void draw_point( gfx::point p, gfx::pixel color );

  // .......................[[ Lines ]]....................... //

  void draw_horizontal_line( gfx::point start, int length,
                             gfx::pixel color );

  void draw_vertical_line( gfx::point start, int length,
                           gfx::pixel color );

  // .....................[[ Solid Rect ]].................... //

  void draw_solid_rect( gfx::rect rect, gfx::pixel color );

  // .....................[[ Empty Rect ]].................... //

  enum class e_border_mode { inside, outside };

  // FIXME: this expensive, requires 24 vertices. Probably should
  // have shader support for these builtin.
  void draw_empty_rect( gfx::rect rect, e_border_mode mode,
                        gfx::pixel color );

  // ......................[[ Sprites ]]...................... //

  void draw_sprite( int atlas_id, gfx::point where );

  void draw_sprite_scale( int atlas_id, gfx::rect dst );

  void draw_silhouette( int atlas_id, gfx::point where,
                        gfx::pixel color );

  void draw_silhouette_scale( int atlas_id, gfx::rect dst,
                              gfx::pixel color );

 private:
  // Should always use this one to emit, that way we never forget
  // the mods.
  void emit( VertexBase&& vert );

  static void add_mods( VertexBase&        vert,
                        PainterMods const& mods );

  // This will draw a box with the border "inside" on the north
  // and west faces but with the border "outside" on the south
  // and east faces.
  void draw_empty_box( gfx::rect r, gfx::pixel color );

  void draw_sprite_impl( gfx::rect src, gfx::rect dst );

  void draw_silhouette_impl( gfx::rect src, gfx::rect dst,
                             gfx::pixel color );

  AtlasMap const&          atlas_;
  Emitter&                 emitter_;
  base::maybe<PainterMods> mods_;
};

} // namespace rr
