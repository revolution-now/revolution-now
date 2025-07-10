/****************************************************************
**stack-trace.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-05.
*
* Description: Utilities for handling stack traces.
*
*****************************************************************/
#include "stack-trace.hpp"

// base
#include "ansi.hpp"
#include "cc-specific.hpp"
#include "fs.hpp"
#include "io.hpp"
#include "logger.hpp"

// C++ standard library
#include <iostream>
#include <regex>
#include <source_location>

using namespace std;

namespace base {

namespace {

// The first file_name is the full path, the second just gets the
// literal file name.
string const kThisFile =
    fs::path( source_location::current().file_name() )
        .filename()
        .string();

maybe<function<void()>> g_cleanup_fn;

#ifdef STACK_TRACE_ON
maybe<string> extract_fn_name( string const& in ) {
  // Attempts to extract `ccc` from:
  //   ::aaa::bbb::ccc<...>( ... )
  static regex const rgx(
      R"(([a-zA-Z_][a-zA-Z0-9_]*)(<.*>)?\()" );
  smatch matches;
  if( regex_search( in, matches, rgx ) )
    for( size_t i = 0; i < matches.size(); ++i )
      if( i == 1 ) //
        return matches[i];
  return nothing;
}

int constexpr kSurroundLines      = 3;
int constexpr kMinLineNumberWidth = 3;
int constexpr kFrameColumnWidth   = 4;
auto constexpr kFileColor         = base::ansi::cyan;
auto constexpr kCenterColor       = base::ansi::yellow;
auto constexpr kLineColor         = base::ansi::green;
auto constexpr kFnColor           = base::ansi::magenta;
auto constexpr kErrColor          = base::ansi::red;

void print_stack_trace_impl( StackTrace const& backtrace,
                             StackTraceOptions const& options ) {
  print_bar( '-' );
  vector<stacktrace_entry> entries( backtrace.begin(),
                                    backtrace.end() );
  reverse( entries.begin(), entries.end() );
  if( options.skip_stacktrace_module )
    erase_if( entries, []( auto const& s ) {
      return s.source_file().ends_with( kThisFile );
    } );
  for( int i = 0; i < options.skip_frames; ++i )
    if( !entries.empty() ) //
      entries.pop_back();
  int const num_entries = entries.size();
  fmt::println( "Stack trace (most recent call last)" );
  map<string, vector<string>> lines_cache;
  for( int i = 0; stacktrace_entry const& entry : entries ) {
    ++i;
    int const frame_idx = num_entries - i;
    if( !should_include_filepath_in_stacktrace(
            entry.source_file(), options.frames ) )
      continue;
    // For clang/libstdc++, most of the time the names are not
    // mangled, but they are in a few cases for some reason.
    string const desc_demangled =
        demangle( entry.description().c_str() );
    auto const fn_name = extract_fn_name( desc_demangled );
    if( entry.source_file().empty() ) {
      if( entry.description().empty() ) continue;
      fmt::println( "#{:<{}} {}{}{}", frame_idx,
                    kFrameColumnWidth, kFnColor, desc_demangled,
                    base::ansi::reset );
      continue;
    }
    string const path = [&] {
      string res = fs::path( entry.source_file() )
                       .lexically_relative( fs::current_path() );
      if( res.find( ".." ) != string::npos )
        res = entry.source_file();
      return res;
    }();
    fmt::println( "#{:<{}} {}{}{}, line {}{}{}, in {}{}{}",
                  frame_idx, kFrameColumnWidth, kFileColor, path,
                  base::ansi::reset, kLineColor,
                  entry.source_line(), base::ansi::reset,
                  kFnColor, fn_name.value_or( desc_demangled ),
                  base::ansi::reset );
    if( !lines_cache.contains( entry.source_file() ) ) {
      vector<string> lines;
      if( !base::read_file_lines( entry.source_file(), lines ) )
        continue;
      lines_cache[entry.source_file()] = std::move( lines );
    }
    auto const& lines = lines_cache[entry.source_file()];
    for( int one_based = entry.source_line() - kSurroundLines;
         one_based <=
         int( entry.source_line() + kSurroundLines );
         ++one_based ) {
      int const zero_based = one_based - 1;
      if( zero_based < 0 || zero_based >= ssize( lines ) )
        continue;
      bool const center =
          one_based == int( entry.source_line() );
      int const line_num_width = std::max(
          ssize( to_string( one_based + kSurroundLines ) ),
          long( kMinLineNumberWidth ) );
      string const num_str =
          format( "      {}{:>{}}", center ? "> " : "  ",
                  one_based, line_num_width );
      string_view const start_style =
          center ? base::ansi::bold : "";
      string_view const start_color =
          center ? ( frame_idx == 0 ? kErrColor : kCenterColor )
                 : "";
      string_view const end_color =
          center ? base::ansi::reset : "";
      fmt::println( "{}{}{:>11}:  {}{}", start_style,
                    start_color, num_str, lines[zero_based],
                    end_color );
    }
  }
}
#else
void print_stack_trace_impl( StackTrace const&,
                             StackTraceOptions const& ) {
  cerr << "(stack trace unavailable: binary built without "
          "support for it)\n";
}
#endif

bool is_rn_source( string const& filepath ) {
  if( !fs::exists( filepath ) ) return false;
  auto const p = fs::canonical( fs::path( filepath ) ).string();
  return p.find( "/src/" ) != string::npos ||
         p.find( "/exe/" ) != string::npos ||
         p.find( "/test/" ) != string::npos;
}

bool is_extern_source( string const& filepath ) {
  if( !fs::exists( filepath ) ) return false;
  auto const p = fs::canonical( fs::path( filepath ) ).string();
  return p.find( "/extern/" ) != string::npos;
}

} // namespace

StackTrace stack_trace_here() {
#ifdef STACK_TRACE_ON
  return stacktrace::current();
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

void print_stack_trace( StackTrace const& st,
                        StackTraceOptions const& options ) {
  print_stack_trace_impl( st, options );
}

void register_cleanup_callback_on_abort(
    maybe<function<void()>> const fn ) {
  g_cleanup_fn = fn;
}

void abort_with_backtrace_here( int skip_frames ) {
  auto const here = stack_trace_here();
  if( g_cleanup_fn.has_value() ) ( *g_cleanup_fn )();
  print_stack_trace(
      here, StackTraceOptions{ .skip_frames = skip_frames } );
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
  auto here = base::stack_trace_here();
  if( base::g_cleanup_fn.has_value() ) ( *base::g_cleanup_fn )();
  print_stack_trace(
      here, base::StackTraceOptions{
              .skip_stacktrace_module = false,
              .skip_frames            = 0,
              .frames = base::e_stack_trace_frames::all } );
  std::abort();
}
