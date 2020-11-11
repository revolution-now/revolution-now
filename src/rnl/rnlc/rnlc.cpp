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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// {fmt}
#include "fmt/format.h"

// C++ PEG-lib
#include "peglib.h"

using namespace std;

struct AlternativeMember {
  string type;
  string var;

  string to_string( string_view spaces ) const {
    string res =
        fmt::format( "{}( {}, {} )\n", spaces, type, var );
    return res;
  }
};

struct Alternative {
  string                    name;
  vector<AlternativeMember> members;

  string to_string( string_view spaces ) const {
    string res = fmt::format( "{}{}:\n", spaces, name );
    for( AlternativeMember const& mem : members )
      res += mem.to_string( string( spaces ) + "  " );
    return res;
  }
};

enum class e_feature { formattable, serializable };

string to_str( e_feature feature ) {
  switch( feature ) {
    case e_feature::formattable: return "formattable";
    case e_feature::serializable: return "serializable";
  }
}

struct Sumtype {
  string              name;
  vector<string>      tmpl_params;
  vector<e_feature>   features;
  vector<Alternative> alternatives;

  string to_string( string_view spaces ) const {
    string res = fmt::format( "{}sumtype: {}\n", spaces, name );
    for( string const& tmpl_param : tmpl_params )
      res += fmt::format( "{}  templ: {}", tmpl_param );
    for( e_feature feature : features )
      res += fmt::format( "{}  feature: {}", to_str( feature ) );
    for( Alternative const& alt : alternatives )
      res += alt.to_string( string( spaces ) + "  " );
    return res;
  }
};

struct Item {
  string          ns;
  vector<Sumtype> sumtypes;

  string to_string( string_view spaces ) const {
    string res = fmt::format( "{}namespace: {}", spaces, ns );
    for( Sumtype const& st : sumtypes )
      res += st.to_string( string( spaces ) + "  " );
    return res;
  }
};

struct Rnl {
  vector<string> imports;
  vector<string> includes;
  vector<Item>   items;

  string to_string() const {
    string res = fmt::format( "imports:\n" );
    for( string const& import : imports )
      res += fmt::format( "  {}", import );
    res += fmt::format( "includes:\n" );
    for( string const& include : includes )
      res += fmt::format( "  {}", include );
    res += fmt::format( "items:\n" );
    for( Item const& item : items )
      res += fmt::format( "  {}", item.to_string( "  " ) );
    return res;
  }
};

template<typename... Args>
void error( string_view fmt, Args... args ) {
  cerr << "error: ";
  cerr << fmt::format( fmt, args... );
  cerr << "\n";
  exit( 1 );
}

int main( int argc, char** argv ) {
  if( argc != 4 ) error( "usage: rnlc <rnl-file> <out-file>" );

  string_view filename = argv[1];
  if( !filename.ends_with( ".rnl" ) )
    error( "filename '{}' does not have a .rnl extension.",
           filename );

  optional<string> rnl = util::read_file_as_string( filename );
  if( !rnl.has_value() )
    error( "failed to open rnl file '{}'.", filename );

  string_view output_file = argv[2];
  if( !output_file.ends_with( ".hpp" ) )
    error( "output file must end with '.hpp'." );

  string_view peg_file = argv[3];
  if( !peg_file.ends_with( ".peg" ) )
    error( "peg file must end with '.peg'." );

  optional<string> peg = util::read_file_as_string( peg_file );
  if( !peg.has_value() )
    error( "failed to open peg file '{}'.", peg_file );

  peg::parser parser;
  parser.log = []( size_t line, size_t col, string const& msg ) {
    error( "failed to parse peg file:{}:{}: {}", line, col,
           msg );
  };

  if( !parser.load_grammar( peg->c_str() ) )
    error( "failed to parse peg file." );
  parser.enable_packrat_parsing();

  // Change the logger since we are now attempting to parse the
  // rnl file.
  parser.log = []( size_t line, size_t col, string const& msg ) {
    error( "failed to parse rnl file:{}:{}: {}", line, col,
           msg );
  };

  string_view stem = filename;
  stem.remove_suffix( 4 );
  ofstream out;
  out.open( string( output_file ) );
  out << "// Parsed file: "
      << filesystem::path( output_file ).stem() << ".\n";

  parser["IMPORTS"].enter = [&]( char const*, size_t, any& ) {
    out << "\n// === Imports ================================\n";
  };

  parser["INCLUDES"].enter = [&]( char const*, size_t, any& ) {
    out << "\n// === Includes ===============================\n";
  };

  parser["ITEMS"].enter = [&]( char const*, size_t, any& ) {
    out << "\n// === Items ==================================\n";
  };

  parser["MODULE_NAME"] = [&]( peg::SemanticValues const& sv ) {
    out << "// "
        << fmt::format( "importing module \"{}\".\n",
                        sv.token() );
  };

  parser["INCL_NAME"] = [&]( peg::SemanticValues const& sv ) {
    out << "// "
        << fmt::format( "including header \"{}\".\n",
                        sv.token() );
  };

  parser["IDENT"] = [&]( peg::SemanticValues const& sv ) {
    return string( sv.token() );
  };

  parser["SUMTYPE"] = [&]( peg::SemanticValues const& sv ) {
    if( sv.size() != 2 )
      throw peg::parse_error(
          "sumtype must contain name and body." );
    out << "// "
        << fmt::format( "sumtype: {}.\n",
                        any_cast<string>( sv[0] ) );
  };

  parser["FEATURE"] = [&]( peg::SemanticValues const& sv ) {
    out << "// "
        << fmt::format( "  feature: {}.\n", sv.token() );
  };

  if( !parser.parse( rnl->c_str() ) )
    error( "failed to parse rnl file." );

  return 0;
}