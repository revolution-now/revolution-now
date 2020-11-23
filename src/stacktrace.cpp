/****************************************************************
**stacktrace.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-22.
*
* Description: Handles printing of stack traces upon error.
*
*****************************************************************/
#include "stacktrace.hpp"

// base
#include "base/fs.hpp"

// base-util
#include "base-util/macros.hpp"

// backward
#ifdef STACK_TRACE_ON
#  include "backward.hpp"
#endif

// C++ standard library
#include <iostream>

using namespace std;

namespace rn {

namespace {

fs::path const& source_tree_root() {
  static const fs::path p = [] {
    return TO_STRING( RN_SOURCE_TREE_ROOT );
  }();
  return p;
}

bool is_src_file_under_folder( string const& filepath,
                               string const& subfolder ) {
  if( !fs::exists( filepath ) ) return false;
  fs::path p = filepath;
  if( !p.is_absolute() ) p = fs::absolute( p );
  p = fs::canonical( p );

  fs::path dir = source_tree_root() / subfolder;

  return string_view( p.string() ).starts_with( dir.string() );
}

bool is_rn_source( string const& filepath ) {
  return is_src_file_under_folder( filepath, "src" ) ||
         is_src_file_under_folder( filepath, "exe" ) ||
         is_src_file_under_folder( filepath, "test" );
}

bool is_extern_source( string const& filepath ) {
  return is_src_file_under_folder( filepath, "extern" );
}

} // namespace

#ifdef STACK_TRACE_ON
StackTrace::StackTrace() {}

StackTrace::~StackTrace() {}

StackTrace::StackTrace( unique_ptr<backward::StackTrace>&& st_ )
  : st( std::move( st_ ) ) {}

StackTrace::StackTrace( StackTrace&& st_ )
  : st( std::move( st_.st ) ) {}
#endif

ND StackTrace stack_trace_here() {
#ifdef STACK_TRACE_ON
  auto st = make_unique<backward::StackTrace>();
  st->load_here( 32 );
  return StackTrace( std::move( st ) );
#else
  return StackTrace{};
#endif
}

void print_stack_trace( StackTrace const&        st_,
                        StackTraceOptions const& options ) {
#ifdef STACK_TRACE_ON
  backward::StackTrace st = *( st_.st );
  // Skip uninteresting stack frames
  st.skip_n_firsts( options.skip_frames );
  backward::Printer p;
  // Enable this when backward-cpp gets support for user call-
  // backs for filtering printing stack frames. See:
  //   https://github.com/bombela/backward-cpp/issues/196
#  if 0
  switch( options.frames ) {
    case e_stack_trace_frames::all:
      p.set_skip_frame_callback(
          []( string const& filepath ) { return false; } );
      break;
    case e_stack_trace_frames::rn_and_extern_only:
      p.set_skip_frame_callback( []( string const& filepath ) {
        return !is_rn_source( filepath ) &&
               !is_extern_source( filepath );
      } );
      break;
    case e_stack_trace_frames::rn_only:
      p.set_skip_frame_callback( []( string const& filepath ) {
        return !is_rn_source( filepath );
      } );
      break;
  }
#  else
  (void)is_rn_source;
  (void)is_extern_source;
#  endif
  p.print( st, stderr );
#else
  (void)st_;
  (void)options;
  (void)is_rn_source;
  (void)is_extern_source;
  cerr << "(stack trace unavailable: binary built without "
          "support for it)\n";
#endif
}

} // namespace rn
