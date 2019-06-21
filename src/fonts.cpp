/****************************************************************
**fonts.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-22.
*
* Description: Code for handling all things text.
*
*****************************************************************/
#include "fonts.hpp"

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "init.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

// Revolution Now (config)
#include "config/ucl/font.inl"
#include "config/ucl/palette.inl"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

// SDL
#include "SDL_ttf.h"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

struct FontDesc {
  fs::path    file_name;
  int         pt_size;
  Y           vert_offset;
  ::TTF_Font* ttf_font;
};

FlatMap<e_font, FontDesc>& loaded_fonts() {
  static FlatMap<e_font, FontDesc> m = [] {
    FlatMap<e_font, FontDesc> res;
    for( auto font : values<e_font> ) {
      auto& path = val_or_die( config_font.paths, font );
      auto& size = val_or_die( config_font.sizes, font );
      auto& vert_offset =
          val_or_die( config_font.offsets, font );
      res[font] = FontDesc{path, size, vert_offset, nullptr};
    }
    return res;
  }();
  return m;
}

Texture render_line_standard_impl( ::TTF_Font* font,
                                   ::SDL_Color fg,
                                   string_view line,
                                   Y           vert_offset ) {
  ASSIGN_CHECK( surface,
                ::TTF_RenderText_Solid(
                    font, string( line ).c_str(), fg ) );
  auto texture = Texture::from_surface( surface );
  ::SDL_FreeSurface( surface );
  if( vert_offset != 0_y ) {
    auto new_texture = create_texture( texture.size() );
    clear_texture_transparent( new_texture );
    copy_texture( texture, new_texture, {0_x, vert_offset} );
    texture = std::move( new_texture );
  }
  // Not sure why this doesn't happen automatically.
  ::SDL_SetTextureAlphaMod( texture, fg.a );
  return texture;
}

struct TextRenderDesc {
  e_font font;
  Color  color;
  string line;

  auto to_tuple() const { return tuple{font, color, line}; }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, TextRenderDesc const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }

  friend bool operator==( TextRenderDesc const& lhs,
                          TextRenderDesc const& rhs ) {
    return lhs.to_tuple() == rhs.to_tuple();
  }
};

absl::flat_hash_map<TextRenderDesc, Texture> text_cache_solid;

void init_fonts() {
  CHECK( !TTF_Init() );
  for( auto& font : loaded_fonts() ) {
    auto& font_desc = font.second;
    int   pt_size   = font_desc.pt_size;
    auto  font_file = font_desc.file_name.string();
    ASSIGN_CHECK( ttf_font,
                  ::TTF_OpenFont( font_file.c_str(), pt_size ) );
    // Check style first before setting this.
    ::TTF_SetFontStyle( ttf_font, TTF_STYLE_NORMAL );
    int outline = 0;
    ::TTF_SetFontOutline( ttf_font, outline );
    font_desc.ttf_font = ttf_font;
  }
}

void cleanup_fonts() {
  for( auto& font : loaded_fonts() ) {
    auto& font_desc = font.second;
    ::TTF_CloseFont( font_desc.ttf_font );
  }
  for( auto& p : text_cache_solid ) p.second.free();
  TTF_Quit();
}

REGISTER_INIT_ROUTINE( fonts );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
namespace fonts {

e_font standard() { return config_font.game_default; }

} // namespace fonts

// All text rendering should ultimately go through this function
// because it does the cache handling.
Texture render_text_line_solid( e_font font, Color fg,
                                string_view line ) {
  auto do_render = [&] {
    auto* ttf_font    = loaded_fonts()[font].ttf_font;
    auto  vert_offset = loaded_fonts()[font].vert_offset;
    return render_line_standard_impl( ttf_font, to_SDL( fg ),
                                      line, vert_offset );
  };

  TextRenderDesc desc{font, fg, string( line )};

  if( auto maybe_cached =
          util::get_val_safe( text_cache_solid, desc );
      maybe_cached.has_value() )
    return maybe_cached.value().get().weak_ref();

  text_cache_solid.emplace( desc, do_render() );
  return text_cache_solid[desc].weak_ref();
}

Texture render_text_line_shadow( e_font font, Color fg,
                                 string_view line ) {
  Color bg        = fg.shaded( 6 );
  bg.a            = 80;
  auto texture_fg = render_text_line_solid( font, fg, line );
  auto texture_bg = render_text_line_solid( font, bg, line );
  auto delta      = texture_delta( texture_fg );
  auto result_texture =
      create_texture( delta.w + 1, delta.h + 1 );
  clear_texture_transparent( result_texture );
  copy_texture( texture_bg, result_texture, {1_y, 1_x} );
  copy_texture( texture_fg, result_texture, {0_y, 0_x} );
  return result_texture;
}

using RenderLineFn = std::function<Texture( string const& )>;

Texture render_lines( H min_skip, vector<string> const& lines,
                      RenderLineFn const& render_line ) {
  auto textures = util::map( render_line, lines );
  auto deltas   = util::map( texture_delta, textures );
  H    res_height( 0 );
  W    res_width( 0 );
  for( auto const& delta : deltas ) {
    res_height += std::max( min_skip, delta.h );
    res_width = std::max( res_width, delta.w );
  }

  auto result_texture = create_texture( res_width, res_height );

  Y y( 0 );
  for( size_t i = 0; i < textures.size(); ++i ) {
    copy_texture( textures[i], result_texture, {y, 0_x} );
    y += std::max( min_skip, deltas[i].h );
  }
  return result_texture;
}

Texture render_wrapped_text(
    H min_skip, string_view text,
    RenderLineFn const&      render_line,
    util::IsStrOkFunc const& width_checker ) {
  auto wrapped = util::wrap_text_fn( text, width_checker );
  return render_lines( min_skip, wrapped, render_line );
}

Texture render_wrapped_text_by_pixels(
    ::TTF_Font* font, int max_pixel_width, H min_skip,
    string_view text, RenderLineFn const& render_line ) {
  auto width_checker = [font,
                        max_pixel_width]( string_view text ) {
    int w, h;
    ::TTF_SizeText( font, string( text ).c_str(), &w, &h );
    return w <= max_pixel_width;
  };
  auto wrapped = util::wrap_text_fn( text, width_checker );
  return render_lines( min_skip, wrapped, render_line );
}

void font_size_spectrum( char const* msg,
                         char const* font_file ) {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  int  y     = 30;
  auto sizes = {
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
  for( auto ptsize : sizes ) {
    ASSIGN_CHECK( font, ::TTF_OpenFont( font_file, ptsize ) );
    std::string num_msg = std::to_string( ptsize ) + ": " + msg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    ::SDL_Color fg{255, 255, 255, 255};
    auto        texture =
        render_line_standard_impl( font, fg, num_msg, 0_y );
    copy_texture( texture, Texture{}, {Y( y ), 0_x} );
    y += ::TTF_FontLineSkip( font );
    TTF_CloseFont( font );
  }
}

Delta font_rendered_width( e_font font, string_view text ) {
  int w, h;
  ::TTF_SizeText( loaded_fonts()[font].ttf_font,
                  string( text ).c_str(), &w, &h );
  return {W( w ), H( h )};
}

void font_test() {
  auto font = e_font::_7_12_serif_16pt;

  H skip( ::TTF_FontLineSkip( loaded_fonts()[font].ttf_font ) );

  char const* msg =
      "Ask not what your country can do for you, "
      "but instead ask what you can do for your country!";

  auto render_line = [font]( string const& text ) {
    Color fg = config_palette.orange.sat1.lum11;
    return render_text_line_shadow( font, fg, text );
  };
  auto texture = render_wrapped_text( skip, msg, render_line,
                                      L( _.size() <= 20 ) );

  copy_texture( texture, Texture{}, {100_y, 100_x} );
  // font_size_spectrum( msg, font_file );

  ::SDL_RenderPresent( g_renderer );

  input::wait_for_q();
}

} // namespace rn
