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
#include "rnl-util.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// {fmt}
#include "fmt/format.h"

// c++ standard library
#include <filesystem>
#include <fstream>
#include <string_view>

using namespace std;

int main( int argc, char** argv ) {
  if( argc != 4 )
    rnl::error( "usage: rnlc <rnl-file> <out-file>" );

  string_view filename = argv[1];
  if( !filename.ends_with( ".rnl" ) )
    rnl::error( "filename '{}' does not have a .rnl extension.",
                filename );

  optional<string> rnl = util::read_file_as_string( filename );
  if( !rnl.has_value() )
    rnl::error( "failed to open rnl file '{}'.", filename );

  string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) )
    rnl::error( "output file must end with '.hpp'." );

  string_view peg_file = argv[3];
  if( !peg_file.ends_with( ".peg" ) )
    rnl::error( "peg file must end with '.peg'." );

  optional<string> peg = util::read_file_as_string( peg_file );
  if( !peg.has_value() )
    rnl::error( "failed to open peg file '{}'.", peg_file );

  optional<rnl::expr::Rnl> maybe_rnl = rnl::parse( *peg, *rnl );
  if( !maybe_rnl.has_value() )
    rnl::error( "failed to parse RNL file '{}'.", filename );

  optional<string> cpp_code = rnl::generate_code( *maybe_rnl );
  if( !cpp_code.has_value() )
    rnl::error( "failed to generate C++ code for RNL file '{}'.",
                filename );

  string_view stem = filename;
  stem.remove_suffix( 4 );
  ofstream out;
  out.open( string( output_file ) );
  out << "// Auto-Generated file, do not modify! ("
      << filesystem::path( output_file ).stem() << ").\n";

  out << *cpp_code;

  return 0;
}