/****************************************************************
**rdsc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-02.
*
* Description: RDS-to-C++ compiler.
*
*****************************************************************/
// rdsc
#include "code-gen.hpp"
#include "expr.hpp"
#include "parser.hpp"
#include "post-process.hpp"
#include "rds-util.hpp"
#include "validate.hpp"

// base
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/maybe.hpp"

// base-util
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_cat.h"

// c++ standard library
#include <fstream>
#include <string_view>

using namespace std;

using ::base::maybe;

namespace base {
void abort_with_backtrace_here( SourceLoc /*loc*/ ) { abort(); }
} // namespace base

int main( int argc, char** argv ) {
  if( argc != 4 )
    rds::error_msg( "usage: rdsc <rds-file> <out-file>" );

  string_view filename = argv[1];
  if( !filename.ends_with( ".rds" ) )
    rds::error_msg(
        "filename '{}' does not have a .rds extension.",
        filename );

  string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) )
    rds::error_msg( "output file must end with '.hpp'." );

  string_view rds_preamble_filename = argv[3];
  if( !rds_preamble_filename.ends_with( ".lua" ) )
    rds::error_msg( "preamble file must end with '.lua'." );

  maybe<rds::expr::Rds> maybe_rds =
      rds::parse( rds_preamble_filename, filename );
  if( !maybe_rds.has_value() )
    rds::error_msg( "failed to parse RDS file '{}'.", filename );

  vector<string> validation_errors = rds::validate( *maybe_rds );
  if( !validation_errors.empty() ) {
    for( string const& error : validation_errors )
      rds::error_no_exit_msg( "{}", error );
    rds::error_msg( "failed validation." );
  }

  // Performs various transformations.
  rds::post_process( *maybe_rds );

  maybe<string> cpp_code = rds::generate_code( *maybe_rds );
  if( !cpp_code.has_value() )
    rds::error_msg(
        "failed to generate C++ code for RDS file '{}'.",
        filename );

  // Need to do this before we compare with existing_contents.
  string_view stem = filename;
  stem.remove_suffix( 4 );
  cpp_code =
      absl::StrCat( "// Auto-Generated file, do not modify! (",
                    fs::path( output_file ).stem().string(),
                    ").\n", *cpp_code );

  auto existing_contents =
      base::read_text_file_as_string( output_file );
  if( existing_contents.has_value() &&
      ( util::strip( *cpp_code ) ==
        util::strip( static_cast<string const&>(
            *existing_contents ) ) ) ) {
    // Don't rewrite the file if its contents are already iden-
    // tical to what we are about to write, that way we don't af-
    // fect the timestamp on the file, since it won't need to
    // trigger any rebuilding.
    return 0;
  }

  ofstream out;
  out.open( string( output_file ) );
  out << *cpp_code;

  return 0;
}