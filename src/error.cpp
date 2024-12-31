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
#include "logger.hpp"
#include "stacktrace.hpp"

// sdl
#include "include-sdl-base.hpp"

// base
#include "base/stack-trace.hpp"

using namespace std;

namespace rn {
namespace {

base::maybe<base::function_ref<void() const>> g_cleanup_fn;

void print_SDL_error() {
  string sdl_error = SDL_GetError();
  if( !sdl_error.empty() )
    lg.error( "SDL error (may be a false positive): {}",
              sdl_error );
}

} // namespace

void register_cleanup_callback_on_abort(
    base::maybe<base::function_ref<void() const>> fn ) {
  g_cleanup_fn = fn;
}

void linker_dont_discard_module_error();
void linker_dont_discard_module_error() {}

} // namespace rn

namespace base {

// See description in base/stack-trace.hpp for what this is.
//
// FIXME: we should probably be using the source location passed
// into this function in case a stack trace is not available.
void abort_with_backtrace_here( source_location /*loc*/ ) {
  auto here = ::rn::stack_trace_here();
  rn::print_SDL_error();
  if( rn::g_cleanup_fn.has_value() ) ( *rn::g_cleanup_fn )();
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
[[noreturn]] void c_abort_with_backtrace_here();
[[noreturn]] void c_abort_with_backtrace_here() {
  auto here = ::rn::stack_trace_here();
  rn::print_SDL_error();
  if( rn::g_cleanup_fn.has_value() ) ( *rn::g_cleanup_fn )();
  print_stack_trace(
      here, ::rn::StackTraceOptions{
              .skip_frames = 3,
              .frames      = rn::e_stack_trace_frames::all } );
  std::abort();
}
