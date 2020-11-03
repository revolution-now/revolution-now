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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include "peglib.h"

int main( int argc, char** argv ) {
  if( argc != 3 ) {
    std::cerr << "usage: rnlc <rnl-file> <out-file>\n";
    return 1;
  }

  std::string_view filename = argv[1];
  if( !filename.ends_with( ".rnl" ) ) {
    std::cerr << "error: filename '" << filename
              << "' does not have .rnl extension.\n";
    return 1;
  }
  std::string_view stem = filename;
  stem.remove_suffix( 4 );

  std::string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) ) {
    std::cerr << "error: output file must end with '.hpp'.\n";
    return 1;
  }

  // std::cerr << "writing output to " << output_file << ".\n";

  std::ofstream out;
  out.open( std::string( output_file ) );
  out << "// empty file "
      << std::filesystem::path( output_file ).stem() << ".\n";

  return 0;
}