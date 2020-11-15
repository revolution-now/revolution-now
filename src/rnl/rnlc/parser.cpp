/****************************************************************
**parser.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: RNL Parser.
*
*****************************************************************/
#include "parser.hpp"

// rnlc
#include "rnl-util.hpp"

// base
#include "base/fs.hpp"
#include "base/source-loc.hpp"

// C++ PEG-lib
#include "peglib.h"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rnl {

namespace {

template<typename T>
T safe_any_cast( any const& v, base::SourceLoc const& location =
                                   base::SourceLoc::current() ) {
  if( v.type() != typeid( T const ) ) {
    throw runtime_error( fmt::format(
        "bad safe_any_cast on line: {}.\n", location.line() ) );
  }
  return any_cast<T const>( v );
}

template<typename T>
vector<T> safe_cast_vec( peg::SemanticValues const& sv,
                         base::SourceLoc const&     location =
                             base::SourceLoc::current() ) {
  vector<T> res;
  for( any const& v : sv )
    res.push_back( safe_any_cast<T>( v, location ) );
  return res;
}

} // namespace

optional<expr::Rnl> parse( string_view   peg_filename,
                           string_view   src_filename,
                           string const& peg_grammar,
                           string const& rnl_text ) {
  peg::parser parser;
  parser.log = [&]( size_t line, size_t col,
                    string const& msg ) {
    error( peg_filename, line, col, "{}", msg );
  };

  if( !parser.load_grammar( peg_grammar.c_str() ) ) {
    // Should not get here, since an error should call the above
    // log callback which should abort.
    exit( 1 );
  }
  parser.enable_packrat_parsing();

  // Change the logger since we are now attempting to parse the
  // rnl file.
  parser.log = [&]( size_t line, size_t col,
                    string const& msg ) {
    error( src_filename, line, col, "{}", msg );
  };

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
    string token = safe_any_cast<string>( sv.token() );
    optional<expr::e_sumtype_feature> o =
        expr::from_str( token );
    if( !o.has_value() )
      throw peg::parse_error(
          fmt::format( "unrecognized feature: \"{}\".", token )
              .c_str() );
    return *o;
  };

  parser["RNL"] = []( peg::SemanticValues const& sv ) {
    return expr::Rnl{
        .imports  = safe_any_cast<vector<string>>( sv[0] ),
        .includes = safe_any_cast<vector<string>>( sv[1] ),
        .items    = safe_any_cast<vector<expr::Item>>( sv[2] ),
    };
  };

  parser["IMPORTS"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<string>( sv );
  };

  parser["INCLUDES"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<string>( sv );
  };

  parser["TEMPLATES"] = []( peg::SemanticValues const& sv ) {
    vector<string> vs = safe_cast_vec<string>( sv );

    vector<expr::TemplateParam> res;
    for( string const& s : vs )
      res.push_back( expr::TemplateParam{ s } );
    return res;
  };

  parser["FEATURES"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<expr::e_sumtype_feature>( sv );
  };

  parser["ALT_VARS"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<expr::AlternativeMember>( sv );
  };

  parser["ALTERNATIVES"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<expr::Alternative>( sv );
  };

  parser["ITEMS"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<expr::Item>( sv );
  };

  parser["ITEM"] = []( peg::SemanticValues const& sv ) {
    return expr::Item{
        .ns = safe_any_cast<string>( sv[0] ),
        .constructs =
            safe_any_cast<vector<expr::Construct>>( sv[1] ) };
  };

  parser["CONSTRUCTS"] = []( peg::SemanticValues const& sv ) {
    return safe_cast_vec<expr::Construct>( sv );
  };

  parser["CONSTRUCT"] = []( peg::SemanticValues const& sv ) {
    if( sv[0].type() == typeid( expr::Sumtype const ) )
      return expr::Construct{
          safe_any_cast<expr::Sumtype>( sv[0] ) };
    throw peg::parse_error( "Unhandled construct type." );
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
    if( sv[i].type() ==
        typeid( vector<expr::e_sumtype_feature> ) )
      res.features =
          safe_any_cast<vector<expr::e_sumtype_feature>>(
              sv[i++] );
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
  if( !parser.parse( rnl_text.c_str(), parsed_rnl ) ) {
    // Should not get here, since an error should call the above
    // log callback which should abort.
    exit( 1 );
  }

  parsed_rnl.meta = expr::Metadata{
      .module_name = fs::path( src_filename ).filename().stem(),
  };

  return parsed_rnl;
}

} // namespace rnl
