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

#include <SDL_ttf.h>

#include <string>

namespace rn {

namespace {


  
} // namespace

int draw_msg( char const* msg, char const* font_file, int y, int ptsize, bool blended ) {
  ASSIGN_ASSERT( font, ::TTF_OpenFont( font_file, ptsize ) );
  int w, h;
  ::TTF_SizeText( font, msg, &w, &h );

  //LOG( "msg size (h,w): (" << h << "," << w << ")" );
  //LOG( "font recommended pixel height: " << ::TTF_FontLineSkip( font ) );

  auto fn = blended ? ::TTF_RenderText_Blended : ::TTF_RenderText_Solid;
  std::string num_msg = std::to_string( ptsize ) + ": " + msg;

  {
    ::SDL_Color fg{ 0, 0, 0, 255 };
    ASSIGN_ASSERT( surface, fn( font, num_msg.c_str(), fg ) );
    ASSIGN_ASSERT( texture, ::SDL_CreateTextureFromSurface( g_renderer, surface ) );
    ::SDL_FreeSurface( surface );
    ::SDL_Rect rect{ 1, y+1, w, h };
    ::SDL_RenderCopy( g_renderer, texture, NULL, &rect );
    ::SDL_DestroyTexture( texture );
  }

  {
    ::SDL_Color fg{ 255, 255, 255, 255 };
    ASSIGN_ASSERT( surface, fn( font, num_msg.c_str(), fg ) );
    ASSIGN_ASSERT( texture, ::SDL_CreateTextureFromSurface( g_renderer, surface ) );
    ::SDL_FreeSurface( surface );
    ::SDL_Rect rect{ 0, y, w, h };
    ::SDL_RenderCopy( g_renderer, texture, NULL, &rect );
    ::SDL_DestroyTexture( texture );
  }

  auto skip = ::TTF_FontLineSkip( font );

  TTF_CloseFont( font );
  return skip;
}

void font_test() {
  ASSERT( !TTF_Init() );
  char const* font_file = "../fonts/7-12-serif/712_serif.ttf";
  //ASSIGN_ASSERT( franklin, ::TTF_OpenFont( "../fonts/advanced_pixel-7.ttf", ptsize ) );
  // Check style first before setting this.
  //::TTF_SetFontStyle( franklin, TTF_STYLE_NORMAL );
  //LOG( "font outline size: " << ::TTF_GetFontOutline( franklin ) );
  //int outline = 0;
  //::TTF_SetFontOutline( franklin, outline );
  //LOG( "font pixel height: " << ::TTF_FontHeight( franklin ) );
  //LOG( "font ascent: " << ::TTF_FontAscent( franklin ) );
  //LOG( "font descent: " << ::TTF_FontDescent( franklin ) );
  //LOG( "font num faces: " << ::TTF_FontFaces( franklin ) );
  // may return NULL
  //LOG( "font name: " << ::TTF_FontFaceFamilyName( franklin ) );
  // may return NULL
  //LOG( "font style: " << ::TTF_FontFaceStyleName( franklin ) );

  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );
  ::SDL_RenderPresent( g_renderer );

  char const* msg = "Ask not what your country can do for you, but instead ask what you can do for your country!";

  int skip = 30;
  // smallest legible:
  // small:
  // small wide:
  // regular:
  auto sizes = {3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
  for( auto ptsize : sizes ) {
    skip += draw_msg( msg, font_file, skip, ptsize, false );
    skip += draw_msg( msg, font_file, skip, ptsize, true );
  }

  ::SDL_RenderPresent( g_renderer );
  ::SDL_Event event;
  while( true ) {
    ::SDL_PollEvent( &event );
    if( event.type == SDL_KEYDOWN )
      break;
    ::SDL_Delay( 200 );
  }

  TTF_Quit();
}

} // namespace rn

