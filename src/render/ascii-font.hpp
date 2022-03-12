/****************************************************************
**ascii-font.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Representation of an ascii font from a sprite
*              sheet.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <array>
#include <memory>

namespace rr {

/****************************************************************
** AsciiFont
*****************************************************************/
struct AsciiFont {
  AsciiFont( std::unique_ptr<std::array<int, 256>> ids,
             gfx::size                             char_size )
    : atlas_ids_( std::move( ids ) ), char_size_{ char_size } {}

  gfx::size char_size() const { return char_size_; }

  int atlas_id_for_char( uint8_t c ) const {
    return ( *atlas_ids_ )[c];
  }

 private:
  std::unique_ptr<std::array<int, 256>> atlas_ids_;
  gfx::size                             char_size_;
};

} // namespace rr
