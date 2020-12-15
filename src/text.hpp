/****************************************************************
**text.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-10.
*
* Description: Handles rendering larger bodies of text with
*              markup.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "coord.hpp"
#include "font.hpp"
#include "macros.hpp"
#include "tx.hpp"

// C++ standard library
#include <tuple>

// FIXME: get rid of this
// Place this into a struct to add the necessary members to make
// it useful as a key in a cache. Must be at the very bottom of
// the struct. Will static_assert that no fields have been left
// out.
#define MAKE_CACHE_KEY( name, ... )                            \
  auto to_tuple() const { return std::tuple{ __VA_ARGS__ }; }  \
                                                               \
  friend bool operator==( name const& lhs, name const& rhs ) { \
    return lhs.to_tuple() == rhs.to_tuple();                   \
  }                                                            \
  }                                                            \
  ;                                                            \
  static_assert( sizeof( decltype( name{}.to_tuple() ) ) ==    \
                 sizeof( name ) );                             \
  struct ____##name {
//

namespace rn {

// Note: these functions cache/reuse results, and so the textures
// returned may be weak references to textures held in the cache.
// However, periodically those textures will be cleaned up, and
// so it is important not to assume anything about the lifetimes
// of the textures referred to by the return values of these
// functions. If you need the texture to last then you should
// clone it.

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is and with the given
// color. Will ignore markup (will render it literally).
Texture const& render_text( e_font font, Color color,
                            std::string_view text );

// Same as above but uses the default font.
Texture const& render_text( std::string_view text, Color color );

// The struct gives the engine information on how to interpret
// the markup language.
struct TextMarkupInfo {
  Color normal;
  Color highlight;
  // !! Update std::hash specialization when adding new fields!

  // Adds some member functions to make this struct a cache key.
  MAKE_CACHE_KEY( TextMarkupInfo, normal, highlight );
};
NOTHROW_MOVE( TextMarkupInfo );

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
Texture const& render_text_markup( e_font                font,
                                   TextMarkupInfo const& info,
                                   std::string_view      text );

struct TextReflowInfo {
  int max_cols;
  // !! Update std::hash specialization when adding new fields!

  // Adds some member functions to make this struct a cache key.
  MAKE_CACHE_KEY( TextReflowInfo, max_cols );
};
NOTHROW_MOVE( TextReflowInfo );

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
Texture const& render_text_markup_reflow(
    e_font font, TextMarkupInfo const& m_info,
    TextReflowInfo const& r_info, std::string_view text );

/****************************************************************
** Debugging
*****************************************************************/
int text_cache_size();

/****************************************************************
** Testing
*****************************************************************/
// For testing
void text_render_test();

} // namespace rn

namespace std {

template<>
struct hash<::rn::TextMarkupInfo> {
  auto operator()(
      ::rn::TextMarkupInfo const& o ) const noexcept {
    uint64_t i = o.normal.to_uint32();
    i <<= 32;
    i += o.highlight.to_uint32();
    return hash<uint64_t>{}( i );
  }
};

template<>
struct hash<::rn::TextReflowInfo> {
  auto operator()(
      ::rn::TextReflowInfo const& o ) const noexcept {
    return hash<int>{}( o.max_cols );
  }
};

} // namespace std
