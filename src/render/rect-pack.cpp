/****************************************************************
**rect-pack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Rect Packer.
*
*****************************************************************/
#include "rect-pack.hpp"

// base
#include "base/fmt.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rr {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

#define RETURN_IF_FAILED( ... )          \
  if( packer::e_status st = __VA_ARGS__; \
      st == packer::e_status::failed )   \
    return {};

struct packer {
  enum class [[nodiscard]] e_status{ failed, good };
  static_assert( e_status{} == e_status::failed );

  template<typename... Args>
  void log( string_view fmt_str, Args&&... args ) const {
    if( !trace_ ) return;
    fmt::print( fmt::runtime( fmt_str ),
                std::forward<Args>( args )... );
    fmt::print( "\n" );
  }

  e_status pack_rect( rect_to_pack& what, rect const& allowed ) {
    log( "pack_rect( what={}, allowed={} )", what, allowed );
    if( allowed.size.h < what.size.h ||
        allowed.size.w < what.size.w )
      return e_status::failed;
    log( "==> packing id={}", what.id );
    point const& where = allowed.origin;
    what.origin        = where;
    stats_.area_occupied += what.size.area();
    stats_.size_used.w =
        std::max( stats_.size_used.w, where.x + what.size.w );
    stats_.size_used.h =
        std::max( stats_.size_used.h, where.y + what.size.h );
    ++cur_;
    return e_status::good;
  }

  e_status pack_cols( rect const& allowed ) {
    log( "pack_cols( allowed={} )", allowed );
    if( cur_ == end_ ) return e_status::good;

    // Try to pack the first rect.
    rect_to_pack& first = *cur_;
    RETURN_IF_FAILED( pack_rect( first, allowed ) );

    // Move to the remaining space in this column. Note that this
    // is not a failure if this fails to find a spot for the next
    // rect since we can always then try the next column.
    e_status status [[maybe_unused]] = pack_rows(
        rect{ .origin = { .x = allowed.origin.x,
                          .y = allowed.origin.y + first.size.h },
              .size   = { .w = first.size.w,
                          .h = allowed.size.h - first.size.h } } );

    // Move to the next column in this row.
    return pack_cols(
        rect{ .origin = { .x = allowed.origin.x + first.size.w,
                          .y = allowed.origin.y },
              .size   = { .w = allowed.size.w - first.size.w,
                          .h = allowed.size.h } } );
  }

  e_status pack_rows( rect const& allowed ) {
    log( "pack_rows( allowed={} )", allowed );
    if( cur_ == end_ ) return e_status::good;
    if( allowed.size.negative() ) return e_status::failed;
    rect_to_pack& first = *cur_;

    // Start the first row, whose height will be the height of
    // the first rect. Note that this is not a failure if this
    // fails to find a spot for the next rect since we can always
    // then try the next
    e_status status [[maybe_unused]] = pack_cols( rect{
        .origin = allowed.origin,
        .size   = {
              .w = allowed.size.w,
              .h = std::min( first.size.h, allowed.size.h ) } } );

    // Move to the remainder of this allowed region (i.e. subse-
    // quent rows).
    return pack_rows(
        rect{ .origin = { .x = allowed.origin.x,
                          .y = allowed.origin.y + first.size.h },
              .size   = { .w = allowed.size.w,
                          .h = allowed.size.h - first.size.h } } );
  }

  e_status pack_rects( span<rect_to_pack> rects,
                       rect const&        allowed ) {
    // The stable sort is mostly for unit testing, so that this
    // producing deterministic results when there are multiple
    // rects (with different ids) but with the same heights.
    std::stable_sort(
        rects.begin(), rects.end(),
        []( rect_to_pack const& l, rect_to_pack const& r ) {
          return l.size.h > r.size.h;
        } );
    log( "pack_rects sorted: {}", rects );
    return pack_rows( allowed );
  }

  span<rect_to_pack>::iterator       cur_   = {};
  span<rect_to_pack>::iterator const end_   = {};
  packing_stats                      stats_ = {};
  // For debugging.
  bool trace_ = false;
};

} // namespace

void to_str( rect_to_pack const& o, std::string& out,
             base::ADL_t tag ) {
  out += "rect_to_pack{id=";
  to_str( o.id, out, tag );
  out += ",size=";
  to_str( o.size, out, tag );
  out += ",origin=";
  to_str( o.origin, out, tag );
  out += '}';
}

void to_str( packing_stats const& o, std::string& out,
             base::ADL_t tag ) {
  out += "packing_stats{size_used=";
  to_str( o.size_used, out, tag );
  out += ",area_occupied=";
  to_str( o.area_occupied, out, tag );
  out += '}';
}

maybe<packing_stats> pack_rects( span<rect_to_pack> rects,
                                 size const&        max_size,
                                 bool               trace ) {
  // Sort in descending order of heights.
  packer p{ .cur_   = rects.begin(),
            .end_   = rects.end(),
            .trace_ = trace };
  RETURN_IF_FAILED( p.pack_rects(
      rects,
      rect{ .origin = point::origin(), .size = max_size } ) );
  return p.stats_;
}

} // namespace rr
