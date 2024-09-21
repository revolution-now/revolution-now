/****************************************************************
**painter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: High level drawing commands that emit vertices.
*
*****************************************************************/
#include "painter.hpp"

// render
#include "atlas.hpp"
#include "emitter.hpp"

using namespace std;

namespace rr {

namespace {

using ::base::maybe;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

template<typename F>
void emit_solid_quad( rect dst, F&& f ) {
  dst            = dst.normalized();
  int dst_left   = dst.origin.x;
  int dst_right  = dst.origin.x + dst.size.w;
  int dst_top    = dst.origin.y;
  int dst_bottom = dst.origin.y + dst.size.h;

  f( point{ .x = dst_left, .y = dst_top } );
  f( point{ .x = dst_left, .y = dst_bottom } );
  f( point{ .x = dst_right, .y = dst_bottom } );
  f( point{ .x = dst_left, .y = dst_top } );
  f( point{ .x = dst_right, .y = dst_top } );
  f( point{ .x = dst_right, .y = dst_bottom } );
}

template<typename F>
void emit_texture_quad( rect src, rect dst, F&& f ) {
  src            = src.normalized();
  dst            = dst.normalized();
  int src_left   = src.origin.x;
  int src_right  = src.origin.x + src.size.w;
  int src_top    = src.origin.y;
  int src_bottom = src.origin.y + src.size.h;
  int dst_left   = dst.origin.x;
  int dst_right  = dst.origin.x + dst.size.w;
  int dst_top    = dst.origin.y;
  int dst_bottom = dst.origin.y + dst.size.h;

  f( point{ .x = dst_left, .y = dst_top },
     point{ .x = src_left, .y = src_top } );
  f( point{ .x = dst_left, .y = dst_bottom },
     point{ .x = src_left, .y = src_bottom } );
  f( point{ .x = dst_right, .y = dst_bottom },
     point{ .x = src_right, .y = src_bottom } );
  f( point{ .x = dst_left, .y = dst_top },
     point{ .x = src_left, .y = src_top } );
  f( point{ .x = dst_right, .y = dst_top },
     point{ .x = src_right, .y = src_top } );
  f( point{ .x = dst_right, .y = dst_bottom },
     point{ .x = src_right, .y = src_bottom } );
}

} // namespace

/****************************************************************
** Painter
*****************************************************************/
void Painter::emit( VertexBase&& vert ) {
  if( mods_ ) add_mods( vert, *mods_ );
  emitter_.emit( vert );
}

base::maybe<PainterMods const&> Painter::mods() const {
  if( mods_ == nullptr ) return base::nothing;
  return *mods_;
}

Painter Painter::with_mods( PainterMods const& mods ) {
  Painter res = *this;
  res.mods_   = &mods;
  return res;
}

Painter Painter::without_mods() {
  Painter res = *this;
  res.mods_   = {};
  return res;
}

void Painter::add_mods( VertexBase&        vert,
                        PainterMods const& mods ) {
  if( mods.depixelate.stage.has_value() )
    vert.set_depixelation_stage( *mods.depixelate.stage );
  if( mods.depixelate.hash_anchor.has_value() )
    vert.set_depixelation_hash_anchor(
        *mods.depixelate.hash_anchor );
  if( mods.depixelate.stage_gradient.has_value() ) {
    // Sanity check to help debugging.
    CHECK( mods.depixelate.stage.has_value(),
           "to use a depixelation gradient you have to set a "
           "depixelation stage." );
    CHECK( mods.depixelate.stage_anchor.has_value(),
           "depixelation stage gradient set but not stage "
           "anchor." );
    vert.set_depixelation_gradient(
        *mods.depixelate.stage_gradient );
  }
  if( mods.depixelate.stage_anchor.has_value() ) {
    // Sanity check to help debugging.
    CHECK( mods.depixelate.stage.has_value(),
           "to use a depixelation gradient you have to set a "
           "depixelation stage." );
    CHECK( mods.depixelate.stage_gradient.has_value(),
           "depixelation stage anchor set but not stage "
           "gradient." );
    vert.set_depixelation_stage_anchor(
        *mods.depixelate.stage_anchor );
  }
  if( mods.depixelate.inverted.has_value() )
    vert.set_depixelation_inversion( *mods.depixelate.inverted );
  if( mods.repos.scale.has_value() )
    vert.set_scaling( *mods.repos.scale );
  if( mods.repos.translation.has_value() )
    vert.set_translation( *mods.repos.translation );
  vert.set_use_camera( mods.repos.use_camera );
  if( mods.alpha.has_value() ) vert.set_alpha( *mods.alpha );
  if( mods.desaturate.has_value() )
    vert.set_desaturate( *mods.desaturate );
  if( mods.fixed_color.has_value() )
    vert.set_fixed_color( *mods.fixed_color );
  if( mods.uniform_depixelation.has_value() )
    vert.set_uniform_depixelation( *mods.uniform_depixelation );
  vert.set_color_cycle( mods.cycling.plan.has_value() );
  if( mods.cycling.plan.has_value() )
    vert.set_aux_idx(
        static_cast<int32_t>( *mods.cycling.plan ) );
}

Painter& Painter::draw_point( point p, pixel color ) {
  draw_solid_rect(
      rect{ .origin = p, .size = size{ .w = 1, .h = 1 } },
      color );
  return *this;
}

Painter& Painter::draw_horizontal_line( point start, int length,
                                        pixel color ) {
  emit_solid_quad(
      rect{ .origin = start, .size = { .w = length, .h = 1 } },
      [&, this]( point p ) {
        emit( SolidVertex( p, color ) );
      } );
  return *this;
}

Painter& Painter::draw_vertical_line( point start, int length,
                                      pixel color ) {
  emit_solid_quad(
      rect{ .origin = start, .size = { .w = 1, .h = length } },
      [&, this]( point p ) {
        emit( SolidVertex( p, color ) );
      } );
  return *this;
}

void Painter::draw_empty_box( rect r, pixel color ) {
  r = r.normalized();
  draw_horizontal_line( r.nw(), r.size.w, color );
  draw_vertical_line( r.ne(), r.size.h + 1, color );
  draw_horizontal_line( r.sw(), r.size.w, color );
  draw_vertical_line( r.nw(), r.size.h + 1, color );
}

Painter& Painter::draw_empty_rect( rect r, e_border_mode mode,
                                   pixel color ) {
  switch( mode ) {
    case e_border_mode::outside: {
      point nw = r.nw();
      point se = r.se();
      nw.x -= 1;
      nw.y -= 1;
      draw_empty_box( rect::from( nw, se ), color );
      break;
    }
    case e_border_mode::inside: {
      if( r.size.w == 0 || r.size.h == 0 ) return *this;
      // r is expected to be normalized here.
      r = r.normalized();
      draw_empty_box( rect{ .origin = r.nw(),
                            .size   = { .w = r.size.w - 1,
                                        .h = r.size.h - 1 } },
                      color );
      break;
    }
    case e_border_mode::in_out: {
      r = r.normalized();
      ++r.size.w;
      ++r.size.h;
      return draw_empty_rect( r, e_border_mode::inside, color );
    }
  }
  return *this;
}

Painter& Painter::draw_empty_rect( rect r, pixel color ) {
  return draw_empty_rect( r, e_border_mode::in_out, color );
}

Painter& Painter::draw_solid_rect( rect r, pixel color ) {
  emit_solid_quad( r, [&, this]( point p ) {
    emit( SolidVertex( p, color ) );
  } );
  return *this;
}

void Painter::draw_sprite_impl( rect src, rect dst ) {
  emit_texture_quad(
      src, dst, [&, this]( point pos, point atlas_pos ) {
        emit( SpriteVertex( pos, atlas_pos, src ) );
      } );
}

void Painter::draw_silhouette_impl( rect src, rect dst,
                                    gfx::pixel color ) {
  emit_texture_quad(
      src, dst, [&, this]( point pos, point atlas_pos ) {
        // FIXME: this is a bit hacky. Ideally we should get rid
        // of silhouettes, but that will require a bit of refac-
        // toring of rr::Typer.
        auto vert = SpriteVertex( pos, atlas_pos, src );
        vert.set_fixed_color( color );
        emit( std::move( vert ) );
      } );
}

void Painter::draw_stencil_impl(
    rect src, rect dst, gfx::size replacement_atlas_offset,
    gfx::pixel key_color ) {
  emit_texture_quad(
      src, dst, [&, this]( point pos, point atlas_pos ) {
        emit( StencilVertex( pos, atlas_pos, src,
                             replacement_atlas_offset,
                             key_color ) );
      } );
}

Painter& Painter::draw_sprite( int atlas_id, point where ) {
  rect src = atlas_.lookup( atlas_id );
  draw_sprite_impl( src,
                    rect{ .origin = where, .size = src.size } );
  return *this;
}

Painter& Painter::draw_sprite_scale( int atlas_id, rect dst ) {
  rect src = atlas_.lookup( atlas_id );
  draw_sprite_impl( src, dst );
  return *this;
}

Painter& Painter::draw_sprite_section(
    int atlas_id, gfx::point where, gfx::rect const section ) {
  rect const  atlas_src = atlas_.lookup( atlas_id );
  maybe<rect> src =
      rect{ .origin = atlas_src.origin +
                      section.origin.distance_from_origin(),
            .size = section.size }
          .clipped_by( atlas_src );
  if( !src.has_value() ) return *this;
  draw_sprite_impl( *src,
                    rect{ .origin = where, .size = src->size } );
  return *this;
}

Painter& Painter::draw_silhouette( int atlas_id, point where,
                                   pixel color ) {
  rect src = atlas_.lookup( atlas_id );
  draw_silhouette_impl(
      src, rect{ .origin = where, .size = src.size }, color );
  return *this;
}

Painter& Painter::draw_silhouette_scale( int atlas_id, rect dst,
                                         pixel color ) {
  rect src = atlas_.lookup( atlas_id );
  draw_silhouette_impl( src, dst, color );
  return *this;
}

Painter& Painter::draw_stencil( int        atlas_id,
                                int        replacement_atlas_id,
                                gfx::point where,
                                gfx::pixel key_color ) {
  rect      src = atlas_.lookup( atlas_id );
  rect      dst = rect{ .origin = where, .size = src.size };
  rect      replacement = atlas_.lookup( replacement_atlas_id );
  gfx::size replacement_atlas_offset =
      replacement.origin - src.origin;
  draw_stencil_impl( src, dst, replacement_atlas_offset,
                     key_color );
  return *this;
}

} // namespace rr
