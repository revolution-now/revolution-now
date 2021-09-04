/****************************************************************
**errors.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-04.
*
* Description: Error handling code.
*
*****************************************************************/
#include "error.hpp"

// Revolution Now
#include "init.hpp"
#include "logger.hpp"
#include "stacktrace.hpp"

// base
#include "base/stack-trace.hpp"

// SDL
#include "SDL.h"

using namespace std;

namespace rn {
namespace {

void print_SDL_error() {
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
}

} // namespace

void linker_dont_discard_module_error() {}

} // namespace rn

namespace base {

// See description in base/stack-trace.hpp for what this is.
void abort_with_backtrace_here( SourceLoc /*loc*/ ) {
  auto here = ::rn::stack_trace_here();
  rn::print_SDL_error();
  rn::run_all_cleanup_routines();
  print_stack_trace(
      here, ::rn::StackTraceOptions{ .skip_frames = 5 } );
  std::abort();
}

} // namespace base

// If you want a C library to be able to abort with a stack
// trace, just go to the C source file and add the following to
// the top:
//
//   void c_abort_with_backtrace_here();
//
// and then call it, and the linker should find it. The C code
// still needs to be compiled as C++ I believe, since this is not
// extern "C", but it does not require any C++ parameters, so
// that C code should be able to call it. It also doesn't filter
// out any frames and trims less off the top.
void c_abort_with_backtrace_here() {
  auto here = ::rn::stack_trace_here();
  rn::print_SDL_error();
  rn::run_all_cleanup_routines();
  print_stack_trace(
      here, ::rn::StackTraceOptions{
                .skip_frames = 3,
                .frames      = rn::e_stack_trace_frames::all } );
  std::abort();
}
