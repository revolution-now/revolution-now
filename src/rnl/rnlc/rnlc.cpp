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
#include <experimental/source_location>
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

template<typename T>
T safe_any_cast(
    any const&                                v,
    const std::experimental::source_location& location =
        std::experimental::source_location::current() ) {
  if( v.type() != typeid( T const ) ) {
    throw runtime_error( fmt::format(
        "bad safe_any_cast on line: {}.\n", location.line() ) );
  }
  return any_cast<T const>( v );
}

template<typename T>
vector<T> cast_vec(
    peg::SemanticValues const&                sv,
    const std::experimental::source_location& location =
        std::experimental::source_location::current() ) {
  vector<T> res;
  for( any const& v : sv )
    res.push_back( safe_any_cast<T>( v, location ) );
  return res;
}

namespace expr {

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

e_feature from_str( string feature ) {
  if( feature == "formattable" ) return e_feature::formattable;
  if( feature == "serializable" ) return e_feature::serializable;
  throw peg::parse_error(
      fmt::format( "unrecognized feature: \"{}\".", feature )
          .c_str() );
}

struct TemplateParam {
  string param;
};

struct Sumtype {
  string                name;
  vector<TemplateParam> tmpl_params;
  vector<e_feature>     features;
  vector<Alternative>   alternatives;

  string to_string( string_view spaces ) const {
    string res = fmt::format( "{}sumtype: {}\n", spaces, name );
    for( TemplateParam const& tmpl_param : tmpl_params )
      res += fmt::format( "{}  templ: {}\n", spaces,
                          tmpl_param.param );
    for( e_feature feature : features )
      res += fmt::format( "{}  feature: {}\n", spaces,
                          to_str( feature ) );
    for( Alternative const& alt : alternatives )
      res += alt.to_string( string( spaces ) + "  " );
    return res;
  }
};

struct Item {
  string          ns;
  vector<Sumtype> sumtypes;

  string to_string( string_view spaces ) const {
    string res = fmt::format( "{}namespace: {}\n", spaces, ns );
    for( Sumtype const& st : sumtypes )
      res += "\n" + st.to_string( string( spaces ) + "  " );
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
      res += fmt::format( "  {}\n", import );
    res += fmt::format( "\nincludes:\n" );
    for( string const& include : includes )
      res += fmt::format( "  {}\n", include );
    res += fmt::format( "\nitems:\n" );
    for( Item const& item : items )
      res += fmt::format( "{}\n", item.to_string( "  " ) );
    return res;
  }
};

} // namespace expr

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

  auto f_str_tok = []( peg::SemanticValues const& sv ) {
    return string( sv.token() );
  };

  parser["IDENT"]         = f_str_tok;
  parser["CPP_TYPE_NAME"] = f_str_tok;
  parser["MODULE_NAME"]   = f_str_tok;
  parser["INCL_NAME"]     = f_str_tok;
  parser["NS_NAME"]       = f_str_tok;
  parser["TMPL_PARAM"]    = f_str_tok;

  parser["FEATURE"] = []( peg::SemanticValues const& sv ) {
    return expr::from_str( safe_any_cast<string>( sv.token() ) );
  };

  parser["RNL"] = []( peg::SemanticValues const& sv ) {
    return expr::Rnl{
        .imports  = safe_any_cast<vector<string>>( sv[0] ),
        .includes = safe_any_cast<vector<string>>( sv[1] ),
        .items    = safe_any_cast<vector<expr::Item>>( sv[2] ),
    };
  };

  parser["IMPORTS"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<string>( sv );
  };

  parser["INCLUDES"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<string>( sv );
  };

  parser["TEMPLATES"] = []( peg::SemanticValues const& sv ) {
    vector<string> vs = cast_vec<string>( sv );

    vector<expr::TemplateParam> res;
    for( string const& s : vs )
      res.push_back( expr::TemplateParam{ s } );
    return res;
  };

  parser["FEATURES"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<expr::e_feature>( sv );
  };

  parser["ALT_VARS"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<expr::AlternativeMember>( sv );
  };

  parser["ALTERNATIVES"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<expr::Alternative>( sv );
  };

  parser["ITEMS"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<expr::Item>( sv );
  };

  parser["ITEM"] = []( peg::SemanticValues const& sv ) {
    return expr::Item{
        .ns = safe_any_cast<string>( sv[0] ),
        .sumtypes =
            safe_any_cast<vector<expr::Sumtype>>( sv[1] ) };
  };

  parser["CONSTRUCTS"] = []( peg::SemanticValues const& sv ) {
    return cast_vec<expr::Sumtype>( sv );
  };

  parser["ALT_VAR"] = []( peg::SemanticValues const& sv ) {
    return expr::AlternativeMember{
        .type = safe_any_cast<string>( sv[0] ),
        .var  = safe_any_cast<string>( sv[1] ),
    };
  };

  parser["ALTERNATIVE"] = []( peg::SemanticValues const& sv ) {
    return expr::Alternative{
        .name = safe_any_cast<string>( sv[0] ),
        .members =
            safe_any_cast<vector<expr::AlternativeMember>>(
                sv[1] ),
    };
  };

  parser["SUMTYPE"] = [&]( peg::SemanticValues const& sv ) {
    expr::Sumtype res;
    unsigned int  i = 0;
    res.name        = safe_any_cast<string>( sv[i++] );
    if( sv[i].type() == typeid( vector<expr::TemplateParam> ) )
      res.tmpl_params =
          safe_any_cast<vector<expr::TemplateParam>>( sv[i++] );
    if( sv.size() == i )
      throw peg::parse_error( "expected ALTERNATIVES." );
    if( sv[i].type() == typeid( vector<expr::e_feature> ) )
      res.features =
          safe_any_cast<vector<expr::e_feature>>( sv[i++] );
    if( sv.size() == i )
      throw peg::parse_error( "expected ALTERNATIVES." );
    res.alternatives =
        safe_any_cast<vector<expr::Alternative>>( sv[i++] );
    if( sv.size() == i ) return res;
    throw peg::parse_error(
        fmt::format( "exhausted elements of sumtype." )
            .c_str() );
  };

  expr::Rnl parsed_rnl;
  if( !parser.parse( rnl->c_str(), parsed_rnl ) )
    error( "failed to parse rnl file." );

  out << "/*\n\n" << parsed_rnl.to_string() << "\n\n*/";
  return 0;
}