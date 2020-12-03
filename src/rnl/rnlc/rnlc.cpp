/****************************************************************
**rnlc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-02.
*
* Description: RNL-to-C++ compiler.
*
*****************************************************************/
// rnlc
#include "code-gen.hpp"
#include "expr.hpp"
#include "parser.hpp"
#include "post-process.hpp"
#include "rnl-util.hpp"
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

int main( int argc, char** argv ) {
  if( argc != 4 )
    rnl::error_msg( "usage: rnlc <rnl-file> <out-file>" );

  string_view filename = argv[1];
  if( !filename.ends_with( ".rnl" ) )
    rnl::error_msg(
        "filename '{}' does not have a .rnl extension.",
        filename );

  auto rnl = base::read_text_file_as_string( filename );
  if( !rnl.has_value() )
    rnl::error_msg( "failed to open rnl file '{}'.", filename );

  string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) )
    rnl::error_msg( "output file must end with '.hpp'." );

  string_view peg_file = argv[3];
  if( !peg_file.ends_with( ".peg" ) )
    rnl::error_msg( "peg file must end with '.peg'." );

  auto peg = base::read_text_file_as_string( peg_file );
  if( !peg.has_value() )
    rnl::error_msg( "failed to open peg file '{}'.", peg_file );

  maybe<rnl::expr::Rnl> maybe_rnl =
      rnl::parse( peg_file, filename, *peg, *rnl );
  if( !maybe_rnl.has_value() )
    rnl::error_msg( "failed to parse RNL file '{}'.", filename );

  vector<string> validation_errors = rnl::validate( *maybe_rnl );
  if( !validation_errors.empty() ) {
    for( string const& error : validation_errors )
      rnl::error_no_exit_msg( "{}", error );
    rnl::error_msg( "failed validation." );
  }

  // Performs various transformations.
  rnl::post_process( *maybe_rnl );

  maybe<string> cpp_code = rnl::generate_code( *maybe_rnl );
  if( !cpp_code.has_value() )
    rnl::error_msg(
        "failed to generate C++ code for RNL file '{}'.",
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