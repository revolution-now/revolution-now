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
#include <iomanip>
#include <sstream>

using namespace std;

namespace rnl {

namespace {

void section( ostringstream& oss, string_view section ) {
  const int line_width = 65;
  char      c          = '-';
  string    comment    = "// ";
  string    bar( line_width - comment.size(), c );
  oss << comment + bar + "\n";
  // Subtract three to shift the section name to the left by two
  // to counter the "//" comment characters that we're putting at
  // the beginning.
  oss << comment
      << fmt::format( "{: ^{}}\n", section,
                      line_width - comment.size() - 2 );
  oss << comment + bar + "\n";
}

void emit_vert_list( ostringstream&        oss,
                     vector<string> const& lines,
                     string_view sep = "", bool quotes = false,
                     string_view spaces = "" ) {
  if( lines.empty() ) return;
  int count = lines.size();
  for( string const& line : lines ) {
    oss << spaces;
    string to_print = line;
    if( count-- > 1 ) to_print += string( sep );
    if( quotes )
      oss << std::quoted( to_print );
    else
      oss << to_print;
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

string alt_types_have_fmt(
    vector<expr::AlternativeMember> const& members ) {
  vector<string> res;
  for( expr::AlternativeMember const& member : members )
    res.push_back(
        fmt::format( "::rn::has_fmt<{}>", member.type ) );
  string sep = ", ";
  return absl::StrJoin( res, sep );
}

string template_params( vector<expr::TemplateParam> const& tmpls,
                        bool put_typename, bool space = true ) {
  if( tmpls.empty() ) return "";
  vector<string> names;
  string         tp_name = put_typename ? "typename " : "";
  for( expr::TemplateParam const& param : tmpls )
    names.push_back( tp_name + param.param );
  string sep = ",";
  if( space ) sep += ' ';
  return "<"s + absl::StrJoin( names, sep ) + ">";
}

string template_params_type_names(
    vector<expr::TemplateParam> const& tmpls ) {
  string params = template_params( tmpls, /*put_typename=*/false,
                                   /*space=*/true );
  return "::rn::type_list_to_names"s + params + "()";
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

void emit_fmt_for_alternative(
    ostringstream& oss, string_view ns, string_view sumtype_name,
    vector<expr::TemplateParam> const& tmpls,
    expr::Alternative const&           alt ) {
  if( !tmpls.empty() )
    emit_template_decl( oss, tmpls );
  else
    oss << "template<>\n";
  string full_alt_name = fmt::format(
      "{}::{}::{}{}", ns, sumtype_name, alt.name,
      template_params( tmpls, /*put_typename=*/false ) );
  oss << fmt::format( "struct fmt::formatter<{}>\n",
                      full_alt_name );
  oss << fmt::format( "  : formatter_base {{\n" );
  oss << "  "
      << "template<typename Context>\n";
  string maybe_o = alt.members.empty() ? "" : " o";
  oss << "  "
      << fmt::format(
             "auto format( {} const&{}, Context& ctx ) {{\n",
             full_alt_name, maybe_o );
  oss << "  "
      << "  "
      << "return formatter_base::format( fmt::format(\n";
  oss << "  "
      << "  "
      << "  ";
  if( tmpls.empty() )
    oss << fmt::format( "\"{}::{}", sumtype_name, alt.name );
  else
    oss << fmt::format( "\"{}::{}<{{}}>", sumtype_name,
                        alt.name );
  vector<string> fmt_args;
  if( !tmpls.empty() )
    fmt_args.push_back( template_params_type_names( tmpls ) );
  if( !alt.members.empty() ) {
    oss << "{{\"\n";
    vector<string> fmt_members;
    for( expr::AlternativeMember const& member : alt.members )
      fmt_members.push_back(
          fmt::format( "{}={{}}", member.var ) );
    emit_vert_list( oss, fmt_members, ",", /*quotes=*/true,
                    "        " );
    for( expr::AlternativeMember const& member : alt.members )
      fmt_args.push_back( fmt::format( "o.{}", member.var ) );
    oss << "  "
        << "  "
        << "  "
        << "\"}}";
  }
  if( !alt.members.empty() || !tmpls.empty() )
    oss << "\",\n";
  else
    oss << "\"\n";
  oss << "      ";
  if( !fmt_args.empty() )
    oss << absl::StrJoin( fmt_args, ", " ) << " ";
  oss << "), ctx );\n";
  oss << "  "
      << "}\n";
  oss << "};\n";
}

bool sumtype_has_feature( expr::Sumtype const& sumtype,
                          expr::e_feature      feature ) {
  for( auto type : sumtype.features ) {
    if( type == feature ) return true;
  }
  return false;
}

void emit( ostringstream& oss, string_view ns,
           string_view                        sumtype_name,
           vector<expr::TemplateParam> const& tmpls,
           expr::Alternative const& alt, bool emit_fmt ) {
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
  // Global namespace.
  if( emit_fmt )
    emit_fmt_for_alternative( oss, ns, sumtype_name, tmpls,
                              alt );
}

void emit( ostringstream& oss, string_view ns,
           expr::Sumtype const& sumtype ) {
  section( oss, "Sum Type: "s + sumtype.name );
  for( expr::Alternative const& alt : sumtype.alternatives ) {
    emit( oss, ns, sumtype.name, sumtype.tmpl_params, alt,
          sumtype_has_feature( sumtype,
                               expr::e_feature::formattable ) );
    oss << "\n";
  }
  open_ns( oss, ns );
  emit_template_decl( oss, sumtype.tmpl_params );
  oss << "using " << sumtype.name << "_t = ";
  if( sumtype.alternatives.empty() ) {
    oss << "std::monostate;\n";
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
      oss << fmt::format( "#include \"rnl/{}.hpp\"\n", import );
    oss << "\n";
  }

  if( !rnl.includes.empty() ) {
    section( oss, "Includes" );
    for( string const& include : rnl.includes )
      oss << fmt::format( "#include {}\n", include );
    oss << "\n";
  }
  oss << "// Revolution Now\n";
  oss << "#include \"cc-specific.hpp\"\n";
  oss << "\n";
  oss << "// base-util\n";
  oss << "#include \"base-util/mp.hpp\"\n";
  oss << "\n";
  oss << "// {fmt}\n";
  oss << "#include \"fmt/format.h\"\n";

  for( expr::Item const& item : rnl.items ) {
    emit( oss, item );
  }

  return oss.str();
}

} // namespace rnl
