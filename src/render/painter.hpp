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

  // Flip the interpretation of the stage. This is similar to
  // doing stage=1.0-stage, but it is not exactly the same be-
  // cause the set of pixels at play will be different when the
  // stage is different. By using this inversion, the set of
  // pixels that are visible/invisible are swapped.
  base::maybe<bool> inverted = {};

  // This should be set for any sprite that might move around on
  // screen as it is depixelating, e.g. a unit is depixelating
  // and the player scrolls the map. It will ensure that the ani-
  // mation proceeds deterministically as the sprite moves. The
  // value that this requires is a bit arbitrary, it just has to
  // move with the sprite. So using the upper left corner of the
  // sprite seems to be a good idea.
  base::maybe<gfx::point> hash_anchor = {};

  // This allows varying the depixelation stage across the body
  // of the triangle. If this is set then the anchor point will
  // have the stage value specified in the `stage` attribute and
  // it will be varied throughout the body of the triangle using
  // this gradient by extrapolating linearly in each dimension.
  // Specifically, each component of this quantity has the dimen-
  // sions of delta(stage)/delta(logical-pixels).
  base::maybe<gfx::dsize> stage_gradient = {};

  // When the depixelation stage is gradiated, this is the loca-
  // tion in game coordinates that will be assumed to hold the
  // base stage value and from which the other values across the
  // triangle will be computed using the stage_gradient slopes to
  // extrapolate.
  base::maybe<gfx::dpoint> stage_anchor = {};
};

// These options allow specifying a global rescaling and transla-
// tion that will be done to all vertices. First a rescaling is
// done around the origin (in screen coords), and then the trans-
// lation.
struct RepositionInfo {
  base::maybe<double> scale           = {};
  base::maybe<gfx::dsize> translation = {};
  bool use_camera                     = false;
};

struct ColorCyclingInfo {
  base::maybe<int> plan = {};
};

struct StencilPlan {
  int replacement_atlas_id = {};
  gfx::pixel key_color     = {};
};

struct PainterMods {
  DepixelateInfo depixelate              = {};
  base::maybe<double> alpha              = {};
  RepositionInfo repos                   = {};
  ColorCyclingInfo cycling               = {};
  base::maybe<bool> desaturate           = {};
  base::maybe<gfx::pixel> fixed_color    = {};
  base::maybe<bool> uniform_depixelation = {};
  base::maybe<StencilPlan> stencil       = {};
};

/****************************************************************
** Painter
*****************************************************************/
struct Painter {
  Painter( AtlasMap const& atlas, Emitter& emitter )
    : atlas_( atlas ), emitter_( emitter ), mods_{} {}

  Painter( AtlasMap const& atlas, Emitter& emitter,
           PainterMods const& mods ATTR_LIFETIMEBOUND )
    : atlas_( atlas ), emitter_( emitter ), mods_( &mods ) {}

  Painter with_mods(
      PainterMods const& mods ATTR_LIFETIMEBOUND );
  Painter without_mods();

  AtlasMap const& atlas() const { return atlas_; }
  Emitter& emitter() const { return emitter_; }
  PainterMods const& mods() const;

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
  enum class e_border_mode {
    inside,
    outside,
    in_out
  };

  // FIXME: these are expensive, requires 24 vertices. Probably
  // should have shader support for these builtin.
  Painter& draw_empty_rect( gfx::rect rect, e_border_mode mode,
                            gfx::pixel color );
  Painter& draw_empty_rect( gfx::rect rect, gfx::pixel color );

  // ......................[[ Sprites ]]...................... //

  Painter& draw_sprite( int atlas_id, gfx::point where );

  Painter& draw_sprite_scale( int atlas_id, gfx::rect dst );

  // This allows drawing a section of a sprite. The `section`
  // rect has its origin relative to the upper left corner of the
  // sprite. Any parts of the section that fall outside of the
  // sprite will be clipped.
  Painter& draw_sprite_section( int atlas_id, gfx::point where,
                                gfx::rect section );

 private:
  // Should always use this one to emit, that way we never forget
  // the mods.
  void emit( VertexBase&& vert );

  static void add_mods( VertexBase& vert,
                        PainterMods const& mods );

  // This will draw a box with the border "inside" on the north
  // and west faces but with the border "outside" on the south
  // and east faces.
  void draw_empty_box( gfx::rect r, gfx::pixel color );

  Painter& draw_sprite_impl( gfx::rect total_src,
                             gfx::point dst_origin,
                             base::maybe<gfx::size> dst_size,
                             base::maybe<gfx::rect> section );

  AtlasMap const& atlas_;
  Emitter& emitter_;
  PainterMods const* mods_ = {}; // nullptr if no mods present.
};

} // namespace rr
