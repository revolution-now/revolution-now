/****************************************************************
**atlas.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-04.
*
* Description: Creates a texture atlas.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/image.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <vector>

namespace rr {

/****************************************************************
** AtlasMap
*****************************************************************/
// Used to get the bounding rect around a sprite in the atlas
// given its ID. The ID of a sprite is what is returned from the
// add_sprite method below when building the atlas.
struct AtlasMap {
  AtlasMap( std::vector<gfx::rect> rects )
    : rects_( std::move( rects ) ) {}

  gfx::rect const& lookup( int id ) const;

  int size() const { return rects_.size(); }

 private:
  // The ID of the sprite is just the index into this vector.
  std::vector<gfx::rect> rects_;
};

/****************************************************************
** Atlas
*****************************************************************/
// This represents a texture atlas. It has the finished atlas
// image (in CPU memory, ready to be copied to the GPU) with all
// of the sprites packed and copied into it, along with a lookup
// interface for looking up the bounding rectangle for a sprite
// given its ID.
struct Atlas {
  // This image can be released without disturbing the lookup
  // mechanism.
  gfx::image img;
  AtlasMap   dict;
};

/****************************************************************
** AtlasBuilder
*****************************************************************/
// This gets passed around allowing different systems to allocate
// their own ID ranges and to add their own textures before the
// final texture packing and copying are done.
struct AtlasBuilder {
  struct [[nodiscard]] ImageBuilder {
    ImageBuilder( AtlasBuilder& builder )
      : atlas_builder_( builder ) {}

    // This rect should be the bounds of the sprite in the source
    // image. Returns the ID of the sprite added. Henceforth that
    // ID must always be used to refer to the sprite in the atlas
    // (so it should be saved).
    [[nodiscard]] int add_sprite( gfx::rect const r ) const;

   private:
    AtlasBuilder& atlas_builder_;
  };

  // Call this once for each new image containing sprites.
  ImageBuilder add_image( gfx::image img ) &;

  // This is expensive... only do this once at the end! It can
  // fail if the rects can't be packed into the max_size dimen-
  // sions using the packing algorithm that we're using (which is
  // not optimal...).
  base::maybe<Atlas> build( gfx::size max_size ) const;

 private:
  struct AtlasImage {
    gfx::image img;
    int        count = 0;
  };

  // This keeps track of how many rects there are per image.
  std::vector<AtlasImage> images_;

  // All the rects are just appended here, and each rect's ID is
  // just its index in the vector.
  std::vector<gfx::rect> rects_;
};

} // namespace rr
