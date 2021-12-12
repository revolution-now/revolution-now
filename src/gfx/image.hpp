/****************************************************************
**image.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Image representation.
*
*****************************************************************/
#pragma once

// gfx
#include "pixel.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <cstddef>
#include <span>

namespace gfx {

struct image : base::zero<image, unsigned char*> {
  // The data pointer passed in is assumed allocated by malloc.
  // This class will take ownership of it and release it with
  // free.
  image( int width_pixels, int height_pixels,
         unsigned char* data );

  gfx::pixel get( int y, int x ) const;

  int height_pixels() const;
  int width_pixels() const;
  int size_bytes() const;
  int total_pixels() const;

  operator std::span<std::byte const>() const;
  operator std::span<char const>() const;
  operator std::span<unsigned char const>() const;

  operator std::span<pixel const>() const;

  static int constexpr kBytesPerPixel = 4;

  unsigned char* data() const;

 private:
  int width_pixels_  = 0;
  int height_pixels_ = 0;

  // Implement base::zero.
  friend base::zero<image, unsigned char*>;
  void free_resource();
};

} // namespace gfx
