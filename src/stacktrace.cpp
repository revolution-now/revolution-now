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

// Revolution Now
#include "aliases.hpp"
#include "maybe.hpp"

// base
#include "base/build-properties.hpp"
#include "base/fs.hpp"

// backward
#ifdef STACK_TRACE_ON
#  include "backward.hpp"
#  include "stacktrace-printer.hpp"
#endif

// C++ standard library
#include <iostream>

using namespace std;

namespace rn {

namespace {

maybe<fs::path> find_file( fs::path const& file ) {
  if( file.is_absolute() )
    return fs::exists( file ) ? file : maybe<fs::path>{};
  if( auto p = base::source_tree_root() / file; fs::exists( p ) )
    return p;
  if( auto p = base::build_output_root() / file;
      fs::exists( p ) )
    return p;
  return nothing;
}

bool is_src_file_under_folder( string const& filepath,
                               string const& subfolder ) {
  auto maybe_path = find_file( filepath );
  if( !maybe_path.has_value() ) return false;
  fs::path& p = *maybe_path;
  if( !p.is_absolute() ) p = fs::absolute( p );
  p = fs::canonical( p );

  fs::path dir = base::source_tree_root() / subfolder;

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

bool should_include_filepath_in_stacktrace(
    std::string_view filepath, e_stack_trace_frames frames ) {
  fs::path p = filepath;
  switch( frames ) {
    case e_stack_trace_frames::all: //
      return true;
    case e_stack_trace_frames::rn_and_extern_only:
      return is_rn_source( p ) || is_extern_source( p );
    case e_stack_trace_frames::rn_only: //
      return is_rn_source( p );
  }
}

void print_stack_trace( StackTrace const&        st_,
                        StackTraceOptions const& options ) {
#ifdef STACK_TRACE_ON
  backward::StackTrace st = *( st_.st );
  // Skip uninteresting stack frames
  st.skip_n_firsts( options.skip_frames );
  rn::stacktrace::StackTracePrinter p;
  p.include_frame_callback =
      [frames = options.frames]( string const& filepath ) {
        return should_include_filepath_in_stacktrace( filepath,
                                                      frames );
      };
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
