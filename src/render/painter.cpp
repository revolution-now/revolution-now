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

Painter Painter::with_mods( PainterMods const& mods ) {
  Painter res = *this;
  res.mods_   = mods;
  return res;
}

Painter Painter::without_mods() {
  Painter res = *this;
  res.mods_.reset();
  return res;
}

void Painter::add_mods( VertexBase&        vert,
                        PainterMods const& mods ) {
  if( mods.depixelate.has_value() )
    vert.set_depixelation_state( *mods.depixelate );
  if( mods.alpha.has_value() ) vert.set_alpha( *mods.alpha );
}

void Painter::draw_point( point p, pixel color ) {
  draw_solid_rect(
      rect{ .origin = p, .size = size{ .w = 1, .h = 1 } },
      color );
}

void Painter::draw_horizontal_line( point start, int length,
                                    pixel color ) {
  emit_solid_quad(
      rect{ .origin = start, .size = { .w = length, .h = 1 } },
      [&, this]( point p ) {
        emit( SolidVertex( p, color ) );
      } );
}

void Painter::draw_vertical_line( point start, int length,
                                  pixel color ) {
  emit_solid_quad(
      rect{ .origin = start, .size = { .w = 1, .h = length } },
      [&, this]( point p ) {
        emit( SolidVertex( p, color ) );
      } );
}

void Painter::draw_empty_box( rect r, pixel color ) {
  r = r.normalized();
  draw_horizontal_line( r.nw(), r.size.w, color );
  draw_vertical_line( r.ne(), r.size.h + 1, color );
  draw_horizontal_line( r.sw(), r.size.w, color );
  draw_vertical_line( r.nw(), r.size.h + 1, color );
}

void Painter::draw_empty_rect( rect r, e_border_mode mode,
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
      if( r.size.w == 0 || r.size.h == 0 ) return;
      // r is expected to be normalized here.
      r = r.normalized();
      draw_empty_box( rect{ .origin = r.nw(),
                            .size   = { .w = r.size.w - 1,
                                        .h = r.size.h - 1 } },
                      color );
      break;
    }
  }
}

void Painter::draw_solid_rect( rect r, pixel color ) {
  emit_solid_quad( r, [&, this]( point p ) {
    emit( SolidVertex( p, color ) );
  } );
}

void Painter::draw_sprite_impl( rect src, rect dst ) {
  emit_texture_quad( src, dst,
                     [&, this]( point pos, point atlas_pos ) {
                       emit( SpriteVertex( pos, atlas_pos ) );
                     } );
}

void Painter::draw_silhouette_impl( rect src, rect dst,
                                    gfx::pixel color ) {
  emit_texture_quad(
      src, dst, [&, this]( point pos, point atlas_pos ) {
        emit( SilhouetteVertex( pos, atlas_pos, color ) );
      } );
}

void Painter::draw_sprite( int atlas_id, point where ) {
  rect src = atlas_.lookup( atlas_id );
  draw_sprite_impl( src,
                    rect{ .origin = where, .size = src.size } );
}

void Painter::draw_sprite_scale( int atlas_id, rect dst ) {
  rect src = atlas_.lookup( atlas_id );
  draw_sprite_impl( src, dst );
}

void Painter::draw_silhouette( int atlas_id, point where,
                               pixel color ) {
  rect src = atlas_.lookup( atlas_id );
  draw_silhouette_impl(
      src, rect{ .origin = where, .size = src.size }, color );
}

void Painter::draw_silhouette_scale( int atlas_id, rect dst,
                                     pixel color ) {
  rect src = atlas_.lookup( atlas_id );
  draw_silhouette_impl( src, dst, color );
}

} // namespace rr
