/****************************************************************
**texture.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-12.
*
* Description: C++ representation of an OpenGL texture.
*
*****************************************************************/
#pragma once

// gl
#include "bindable.hpp"
#include "types.hpp"

// gfx
#include "gfx/image.hpp"

// base
#include "base/zero.hpp"

namespace gl {

/****************************************************************
** Texture
*****************************************************************/
struct Texture : base::zero<Texture, ObjId>, bindable<Texture> {
  Texture();

  Texture( gfx::image const& img );

  void set_image( gfx::image const& img );

 private:
  Texture( ObjId tx_id );

 private:
  // Implement base::zero.
  friend base::zero<Texture, ObjId>;

  void free_resource();

 private:
  // Implement bindable.
  friend bindable<Texture>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

} // namespace gl
