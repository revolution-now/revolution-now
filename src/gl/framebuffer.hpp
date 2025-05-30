/****************************************************************
**framebuffer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-27.
*
* Description: C++ representation of an OpenGL Framebuffer.
*
*****************************************************************/
#pragma once

// gl
#include "bindable.hpp"
#include "types.hpp"

// base
#include "base/zero.hpp"

namespace gl {

struct Texture;

/****************************************************************
** Framebuffer
*****************************************************************/
struct Framebuffer : base::zero<Framebuffer, ObjId>,
                     bindable<Framebuffer> {
  Framebuffer();

  void set_color_attachment( Texture const& tx );

  bool is_framebuffer_complete() const;

 private:
  Framebuffer( ObjId fb_id );

 private:
  // Implement base::zero.
  friend base::zero<Framebuffer, ObjId>;

  void free_resource();

 private:
  // Implement bindable.
  friend bindable<Framebuffer>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

} // namespace gl
