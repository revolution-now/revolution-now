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

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// C++ PEG-lib
#include "peglib.h"

std::string remove_comments( std::string const& rnl ) {
  auto lines = util::split_strip_any( rnl, "\n\r" );
  std::vector<std::string> res;
  for( auto line : lines ) {
    auto n = line.find_first_of( '#' );
    if( n != line.npos )
      line = std::string_view( line.begin(), n );
    line = util::strip( line );
    if( line.empty() ) continue;
    res.push_back( std::string( line ) );
  }
  return util::join( res, "\n" );
}

int main( int argc, char** argv ) {
  if( argc != 4 ) {
    std::cerr << "usage: rnlc <rnl-file> <out-file>\n";
    return 1;
  }

  std::string_view filename = argv[1];
  if( !filename.ends_with( ".rnl" ) ) {
    std::cerr << "error: filename '" << filename
              << "' does not have .rnl extension.\n";
    return 1;
  }

  std::optional<std::string> rnl =
      util::read_file_as_string( filename );
  if( !rnl.has_value() ) {
    std::cerr << "error: failed to open rnl file: " << filename
              << ".\n";
    return 1;
  }

  std::string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) ) {
    std::cerr << "error: output file must end with '.hpp'.\n";
    return 1;
  }

  std::string_view peg_file = argv[3];
  if( !peg_file.ends_with( ".peg" ) ) {
    std::cerr << "error: peg file must end with '.peg'.\n";
    return 1;
  }

  std::optional<std::string> peg =
      util::read_file_as_string( peg_file );
  if( !peg.has_value() ) {
    std::cerr << "error: failed to open peg file: " << peg_file
              << ".\n";
    return 1;
  }

  peg::parser parser;
  parser.log = []( size_t line, size_t col,
                   std::string const& msg ) {
    std::cerr << "error parsing peg file:" << line << ":" << col
              << ": " << msg << "\n";
    return 1;
  };

  if( !parser.load_grammar( peg->c_str() ) ) {
    std::cerr << "error: failed to parse peg file.\n";
    return 1;
  }
  parser.enable_packrat_parsing();

  // Change the logger since we are now attempting to parse the
  // rnl file.
  parser.log = []( size_t line, size_t col,
                   std::string const& msg ) {
    std::cerr << "error parsing rnl file:" << line << ":" << col
              << ": " << msg << "\n";
    return 1;
  };

  // Remove comments.
  std::string rnl_no_comments = remove_comments( *rnl );
  if( !parser.parse( rnl_no_comments.c_str() ) ) {
    std::cerr << "error: failed to parse rnl file.\n";
    return 1;
  }

  std::string_view stem = filename;
  stem.remove_suffix( 4 );
  std::ofstream out;
  out.open( std::string( output_file ) );
  out << "// output file: "
      << std::filesystem::path( output_file ).stem() << ".\n";

  for( auto s : util::split( rnl_no_comments, '\n' ) )
    out << "// " << s << "\n";

  return 0;
}