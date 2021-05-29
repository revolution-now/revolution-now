/****************************************************************
**gfx-api.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-03-08.
*
* Description: API for a batching GPU-based 2D rendering.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "typed-int.hpp"

// Abseil
#include "absl/types/span.h"

TYPED_ID_NS( rn::render, GfxObjId );

namespace rn::render {

//////////////////////// Initialization /////////////////////////
valid_or<generic_err> initialize();

/////////////////////// Screen Rendering ////////////////////////
void begin_render_to_screen();
void end_render_to_screen();

/////////////////////// Buffer Rendering ////////////////////////
void                   begin_cpu_buffer();
std::vector<std::byte> end_cpu_buffer();

void     begin_gpu_buffer();
GfxObjId end_gpu_buffer();

//////////////////////////// Textures ///////////////////////////
void create_texture_from_bytes(
    absl::Span<std::byte const> bytes );

void free_texture( GfxObjId id );

//////////////////////// Texture Atlases ////////////////////////
GfxObjId set_texture_atlas( GfxObjId id );

////////////////////// Rendering Buffers / //////////////////////
void render_cpu_buffer( std::vector<std::byte> const& buf );
void render_gpu_buffer( GfxObjId id );

///////////////////// Rendering Primitives //////////////////////
void line( Coord start, Coord end, Color c );

void rect_unfilled( Coord corner, Coord opposite_corner,
                    Color c );
void rect_unfilled_inside( Rect const& rect, Color c );
void rect_unfilled_outside( Rect const& rect, Color c );

void rect_filled( Coord corner, Coord opposite_corner, Color c );
void rect_filled_inside( Rect const& rect, Color c );
void rect_filled_outside( Rect const& rect, Color c );

void texture( Rect const& atlas_rect, Coord const& target );
void texture( Rect const& atlas_rect, Rect const& target );

void texture( GfxObjId id, Rect const& src_rect,
              Coord const& target );
void texture( GfxObjId id, Rect const& src_rect,
              Rect const& target );

} // namespace rn::render