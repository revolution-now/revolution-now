/****************************************************************
* loops.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: 
*
*****************************************************************/
#include "loops.hpp"

#include "movement.hpp"
#include "ownership.hpp"
#include "physics.hpp"
#include "render.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"

namespace rn {

namespace {

} // namespace

e_orders_loop_result loop_orders(
    UnitId id, std::function<void(UnitId)> prioritize ) {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;

  auto& unit = unit_from_id( id );
  auto coords = coords_for_unit( id );

  UnitMoveDesc move_desc;

  viewport().ensure_tile_surroundings_visible( coords );

  long total_frames = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    auto ticks_render_start = ::SDL_GetTicks();
    render_world_viewport( id );
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += (ticks_render_end-ticks_render_start);
    total_frames++;

    e_push_direction zoom_direction = e_push_direction::none;

    ::SDL_Event event;
    while( SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          return e_orders_loop_result::quit;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q:
              { running = false;
              auto ticks_end_loop = ::SDL_GetTicks();
              std::cerr << "average framerate: " << 1000.0*double(total_frames)/(ticks_end_loop-ticks_start_loop) << "\n";
              std::cerr << "average ticks/render: " << double(ticks_render)/total_frames << "\n";
              std::cerr << "render % of frame: " << 100.0*double(ticks_render)/(ticks_end_loop-ticks_start_loop) << "\n";
              return e_orders_loop_result::quit;}
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
            case ::SDLK_t:
              running = false;
              return e_orders_loop_result::wait;
            case ::SDLK_SPACE:
              running = false;
              unit.forfeight_mv_points();
              return e_orders_loop_result::moved;
            case ::SDLK_LEFT:
              move_desc = move_consequences(
                  id, coords.moved( direction::w ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_RIGHT:
              move_desc = move_consequences(
                  id, coords.moved( direction::e ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_DOWN:
              move_desc = move_consequences(
                  id, coords.moved( direction::s ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
            case ::SDLK_UP:
              move_desc = move_consequences(
                  id, coords.moved( direction::n ) );
              if( move_desc.can_move ) {
                // may need to ask user a question here.
                // Assuming that they want to proceed with
                // the move, then:
                running = false;
              }
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom_direction = e_push_direction::negative;
          if( event.wheel.y > 0 )
            zoom_direction = e_push_direction::positive;
          break;
        default:
          break;
      }
    }

    auto const* state = ::SDL_GetKeyboardState( NULL );

    viewport().advance(
      // x motion
        state[::SDL_SCANCODE_A] ? e_push_direction::negative
      : state[::SDL_SCANCODE_D] ? e_push_direction::positive
      : e_push_direction::none,
      // y motion
        state[::SDL_SCANCODE_W] ? e_push_direction::negative
      : state[::SDL_SCANCODE_S] ? e_push_direction::positive
      : e_push_direction::none,
      // zoom motion
        zoom_direction );

    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  // Check if the unit is physicall moving; usually at this point
  // it will be unless it is e.g. a ship offloading units.
  if( coords_for_unit( id ) != move_desc.coords )
    loop_mv_unit( id, move_desc.coords );
  move_unit( id, move_desc );
  for( auto id : move_desc.to_prioritize )
    prioritize( id );
  if( std::holds_alternative<e_unit_mv_good>( move_desc.desc ) )
    if( std::get<e_unit_mv_good>( move_desc.desc ) ==
        e_unit_mv_good::land_fall )
      return e_orders_loop_result::offboard;
  return e_orders_loop_result::moved;
}

e_eot_loop_result loop_eot() {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  bool running = true;
  e_eot_loop_result result = e_eot_loop_result::none;

  long total_frames = 0;
  auto ticks_start_loop = ::SDL_GetTicks();

  long ticks_render = 0;

  // we can also use the SDL_GetKeyboardState to get an
  // array that tells us if a key is down or not instead
  // of keeping track of it ourselves.
  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    total_frames++;

    auto ticks_render_start = ::SDL_GetTicks();
    render_world_viewport();
    auto ticks_render_end = ::SDL_GetTicks();
    ticks_render += (ticks_render_end-ticks_render_start);

    e_push_direction zoom_direction = e_push_direction::none;

    ::SDL_Event event;
    while( SDL_PollEvent( &event ) ) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          result = e_eot_loop_result::quit;
          break;
        case ::SDL_WINDOWEVENT:
          switch( event.window.event) {
            case ::SDL_WINDOWEVENT_RESIZED:
            case ::SDL_WINDOWEVENT_RESTORED:
            case ::SDL_WINDOWEVENT_EXPOSED:
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch( event.key.keysym.sym ) {
            case ::SDLK_q:
              { running = false;
              result = e_eot_loop_result::quit;
              auto ticks_end_loop = ::SDL_GetTicks();
              std::cerr << "average framerate: " << 1000.0*double(total_frames)/(ticks_end_loop-ticks_start_loop) << "\n";
              std::cerr << "average ticks/render: " << double(ticks_render)/total_frames << "\n";
              std::cerr << "render % of frame: " << 100.0*double(ticks_render)/(ticks_end_loop-ticks_start_loop) << "\n";
              break;}
            case ::SDLK_F11:
              toggle_fullscreen();
              break;
          }
          break;
        case ::SDL_MOUSEWHEEL:
          if( event.wheel.y < 0 )
            zoom_direction = e_push_direction::negative;
          if( event.wheel.y > 0 )
            zoom_direction = e_push_direction::positive;
          break;
        default:
          break;
      }
    }

    auto const* state = ::SDL_GetKeyboardState( NULL );

    viewport().advance(
      // x motion
        state[::SDL_SCANCODE_A] ? e_push_direction::negative
      : state[::SDL_SCANCODE_D] ? e_push_direction::positive
      : e_push_direction::none,
      // y motion
        state[::SDL_SCANCODE_W] ? e_push_direction::negative
      : state[::SDL_SCANCODE_S] ? e_push_direction::positive
      : e_push_direction::none,
      // zoom motion
        zoom_direction );

    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
  return result;
}

void loop_mv_unit( UnitId id, Coord target ) {
  int frame_rate = 60;
  double frame_length_millis = 1000.0/frame_rate;

  DissipativeVelocity percent_vel(
    /*min_velocity=*/0,
    /*max_velocity=*/.1,
    /*initial_velocity=*/.1,
    /*mag_acceleration=*/1, // not relevant
    /*mag_drag_acceleration=*/.004
  );

  double percent = 0;
  bool running = true;

  while( running ) {
    auto ticks_start = ::SDL_GetTicks();

    render_world_viewport_mv_unit( id, target, percent );

    percent_vel.advance( e_push_direction::none );
    percent += percent_vel;
    if( percent > 1.0 )
      running = false;

    auto ticks_end = ::SDL_GetTicks();
    auto delta = ticks_end-ticks_start;
    if( delta < frame_length_millis )
      ::SDL_Delay( frame_length_millis - delta );
  }
}

} // namespace rn
