/****************************************************************
**image.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-04.
*
* Description: C++ wrapper around stb_image.
*
*****************************************************************/
#pragma once

// base
#include "base/fs.hpp"
#include "base/maybe.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <cstddef>
#include <span>

namespace stb {

struct pixel {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
  unsigned char a = 0;
};

struct image : base::zero<image, unsigned char*> {
  pixel get( int y, int x ) const;

  int height_pixels() const;
  int width_pixels() const;
  int size_bytes() const;
  int total_pixels() const;

  operator std::span<std::byte const>() const;
  operator std::span<char const>() const;
  operator std::span<unsigned char const>() const;

  static int constexpr kBytesPerPixel = 4;

  image( int width_pixels, int height_pixels,
         unsigned char* data );

  unsigned char* data() const;

 private:
  int width_pixels_  = 0;
  int height_pixels_ = 0;

  // Implement base::zero.
  friend base::zero<image, unsigned char*>;
  void free_resource();
};

image load_image( fs::path const& p );

} // namespace stb
