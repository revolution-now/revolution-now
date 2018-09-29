/****************************************************************
* fonts.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-22.
*
* Description: Code for handling all things text.
*
*****************************************************************/
#include "fonts.hpp"

#include "globals.hpp"
#include "macros.hpp"
#include "base-util.hpp"
#include "sdl-util.hpp"

#include "util/algo.hpp"
#include "util/string-util.hpp"

#include <SDL_ttf.h>

#include <algorithm>
#include <string>
#include <string_view>

using namespace std;

namespace rn {

namespace {



} // namespace

::SDL_Texture* render_line_standard(
    ::TTF_Font* font, ::SDL_Color fg, string const& line ) {
  ASSIGN_CHECK( surface, ::TTF_RenderText_Solid( font, line.c_str(), fg ) );
  ASSIGN_CHECK( texture, ::SDL_CreateTextureFromSurface( g_renderer, surface ) );
  ::SDL_FreeSurface( surface );
  return texture;
}

::SDL_Texture* render_line_shadow(
    ::TTF_Font* font, string const& line ) {
  ::SDL_Color fg{ 255, 255, 255, 255 };
  ::SDL_Color bg{ 128, 128, 128, 255 };
  auto* texture_fg = render_line_standard( font, fg, line.c_str() );
  auto* texture_bg = render_line_standard( font, bg, line.c_str() );
  auto rect = texture_rect( texture_fg );
  auto result_texture = create_texture( rect.w+1, rect.h+1 );
  CHECK( copy_texture( texture_bg, result_texture, Y(1), X(1) ) );
  CHECK( copy_texture( texture_fg, result_texture, Y(0), X(0) ) );
  ::SDL_DestroyTexture( texture_bg );
  ::SDL_DestroyTexture( texture_fg );
  return result_texture;
}

using RenderLineFn = std::function<::SDL_Texture*( string const& )>;

::SDL_Texture* render_lines(
    H min_skip, vector<string> const& lines, RenderLineFn render_line ) {
  auto textures = util::map( render_line, lines );
  auto rects = util::map( texture_rect, textures );
  H res_height( 0 );
  W res_width( 0 );
  for( auto rect : rects ) {
    res_height += max( min_skip, rect.h );
    res_width = max( res_width, rect.w );
  }

  auto result_texture = create_texture( res_width, res_height );

  Y y( 0 );
  for( size_t i = 0; i < textures.size(); ++i ) {
    CHECK( copy_texture( textures[i], result_texture, Y(y), X(0) ) );
    y += max( min_skip, rects[i].h );
  }

  util::map_( ::SDL_DestroyTexture, textures );
  return result_texture;
}

::SDL_Texture* render_wrapped_text( H min_skip,
                                    string_view text,
                                    RenderLineFn render_line,
                                    util::IsStrOkFunc width_checker ) {
  auto wrapped = util::wrap_text_fn( text, width_checker );
  return render_lines( min_skip, wrapped, render_line );
}

::SDL_Texture* render_wrapped_text_by_pixels(
    ::TTF_Font* font,
    int max_pixel_width,
    H min_skip,
    string_view text,
    RenderLineFn render_line ) {
  auto width_checker = [font, max_pixel_width]( string_view text ) {
    int w, h;
    ::TTF_SizeText( font, string( text ).c_str(), &w, &h );
    return w <= max_pixel_width;
  };
  auto wrapped = util::wrap_text_fn( text, width_checker );
  return render_lines( min_skip, wrapped, render_line );
}

void font_size_spectrum( char const* msg, char const* font_file ) {
  int y = 30;
  auto sizes = {3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
  for( auto ptsize : sizes ) {
    ASSIGN_CHECK( font, ::TTF_OpenFont( font_file, ptsize ) );
    std::string num_msg = std::to_string( ptsize ) + ": " + msg;
    ::SDL_Color fg{ 255, 255, 255, 255 };
    auto* texture = render_line_standard( font, fg, num_msg );
    CHECK( copy_texture( texture, NULL, Y(y), X(0) ) );
    ::SDL_DestroyTexture( texture );
    TTF_CloseFont( font );

    y += ::TTF_FontLineSkip( font );
  }
  ::SDL_RenderPresent( g_renderer );
}

void font_test() {
  CHECK( !TTF_Init() );
  constexpr int ptsize = 16;
  char const* font_file = "../fonts/7-12-serif/712_serif.ttf";
  ASSIGN_CHECK( font, ::TTF_OpenFont( font_file, ptsize ) );
  // Check style first before setting this.
  ::TTF_SetFontStyle( font, TTF_STYLE_NORMAL );
  int outline = 0;
  ::TTF_SetFontOutline( font, outline );

  H skip(::TTF_FontLineSkip( font ));

  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );
  ::SDL_RenderPresent( g_renderer );

  char const* msg = "Ask not what your country can do for you, "
                    "but instead ask what you can do for your country!";

  auto render_line = [font]( string const& text ) {
      return render_line_shadow( font, text );
  };
  auto* texture = render_wrapped_text_by_pixels(
      font, 200, skip, msg, render_line );

  CHECK( copy_texture( texture, NULL, Y(100), X(100) ) );

  ::SDL_RenderPresent( g_renderer );
  ::TTF_CloseFont( font );

  ::SDL_Event event;
  while( true ) {
    ::SDL_PollEvent( &event );
    if( event.type == SDL_KEYDOWN )
      break;
    ::SDL_Delay( 10 );
  }

  TTF_Quit();
}

} // namespace rn
