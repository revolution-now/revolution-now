#include <SDL.h>
#include <SDL_image.h>

#include <iostream>
#include <utility>

SDL_Window* g_pWindow = 0;
SDL_Renderer* g_pRenderer = 0;

void die( char const* msg ) {
  std::cerr << msg << "\n";
  std::terminate();
}

int main( int, char** ) {
  // initialize SDL
  if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    die( "sdl could not initialize" );

  // ::SDL_WINDOW_FULLSCREEN,  ::SDL_WINDOW_OPENGL,
  // ::SDL_WINDOW_HIDDEN,    ::SDL_WINDOW_BORDERLESS,
  // ::SDL_WINDOW_RESIZABLE,   ::SDL_WINDOW_MAXIMIZED,
  // ::SDL_WINDOW_MINIMIZED,   ::SDL_WINDOW_INPUT_GRABBED,
  // ::SDL_WINDOW_ALLOW_HIGHDPI, ::SDL_WINDOW_VULKAN.
  // if succeeded create our window
  g_pWindow = SDL_CreateWindow( "Chapter 1: Setting up SDL",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 480,
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI );
  // if the window creation succeeded create our renderer
  if( g_pWindow != 0 )
    g_pRenderer = SDL_CreateRenderer( g_pWindow, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( !g_pRenderer )
    die( "failed to create renderer" );

  // This function expects Red, Green, Blue and Alpha as color
  // values
  SDL_SetRenderDrawColor( g_pRenderer, 0, 0, 0, 255 );
  // Clear the window to the current drawing color.
  SDL_RenderClear( g_pRenderer );

  SDL_Texture* m_pTexture; // the new SDL_Texture variable
  SDL_Rect m_sourceRectangle; // the first rectangle
  SDL_Rect m_destinationRectangle; // another rectangle

  SDL_Surface* pTempSurface = IMG_Load( "../art/land-tiles.png" );
  if( !pTempSurface )
    die( "failed to load image" );
  m_pTexture = SDL_CreateTextureFromSurface( g_pRenderer, pTempSurface );
  if( !m_pTexture )
    die( "failed to create texture" );
  SDL_FreeSurface( pTempSurface );

  SDL_QueryTexture(m_pTexture, NULL, NULL,
      &m_sourceRectangle.w, &m_sourceRectangle.h);

  m_destinationRectangle.x = m_sourceRectangle.x = 0;
  m_destinationRectangle.y = m_sourceRectangle.y = 0;
  m_destinationRectangle.w = m_sourceRectangle.w*4;
  m_destinationRectangle.h = m_sourceRectangle.h*4;

  if( SDL_RenderCopy(g_pRenderer, m_pTexture, &m_sourceRectangle, &m_destinationRectangle) )
    die( "failed to render texture" );

  // Update the screen with rendering performed.
  SDL_RenderPresent( g_pRenderer );

  // Set a delay before quitting
  SDL_Delay( 3000 );

/*
 *  bool running = true;
 *
 *  while( running ) {
 *    if(SDL_Event event; SDL_PollEvent(&event)) {
 *      switch (event.type) {
 *        case SDL_QUIT:
 *          running = false;
 *          break;
 *        default:
 *          break;
 *      }
 *    }
 *  }
 */

  // clean up SDL
  SDL_DestroyRenderer(g_pRenderer);
  SDL_DestroyWindow(g_pWindow);
  SDL_Quit();
  return 0;
}
