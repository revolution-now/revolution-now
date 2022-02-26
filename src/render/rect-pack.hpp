/****************************************************************
**rect-pack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Rect Packer.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"
#include "base/to-str.hpp"

// C++ standard library
#include <span>

// TODO: move these into gfx and give them reflection.
namespace gfx {

struct size {
  int w = 0;
  int h = 0;

  bool negative() const { return w < 0 || h < 0; };

  int area() const { return w * h; }

  size max_with( size const& rhs ) const {
    return size{ std::max( w, rhs.w ), std::max( h, rhs.h ) };
  }

  friend void to_str( size const& o, std::string& out,
                      base::ADL_t tag ) {
    out += "size{w=";
    to_str( o.w, out, tag );
    out += ",h=";
    to_str( o.h, out, tag );
    out += '}';
  }

  bool operator==( size const& ) const = default;
};

struct point {
  int x = 0;
  int y = 0;

  static point const& origin() {
    static point p = {};
    return p;
  }

  friend void to_str( point const& o, std::string& out,
                      base::ADL_t tag ) {
    out += "point{x=";
    to_str( o.x, out, tag );
    out += ",y=";
    to_str( o.y, out, tag );
    out += '}';
  }

  bool operator==( point const& ) const = default;
};

struct rect {
  point     origin = {}; // upper left.
  gfx::size size   = {};

  int area() const { return size.area(); }

  friend void to_str( rect const& o, std::string& out,
                      base::ADL_t tag ) {
    out += "rect{origin=";
    to_str( o.origin, out, tag );
    out += ",size=";
    to_str( o.size, out, tag );
    out += '}';
  }

  bool operator==( rect const& ) const = default;
};

} // namespace gfx

namespace rr {

struct rect_to_pack {
  // Inputs.
  // ------------------------------------------------------------
  // Each input should have a unique ID. This won't be enforced,
  // but the results probably wouldn't be easily usable without
  // this.
  int id = {};

  // Size of the rect to be packed.
  gfx::size size = {};

  // Outputs.
  // ------------------------------------------------------------
  // Origin of this rect has after packing.
  gfx::point origin = {};
};

void to_str( rect_to_pack const& o, std::string& out,
             base::ADL_t tag );

struct packing_stats {
  bool operator==( packing_stats const& ) const = default;

  double percent_occupancy() const {
    return double( area_occupied ) / size_used.area();
  }

  // Max width and height of packing region used to hold the
  // rects.
  gfx::size size_used = {};

  // Accumulated area currently occupied by packed rects.
  int area_occupied = 0;
};

void to_str( packing_stats const& o, std::string& out,
             base::ADL_t tag );

base::maybe<packing_stats> pack_rects(
    std::span<rect_to_pack> rp, gfx::size const& max_size,
    bool trace = false );

} // namespace rr
