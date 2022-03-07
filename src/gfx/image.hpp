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
#include "cartesian.hpp"
#include "pixel.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <cstddef>
#include <span>

namespace gfx {

/****************************************************************
** image
*****************************************************************/
// Holds ownership over a byte buffer representing an image. Each
// pixel is assumed to be four bytes (RGBA).
struct image : base::zero<image, unsigned char*> {
  // The data pointer passed in is assumed allocated by malloc.
  // This class will take ownership of it and release it with
  // free.
  image( size size_pixels, unsigned char* data );

  image( image&& ) = default;
  image& operator=( image&& ) = default;

  gfx::pixel const& at( point p ) const;
  gfx::pixel&       at( point p );

  gfx::pixel const& operator[]( point p ) const;
  gfx::pixel&       operator[]( point p );

  size size_pixels() const;
  rect rect_pixels() const;
  int  height_pixels() const;
  int  width_pixels() const;
  int  size_bytes() const;
  int  total_pixels() const;

  operator std::span<std::byte const>() const;
  operator std::span<char const>() const;
  operator std::span<unsigned char const>() const;

  operator std::span<pixel const>() const;
  operator std::span<pixel>();

  void blit_from( image const& other, rect const src,
                  point const dst_origin );

  static int constexpr kBytesPerPixel = 4;

  unsigned char* data() const;

 private:
  unsigned char* data_for( point const p ) const;

  image( image const& ) = delete;
  image& operator=( image const& ) = delete;

  size size_pixels_ = {};

  // Implement base::zero.
  friend base::zero<image, unsigned char*>;
  void free_resource();
};

/****************************************************************
** Helpers
*****************************************************************/
// Returns an image with all pixels set to zero (0,0,0,0).
image new_empty_image( size size_pixels );

// Returns an image with all pixels set to color.
image new_filled_image( size size_pixels, pixel color );

/****************************************************************
** Testing
*****************************************************************/
// Useful for testing.  Generally inefficient.
namespace testing {

bool compare_image( image const&           img,
                    std::span<pixel const> sp );

image new_image_from_pixels( size                   dimensions,
                             std::span<pixel const> sp );

}

} // namespace gfx
