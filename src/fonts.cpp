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

#include "config-files.hpp"
#include "errors.hpp"
#include "globals.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/string.hpp"

#include "SDL_ttf.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

struct FontDesc {
  char const* file_name;
  int         pt_size;
  ::TTF_Font* ttf_font;
};

constexpr int _7_12_font_pt_size = 16;

unordered_map<e_font, FontDesc> loaded_fonts{
    {e_font::_7_12_serif_16pt,
     {"assets/fonts/7-12-serif/712_serif.ttf",
      _7_12_font_pt_size, nullptr}}};

Texture render_line_standard_impl( ::TTF_Font*   font,
                                   ::SDL_Color   fg,
                                   string const& line ) {
  ASSIGN_CHECK( surface, ::TTF_RenderText_Blended(
                             font, line.c_str(), fg ) );
  auto texture = Texture::from_surface( surface );
  ::SDL_FreeSurface( surface );
  // Not sure why this doesn't happen automatically.
  ::SDL_SetTextureAlphaMod( texture, fg.a );
  return texture;
}

} // namespace

Texture render_line_standard( e_font font, Color fg,
                              string const& line ) {
  auto* ttf_font = loaded_fonts[font].ttf_font;
  return render_line_standard_impl( ttf_font, to_SDL( fg ),
                                    line );
}

Texture render_line_shadow( e_font font, Color fg,
                            string const& line ) {
  Color bg        = Color::black();
  bg.a            = 80;
  auto texture_fg = render_line_standard( font, fg, line );
  auto texture_bg = render_line_standard( font, bg, line );
  auto delta      = texture_delta( texture_fg );
  auto result_texture =
      create_texture( delta.w + 1, delta.h + 1 );
  CHECK( copy_texture( texture_bg, result_texture, 1_y, 1_x ) );
  CHECK( copy_texture( texture_fg, result_texture, 0_y, 0_x ) );
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

  //::SDL_SetRenderTarget( g_renderer, result_texture );
  //::SDL_SetRenderDrawColor( g_renderer, 104, 78, 32, 200 );
  //::SDL_RenderClear( g_renderer );

  Y y( 0 );
  for( size_t i = 0; i < textures.size(); ++i ) {
    CHECK( copy_texture( textures[i], result_texture, Y( y ),
                         0_x ) );
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
        render_line_standard_impl( font, fg, num_msg );
    CHECK( copy_texture( texture, nullopt, Y( y ), 0_x ) );
    y += ::TTF_FontLineSkip( font );
    TTF_CloseFont( font );
  }
}

Delta font_rendered_width( e_font             font,
                           std::string const& text ) {
  int w, h;
  ::TTF_SizeText( loaded_fonts[font].ttf_font, text.c_str(), &w,
                  &h );
  return {W( w ), H( h )};
}

void init_fonts() {
  CHECK( !TTF_Init() );
  for( auto& font : loaded_fonts ) {
    auto&       font_desc = font.second;
    int         pt_size   = font_desc.pt_size;
    char const* font_file = font_desc.file_name;
    ASSIGN_CHECK( ttf_font,
                  ::TTF_OpenFont( font_file, pt_size ) );
    // Check style first before setting this.
    ::TTF_SetFontStyle( ttf_font, TTF_STYLE_NORMAL );
    int outline = 0;
    ::TTF_SetFontOutline( ttf_font, outline );
    font_desc.ttf_font = ttf_font;
  }
}

void unload_fonts() {
  for( auto& font : loaded_fonts ) {
    auto& font_desc = font.second;
    ::TTF_CloseFont( font_desc.ttf_font );
  }
  TTF_Quit();
}

void font_test() {
  auto font = e_font::_7_12_serif_16pt;

  H skip( ::TTF_FontLineSkip( loaded_fonts[font].ttf_font ) );

  char const* msg =
      "Ask not what your country can do for you, "
      "but instead ask what you can do for your country!";

  auto render_line = [font]( string const& text ) {
    Color fg = config_palette.orange.sat1.lum11;
    return render_line_shadow( font, fg, text );
  };
  auto texture = render_wrapped_text( skip, msg, render_line,
                                      L( _.size() <= 20 ) );

  CHECK( copy_texture( texture, nullopt, 100_y, 100_x ) );
  // font_size_spectrum( msg, font_file );

  ::SDL_RenderPresent( g_renderer );

  ::SDL_Event event;
  while( true ) {
    ::SDL_PollEvent( &event );
    if( event.type == SDL_KEYDOWN ) break;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    ::SDL_Delay( 10 );
  }
}

} // namespace rn
