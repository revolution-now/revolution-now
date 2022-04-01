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
// Only the leaf mod traits should be wrapped in maybe's instead
// of the structs grouping them because it makes it easier to
// perform focused updates on individual fields.
struct DepixelateInfo {
  base::maybe<double> stage = {};

  // If the sprite is depixelating to a different sprite then
  // this will be the offset from the source pixel to the target
  // pixel in the atlas. Otherwise, the depixelation just goes to
  // full transparency.
  base::maybe<gfx::size> target = {};

  // This should be set for any sprite that might move around on
  // screen as it is depixelating, e.g. a unit is depixelating
  // and the player scrolls the map. It will ensure that the ani-
  // mation proceeds deterministically as the sprite moves. The
  // value that this requires is a bit arbitrary, it just has to
  // move with the sprite. So using the upper left corner of the
  // sprite seems to be a good idea.
  base::maybe<gfx::point> anchor = {};
};

// These options allow specifying a global rescaling and transla-
// tion that will be done to all vertices. First a rescaling is
// done around the origin (in screen coords), and then the trans-
// lation.
struct RepositionInfo {
  base::maybe<double>    scale       = {};
  base::maybe<gfx::size> translation = {};
};

struct PainterMods {
  DepixelateInfo      depixelate = {};
  base::maybe<double> alpha      = {};
  RepositionInfo      repos      = {};
};

/****************************************************************
** Painter
*****************************************************************/
struct Painter {
  Painter( AtlasMap const& atlas, Emitter& emitter )
    : atlas_( atlas ), emitter_( emitter ), mods_{} {}

  Painter( AtlasMap const& atlas, Emitter& emitter,
           PainterMods mods )
    : atlas_( atlas ),
      emitter_( emitter ),
      mods_( std::move( mods ) ) {}

  Painter with_mods( PainterMods const& mods );
  Painter without_mods();

  AtlasMap const& atlas() const { return atlas_; }
  Emitter&        emitter() const { return emitter_; }
  base::maybe<PainterMods const&> mods() const { return mods_; }

  // .......................[[ Points ]]...................... //

  Painter& draw_point( gfx::point p, gfx::pixel color );

  // .......................[[ Lines ]]....................... //

  Painter& draw_horizontal_line( gfx::point start, int length,
                                 gfx::pixel color );

  Painter& draw_vertical_line( gfx::point start, int length,
                               gfx::pixel color );

  // .....................[[ Solid Rect ]].................... //

  Painter& draw_solid_rect( gfx::rect rect, gfx::pixel color );

  // .....................[[ Empty Rect ]].................... //

  // in_out means that the border will be on the inside on the
  // top and left while it will be on the outside on the bottom
  // and right.
  enum class e_border_mode { inside, outside, in_out };

  // FIXME: this expensive, requires 24 vertices. Probably should
  // have shader support for these builtin.
  Painter& draw_empty_rect( gfx::rect rect, e_border_mode mode,
                            gfx::pixel color );

  // ......................[[ Sprites ]]...................... //

  Painter& draw_sprite( int atlas_id, gfx::point where );

  Painter& draw_sprite_scale( int atlas_id, gfx::rect dst );

  // This allows drawing a section of a sprite. The `section`
  // rect has its origin relative to the upper left corner of the
  // sprite. Any parts of the section that fall outside of the
  // sprite will be clipped.
  Painter& draw_sprite_section( int atlas_id, gfx::point where,
                                gfx::rect section );

  Painter& draw_silhouette( int atlas_id, gfx::point where,
                            gfx::pixel color );

  Painter& draw_silhouette_scale( int atlas_id, gfx::rect dst,
                                  gfx::pixel color );

  gfx::size depixelation_offset( int from_atlas_id,
                                 int to_atlas_id ) const;

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
