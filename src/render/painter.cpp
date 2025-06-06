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
using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

void emit_triangle( point const p1, point const p2,
                    point const p3, auto const& f ) {
  f( p1 );
  f( p2 );
  f( p3 );
}

void emit_solid_quad( rect const dst, auto const& f ) {
  auto const r = dst.normalized();
  emit_triangle( r.nw(), r.sw(), r.se(), f );
  emit_triangle( r.nw(), r.ne(), r.se(), f );
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

PainterMods const& Painter::mods() const {
  static PainterMods const empty;
  if( mods_ == nullptr ) return empty;
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

void Painter::add_mods( VertexBase& vert,
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
  if( mods.repos.translation1.has_value() )
    vert.set_translation1( *mods.repos.translation1 );
  if( mods.repos.translation2.has_value() )
    vert.set_translation2( *mods.repos.translation2 );
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
    vert.set_color_cycle_plan( *mods.cycling.plan );
  if( mods.sampling.downsample.has_value() )
    vert.set_downsampling_power( *mods.sampling.downsample );
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

Painter& Painter::draw_solid_triangle( point const p1,
                                       point const p2,
                                       point const p3,
                                       pixel const color ) {
  emit_triangle( p1, p2, p3, [&]( point const p ) {
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
  // Here we need to be careful not to let any of the lines
  // overlap even by one pixel, since if alpha compositing is in-
  // volved that can cause the corners to be more opaque than the
  // sides, which does not look good.
  draw_horizontal_line( r.nw(), r.size.w, color );
  draw_vertical_line( r.ne(), r.size.h + 1, color );
  draw_horizontal_line( r.sw(), r.size.w, color );
  draw_vertical_line( r.nw() + size{ .h = 1 }, r.size.h - 1,
                      color );
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

Painter& Painter::draw_sprite_impl( rect total_src,
                                    point dst_origin,
                                    maybe<size> dst_size,
                                    maybe<rect> section ) {
  rect const src =
      section ? section->origin_becomes_point( total_src.origin )
                    .clipped_by( total_src )
                    .value_or( rect{} )
              : total_src;

  rect const dst{ .origin = dst_origin,
                  .size   = dst_size.value_or( src.size ) };

  if( auto const& stencil = mods().stencil; stencil ) {
    auto const& [replacement_atlas_id, key_color] = *stencil;
    gfx::size const replacement_atlas_offset =
        atlas_.lookup( replacement_atlas_id ).origin -
        total_src.origin;
    emit_texture_quad(
        src, dst, [&, this]( point pos, point atlas_pos ) {
          emit( StencilVertex( pos, atlas_pos, src,
                               replacement_atlas_offset,
                               key_color ) );
        } );
    return *this;
  }

  emit_texture_quad(
      src, dst, [&, this]( point pos, point atlas_pos ) {
        emit( SpriteVertex( pos, atlas_pos, src ) );
      } );
  return *this;
}

Painter& Painter::draw_sprite( int const atlas_id,
                               point const where ) {
  return draw_sprite_impl( atlas_.lookup( atlas_id ), where,
                           /*dst_size=*/nothing,
                           /*section=*/nothing );
}

Painter& Painter::draw_sprite_scale( int const atlas_id,
                                     rect const dst ) {
  return draw_sprite_impl( atlas_.lookup( atlas_id ), dst.origin,
                           dst.size,
                           /*section=*/nothing );
}

Painter& Painter::draw_sprite_section(
    int const atlas_id, gfx::point const where,
    gfx::rect const section ) {
  return draw_sprite_impl( atlas_.lookup( atlas_id ), where,
                           /*dst_size=*/nothing, section );
}

Painter& Painter::draw_line( point start, point end,
                             pixel const color ) {
  point p1, p2, p3, p4;
  size const delta = ( end - start ).abs();
  // Now we need to create a thin pallelogram to bound the line
  // and limit the number of pixels that we need to shade, while
  // taking care to not exclude and pixels. The numbers chosen
  // below are basically the smallest they could be without
  // causing some part of the line in some configuration to get
  // chopped off.
  if( delta.h >= delta.w ) {
    if( start.y > end.y ) swap( start, end );
    // At this point the line will look something like this. Note
    // that it could be flipped horizontally, but that's ok.
    // . . . . . . . . . . .
    // . . . . . . . . . . .
    // . . . 1 . . . . . 2 .
    // . . . . . . S . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . . x . . . . .
    // . . . . E . . . . . .
    // . 4 . . . . . 3 . . .
    // . . . . . . . . . . .
    // . . . . . . . . . . .
    p1 = start.moved_left( 3 ).moved_up();
    p2 = start.moved_right( 3 ).moved_up();
    p3 = end.moved_right( 3 ).moved_down();
    p4 = end.moved_left( 3 ).moved_down();
  } else {
    if( start.x > end.x ) swap( start, end );
    // At this point the line will look something like this. Note
    // that it could be flipped vertically, but that's ok.
    // . . . . . . . . . . . . . . . . . . . .
    // . . 1 . . . . . . . . . . . . . . . . .
    // . . . . . . . . . . . . . . . . . . . .
    // . . . . . . . . . . . . . . . . . 4 . .
    // . . . S . . . . . . . . . . . . . . . .
    // . . . . x x x x x x x x x x x x . . . .
    // . . . . . . . . . . . . . . . . E . . .
    // . . 2 . . . . . . . . . . . . . . . . .
    // . . . . . . . . . . . . . . . . . . . .
    // . . . . . . . . . . . . . . . . . 3 . .
    // . . . . . . . . . . . . . . . . . . . .
    p1 = start.moved_up( 3 ).moved_left();
    p2 = start.moved_down( 3 ).moved_left();
    p3 = end.moved_down( 3 ).moved_right();
    p4 = end.moved_up( 3 ).moved_right();
  }
  // Draw the angled line.
  auto const vertex = [&, this]( point const p ) {
    emit( LineVertex( p, start, end, color ) );
  };
  vertex( p1 );
  vertex( p2 );
  vertex( p3 );
  vertex( p3 );
  vertex( p4 );
  vertex( p1 );
  return *this;
}

} // namespace rr
