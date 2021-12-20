/****************************************************************
**cli-args.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: Command-line argument parsing.
*
*****************************************************************/
#include "cli-args.hpp"

// base
#include "error.hpp"
#include "fmt.hpp"

// Abseil
#include "absl/strings/str_split.h"

// C++ standard library
#include <string_view>

using namespace std;

namespace base {

expect<ProgramArguments> parse_args( span<string const> args ) {
  ProgramArguments res;
  for( string const& arg : args ) {
    if( arg.starts_with( "--" ) ) {
      string stem( arg.begin() + 2, arg.end() ); // remove --
      if( arg.find( "=" ) != string::npos ) {
        vector<string> parts = absl::StrSplit( stem, "=" );
        RETURN_IF( parts.size() != 2, "invalid argument `{}'.",
                   stem );
        string const& key = parts[0];
        string const& val = parts[1];
        RETURN_IF( res.key_val_args.contains( key ),
                   "duplicate key/value argument `{}'.", key );
        res.key_val_args[key] = val;
        continue;
      } else {
        RETURN_IF( res.flag_args.contains( stem ),
                   "duplicate flag argument `{}'.", stem );
        res.flag_args.insert( stem );
        continue;
      }
    }
    // Positional argument.
    res.positional_args.push_back( arg );
  }
  return res;
}

void to_str( ProgramArguments const& pa, string& out,
             base::ADL_t ) {
  if( !pa.key_val_args.empty() ) {
    out += "Key/Val:\n";
    for( auto const& [k, v] : pa.key_val_args )
      out += fmt::format( "  {}: {}\n", k, v );
  }
  if( !pa.flag_args.empty() ) {
    out += "Flags:\n";
    for( auto const& f : pa.flag_args )
      out += fmt::format( "  {}\n", f );
  }
  if( !pa.positional_args.empty() ) {
    out += "Positional:\n";
    for( auto const& p : pa.positional_args )
      out += fmt::format( "  {}\n", p );
  }
}

ProgramArguments parse_args_or_die_with_usage(
    std::span<std::string const> args ) {
  expect<ProgramArguments> parsed = parse_args( args );
  if( parsed ) return std::move( *parsed );
  fmt::print( stderr,
              "failed to parse command-line arguments: {}\n",
              parsed.error() );
  std::exit( 1 );
}

} // namespace base
