/****************************************************************
**expr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Expression objects for the RNL language.
*
*****************************************************************/
#include "expr.hpp"

// base
#include "base/maybe.hpp"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rnl::expr {

using ::base::maybe;
using ::base::nothing;

string AlternativeMember::to_string( string_view spaces ) const {
  string res =
      fmt::format( "{}( {}, {} )\n", spaces, type, var );
  return res;
}

string Alternative::to_string( string_view spaces ) const {
  string res = fmt::format( "{}{}:\n", spaces, name );
  for( AlternativeMember const& mem : members )
    res += mem.to_string( string( spaces ) + "  " );
  return res;
}

std::string to_str( e_sumtype_feature feature ) {
  switch( feature ) {
    case e_sumtype_feature::formattable: return "formattable";
    case e_sumtype_feature::serializable: return "serializable";
    case e_sumtype_feature::equality: return "equality";
  }
    // TODO: When C++20 comes change this to the new
    // [[unreachable]].
#ifndef _MSC_VER
  // POSIX.
  __builtin_unreachable();
#else
  // MSVC.
  __assume( false );
#endif
}

maybe<e_sumtype_feature> from_str( std::string feature ) {
  if( feature == "formattable" )
    return e_sumtype_feature::formattable;
  if( feature == "serializable" )
    return e_sumtype_feature::serializable;
  if( feature == "equality" ) return e_sumtype_feature::equality;
  return nothing;
}

std::string Sumtype::to_string( std::string_view spaces ) const {
  string res = fmt::format( "{}sumtype: {}\n", spaces, name );
  for( TemplateParam const& tmpl_param : tmpl_params )
    res += fmt::format( "{}  templ: {}\n", spaces,
                        tmpl_param.param );
  if( features.has_value() ) {
    for( e_sumtype_feature feature : *features )
      res += fmt::format( "{}  feature: {}\n", spaces,
                          to_str( feature ) );
  }
  for( Alternative const& alt : alternatives )
    res += alt.to_string( string( spaces ) + "  " );
  return res;
}

std::string to_str( Construct const& construct,
                    std::string_view spaces ) {
  // TODO: use `overload`.
  auto visitor = [&]( auto const& v ) {
    return v.to_string( spaces );
  };
  return visit( visitor, construct );
}

std::string Item::to_string( std::string_view spaces ) const {
  string res = fmt::format( "\n{}namespace: {}\n", spaces, ns );
  for( Construct const& construct : constructs )
    res += "\n" + to_str( construct, string( spaces ) + "  " );
  return res;
}

std::string Rnl::to_string() const {
  string res =
      fmt::format( "module name: {}\n", meta.module_name );
  res += fmt::format( "imports:\n" );
  for( string const& import : imports )
    res += fmt::format( "  {}\n", import );
  res += fmt::format( "\nincludes:\n" );
  for( string const& include : includes )
    res += fmt::format( "  {}\n", include );
  res += fmt::format( "\nitems:\n" );
  for( Item const& item : items ) res += item.to_string( "  " );
  return res;
}
} // namespace rnl::expr
