/****************************************************************
**code-gen.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Code generator for the RNL language.
*
*****************************************************************/
#include "code-gen.hpp"

// rnlc
#include "rnl-util.hpp"

// {fmt}
#include "fmt/format.h"

// Abseil
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"

// c++ standard library
#include <sstream>

using namespace std;

namespace rnl {

namespace {

void section( ostringstream& oss, string_view section ) {
  oss << "// "
         "======================================================"
         "========\n";
  oss << "// " << section << "\n";
  oss << "// "
         "======================================================"
         "========\n";
}

void emit_vert_list( ostringstream&        oss,
                     vector<string> const& lines,
                     string_view           sep = "" ) {
  if( lines.empty() ) return;
  int count = lines.size();
  for( string const& line : lines ) {
    oss << line;
    if( count-- > 1 ) oss << sep;
    oss << "\n";
  }
}

void open_ns( ostringstream& oss, string_view ns,
              string_view leaf = "" ) {
  oss << "namespace " << ns;
  if( !leaf.empty() ) oss << absl::StrCat( "::", leaf );
  oss << " {\n";
}

void close_ns( ostringstream& oss, string_view ns,
               string_view leaf = "" ) {
  oss << "}  // namespace " << ns;
  if( !leaf.empty() ) oss << absl::StrCat( "::", leaf );
  oss << "\n";
}

string template_params( vector<expr::TemplateParam> const& tmpls,
                        bool put_typename ) {
  if( tmpls.empty() ) return "";
  vector<string> names;
  string         tp_name = put_typename ? "typename " : "";
  for( expr::TemplateParam const& param : tmpls )
    names.push_back( tp_name + param.param );
  return "<"s + absl::StrJoin( names, ", " ) + ">";
}

void emit_template_params(
    ostringstream& oss, vector<expr::TemplateParam> const& tmpls,
    bool put_typename ) {
  oss << template_params( tmpls, put_typename );
}

void emit_template_decl(
    ostringstream&                     oss,
    vector<expr::TemplateParam> const& tmpls ) {
  if( tmpls.empty() ) return;
  oss << "template";
  emit_template_params( oss, tmpls, /*put_typename=*/true );
  oss << "\n";
}

void emit( ostringstream& oss, string_view ns,
           string_view                        sumtype_name,
           vector<expr::TemplateParam> const& tmpls,
           expr::Alternative const&           alt ) {
  open_ns( oss, ns, sumtype_name );
  emit_template_decl( oss, tmpls );
  oss << "struct " << alt.name;
  if( alt.members.empty() ) {
    oss << " {};\n";
  } else {
    oss << " {\n";
    for( expr::AlternativeMember const& alt_mem : alt.members )
      oss << "  " << alt_mem.type << " " << alt_mem.var << ";\n";
    oss << "};\n";
  }
  close_ns( oss, ns, sumtype_name );
}

void emit( ostringstream& oss, string_view ns,
           expr::Sumtype const& sumtype ) {
  section( oss, "Sum Type: "s + sumtype.name );
  for( expr::Alternative const& alt : sumtype.alternatives ) {
    emit( oss, ns, sumtype.name, sumtype.tmpl_params, alt );
    oss << "\n";
  }
  open_ns( oss, ns );
  emit_template_decl( oss, sumtype.tmpl_params );
  oss << "using " << sumtype.name << "_t = ";
  if( sumtype.alternatives.empty() ) {
    oss << "std::variant<>;\n";
  } else {
    oss << "std::variant<\n";
    vector<string> variants;
    for( expr::Alternative const& alt : sumtype.alternatives )
      variants.push_back( absl::StrCat(
          "  ", sumtype.name, "::", alt.name,
          template_params( sumtype.tmpl_params,
                           /*put_typename=*/false ) ) );
    emit_vert_list( oss, variants, "," );
    oss << ">;\n";
  }
  close_ns( oss, ns );
}

void emit( ostringstream& oss, expr::Item const& item ) {
  string cpp_ns =
      absl::StrReplaceAll( item.ns, { { ".", "::" } } );
  auto visitor = [&]( auto const& v ) {
    emit( oss, cpp_ns, v );
  };
  for( expr::Construct const& construct : item.constructs ) {
    oss << "\n";
    visit( visitor, construct );
  }
}

} // namespace

optional<string> generate_code( expr::Rnl const& rnl ) {
  ostringstream oss;
  oss << "#pragma once\n";
  oss << "\n";

  if( !rnl.imports.empty() ) {
    section( oss, "Imports" );
    for( string const& import : rnl.imports )
      oss << fmt::format( "#include \"rnl/{}.rnl\"\n", import );
  }

  if( !rnl.includes.empty() ) {
    section( oss, "Includes" );
    for( string const& include : rnl.includes )
      oss << fmt::format( "#include {}\n", include );
  }

  for( expr::Item const& item : rnl.items ) {
    emit( oss, item );
  }

  return oss.str();
}

} // namespace rnl
