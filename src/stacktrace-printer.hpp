/****************************************************************
**stacktrace-printer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-23.
*
* Description: Class for printing stack traces.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "backward.hpp"

// C++ standard library
#include <functional>

namespace rn::stacktrace {

namespace bw = ::backward;

// This class has been adapted from the sample one provided in
// the backward-cpp package, with a few changes, namely:
//
//   1. Allow filtering of stack traces via a user callback.
//   2. Tweaking of colors.
//
struct StackTracePrinter {
  using InclFrameCallback =
      std::function<bool( std::string const & )>;

  bool                snippet;
  bw::ColorMode::type color_mode;
  bool                address;
  bool                object;
  int                 inliner_context_size;
  int                 trace_context_size;
  InclFrameCallback   include_frame_callback;

  StackTracePrinter()
    : snippet( true ),
      color_mode( bw::ColorMode::automatic ),
      address( false ),
      object( false ),
      inliner_context_size( 5 ),
      trace_context_size( 7 ),
      include_frame_callback{} {}

  template<typename ST>
  FILE *print( ST &st, FILE *fp = stderr ) {
    bw::cfile_streambuf obuf( fp );
    std::ostream        os( &obuf );
    print_stacktrace( st, os );
    return fp;
  }

  template<typename ST>
  std::ostream &print( ST &st, std::ostream &os ) {
    print_stacktrace( st, os );
    return os;
  }

  template<typename IT>
  FILE *print( IT begin, IT end, FILE *fp = stderr,
               size_t thread_id = 0 ) {
    bw::cfile_streambuf obuf( fp );
    std::ostream        os( &obuf );
    print_stacktrace( begin, end, os, thread_id );
    return fp;
  }

  template<typename IT>
  std::ostream &print( IT begin, IT end, std::ostream &os,
                       size_t thread_id = 0 ) {
    print_stacktrace( begin, end, os, thread_id );
    return os;
  }

  bw::TraceResolver const &resolver() const { return _resolver; }

private:
  bw::TraceResolver  _resolver;
  bw::SnippetFactory _snippets;

  template<typename ST>
  void print_stacktrace( ST &st, std::ostream &os ) {
    print_header( os, st.thread_id() );
    _resolver.load_stacktrace( st );
    for( size_t trace_idx = st.size(); trace_idx > 0;
         --trace_idx ) {
      print_trace( os, _resolver.resolve( st[trace_idx - 1] ) );
    }
  }

  template<typename IT>
  void print_stacktrace( IT begin, IT end, std::ostream &os,
                         size_t thread_id ) {
    print_header( os, thread_id );
    for( ; begin != end; ++begin ) { print_trace( os, *begin ); }
  }

  void print_header( std::ostream &os, size_t thread_id ) {
    os << "Stack trace (most recent call last)";
    if( thread_id ) { os << " in thread " << thread_id; }
    os << ":\n";
  }

  void print_trace( std::ostream &           os,
                    const bw::ResolvedTrace &trace ) {
    if( include_frame_callback )
      if( trace.source.filename.size() )
        if( !include_frame_callback( trace.source.filename ) )
          return;

    os << "#" << std::left << std::setw( 2 ) << trace.idx
       << std::right;
    bool already_indented = true;

    if( !trace.source.filename.size() || object ) {
      os << "   Object \"" << trace.object_filename << "\", at "
         << trace.addr << ", in " << trace.object_function
         << "\n";
      already_indented = false;
    }

    for( size_t inliner_idx = trace.inliners.size();
         inliner_idx > 0; --inliner_idx ) {
      if( !already_indented ) { os << "   "; }
      const bw::ResolvedTrace::SourceLoc &inliner_loc =
          trace.inliners[inliner_idx - 1];
      print_source_loc( os, " | ", inliner_loc );
      if( snippet ) {
        print_snippet( os, "    | ", inliner_loc,
                       /*purple=*/"\033[35m",
                       inliner_context_size );
      }
      already_indented = false;
    }

    if( trace.source.filename.size() ) {
      if( !already_indented ) { os << "   "; }
      print_source_loc( os, "   ", trace.source, trace.addr );
      if( snippet ) {
        print_snippet( os, "      ", trace.source,
                       /*bright yellow=*/"\033[33;1m",
                       trace_context_size );
      }
    }
  }

  void print_snippet(
      std::ostream &os, const char *indent,
      const bw::ResolvedTrace::SourceLoc &source_loc,
      std::string_view color, int context_size ) {
    using namespace std;
    typedef bw::SnippetFactory::lines_t lines_t;

    lines_t lines = _snippets.get_snippet(
        source_loc.filename, source_loc.line,
        static_cast<unsigned>( context_size ) );

    for( lines_t::const_iterator it = lines.begin();
         it != lines.end(); ++it ) {
      if( it->first == source_loc.line ) {
        os << color << indent << ">";
      } else {
        os << indent << " ";
      }
      os << std::setw( 4 ) << it->first << ": " << it->second
         << "\n";
      if( it->first == source_loc.line ) {
        os << "\033[39;0m"; // reset
      }
    }
  }

  void print_source_loc(
      std::ostream &os, const char *indent,
      const bw::ResolvedTrace::SourceLoc &source_loc,
      void *                              addr = nullptr ) {
    os << indent << "Source \"" << source_loc.filename
       << "\", line " << source_loc.line << ", in "
       << source_loc.function;

    if( address && addr != nullptr ) {
      os << " [" << addr << "]";
    }
    os << "\n";
  }
};

} // namespace rn::stacktrace
