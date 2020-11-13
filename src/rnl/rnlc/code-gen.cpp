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

bool sumtype_has_feature( expr::Sumtype const& sumtype,
                          expr::e_feature      feature ) {
  for( auto type : sumtype.features ) {
    if( type == feature ) return true;
  }
  return false;
}

struct CodeGenerator {
  ostringstream oss_;

  void section( string_view section ) {
    const int line_width = 65;
    char      c          = '-';
    string    comment    = "// ";
    string    bar( line_width - comment.size(), c );
    oss_ << comment + bar + "\n";
    // Subtract three to shift the section name to the left by
    // two to counter the "//" comment characters that we're
    // putting at the beginning.
    oss_ << comment
         << fmt::format( "{: ^{}}\n", section,
                         line_width - comment.size() - 2 );
    oss_ << comment + bar + "\n";
  }

  void emit_vert_list( vector<string> const& lines,
                       string_view sep = "", bool quotes = false,
                       string_view spaces = "" ) {
    if( lines.empty() ) return;
    int count = lines.size();
    for( string const& line : lines ) {
      oss_ << spaces;
      string to_print = line;
      if( count-- > 1 ) to_print += string( sep );
      if( quotes )
        oss_ << std::quoted( to_print );
      else
        oss_ << to_print;
      oss_ << "\n";
    }
  }

  void open_ns( string_view ns, string_view leaf = "" ) {
    oss_ << "namespace " << ns;
    if( !leaf.empty() ) oss_ << absl::StrCat( "::", leaf );
    oss_ << " {\n";
  }

  void close_ns( string_view ns, string_view leaf = "" ) {
    oss_ << "}  // namespace " << ns;
    if( !leaf.empty() ) oss_ << absl::StrCat( "::", leaf );
    oss_ << "\n";
  }

  void emit_template_params(
      vector<expr::TemplateParam> const& tmpls,
      bool                               put_typename ) {
    oss_ << template_params( tmpls, put_typename );
  }

  void emit_template_decl(
      vector<expr::TemplateParam> const& tmpls ) {
    if( tmpls.empty() ) return;
    oss_ << "template";
    emit_template_params( tmpls, /*put_typename=*/true );
    oss_ << "\n";
  }

  void emit_fmt_for_alternative(
      string_view ns, string_view sumtype_name,
      vector<expr::TemplateParam> const& tmpls,
      expr::Alternative const&           alt ) {
    if( !tmpls.empty() )
      emit_template_decl( tmpls );
    else
      oss_ << "template<>\n";
    string full_alt_name = fmt::format(
        "{}::{}::{}{}", ns, sumtype_name, alt.name,
        template_params( tmpls, /*put_typename=*/false ) );
    oss_ << fmt::format( "struct fmt::formatter<{}>\n",
                         full_alt_name );
    oss_ << fmt::format( "  : formatter_base {{\n" );
    oss_ << "  "
         << "template<typename Context>\n";
    string maybe_o = alt.members.empty() ? "" : " o";
    oss_ << "  "
         << fmt::format(
                "auto format( {} const&{}, Context& ctx ) {{\n",
                full_alt_name, maybe_o );
    oss_ << "  "
         << "  "
         << "return formatter_base::format( fmt::format(\n";
    oss_ << "  "
         << "  "
         << "  ";
    if( tmpls.empty() )
      oss_ << fmt::format( "\"{}::{}", sumtype_name, alt.name );
    else
      oss_ << fmt::format( "\"{}::{}<{{}}>", sumtype_name,
                           alt.name );
    vector<string> fmt_args;
    if( !tmpls.empty() )
      fmt_args.push_back( template_params_type_names( tmpls ) );
    if( !alt.members.empty() ) {
      oss_ << "{{\"\n";
      vector<string> fmt_members;
      for( expr::AlternativeMember const& member : alt.members )
        fmt_members.push_back(
            fmt::format( "{}={{}}", member.var ) );
      emit_vert_list( fmt_members, ",", /*quotes=*/true,
                      "        " );
      for( expr::AlternativeMember const& member : alt.members )
        fmt_args.push_back( fmt::format( "o.{}", member.var ) );
      oss_ << "  "
           << "  "
           << "  "
           << "\"}}";
    }
    if( !alt.members.empty() || !tmpls.empty() )
      oss_ << "\",\n";
    else
      oss_ << "\"\n";
    oss_ << "      ";
    if( !fmt_args.empty() )
      oss_ << absl::StrJoin( fmt_args, ", " ) << " ";
    oss_ << "), ctx );\n";
    oss_ << "  "
         << "}\n";
    oss_ << "};\n";
  }

  void emit( string_view ns, string_view sumtype_name,
             vector<expr::TemplateParam> const& tmpls,
             expr::Alternative const& alt, bool emit_fmt ) {
    open_ns( ns, sumtype_name );
    emit_template_decl( tmpls );
    oss_ << "struct " << alt.name;
    if( alt.members.empty() ) {
      oss_ << " {};\n";
    } else {
      oss_ << " {\n";
      for( expr::AlternativeMember const& alt_mem : alt.members )
        oss_ << "  " << alt_mem.type << " " << alt_mem.var
             << ";\n";
      oss_ << "};\n";
    }
    close_ns( ns, sumtype_name );
    // Global namespace.
    if( emit_fmt )
      emit_fmt_for_alternative( ns, sumtype_name, tmpls, alt );
  }

  void emit( string_view ns, expr::Sumtype const& sumtype ) {
    section( "Sum Type: "s + sumtype.name );
    for( expr::Alternative const& alt : sumtype.alternatives ) {
      emit( ns, sumtype.name, sumtype.tmpl_params, alt,
            sumtype_has_feature(
                sumtype, expr::e_feature::formattable ) );
      oss_ << "\n";
    }
    open_ns( ns );
    emit_template_decl( sumtype.tmpl_params );
    oss_ << "using " << sumtype.name << "_t = ";
    if( sumtype.alternatives.empty() ) {
      oss_ << "std::monostate;\n";
    } else {
      oss_ << "std::variant<\n";
      vector<string> variants;
      for( expr::Alternative const& alt : sumtype.alternatives )
        variants.push_back( absl::StrCat(
            "  ", sumtype.name, "::", alt.name,
            template_params( sumtype.tmpl_params,
                             /*put_typename=*/false ) ) );
      emit_vert_list( variants, "," );
      oss_ << ">;\n";
    }
    close_ns( ns );
  }

  void emit( expr::Item const& item ) {
    string cpp_ns =
        absl::StrReplaceAll( item.ns, { { ".", "::" } } );
    auto visitor = [&]( auto const& v ) { emit( cpp_ns, v ); };
    for( expr::Construct const& construct : item.constructs ) {
      oss_ << "\n";
      visit( visitor, construct );
    }
  }

  void emit_preamble() {
    oss_ << "#pragma once\n";
    oss_ << "\n";
  }

  void emit_imports( vector<string> const& imports ) {
    if( !imports.empty() ) {
      section( "Imports" );
      for( string const& import : imports )
        oss_ << fmt::format( "#include \"rnl/{}.hpp\"\n",
                             import );
      oss_ << "\n";
    }
  }

  void emit_includes( vector<string> const& includes ) {
    if( !includes.empty() ) {
      section( "Includes" );
      for( string const& include : includes )
        oss_ << fmt::format( "#include {}\n", include );
      oss_ << "\n";
    }

    oss_ << "// Revolution Now\n";
    oss_ << "#include \"cc-specific.hpp\"\n";
    oss_ << "\n";
    oss_ << "// base-util\n";
    oss_ << "#include \"base-util/mp.hpp\"\n";
    oss_ << "\n";
    oss_ << "// {fmt}\n";
    oss_ << "#include \"fmt/format.h\"\n";
  }

  void emit_rnl( expr::Rnl const& rnl ) {
    emit_preamble();
    emit_imports( rnl.imports );
    emit_includes( rnl.includes );

    for( expr::Item const& item : rnl.items ) emit( item );
  }
};

} // namespace

optional<string> generate_code( expr::Rnl const& rnl ) {
  CodeGenerator gen;
  gen.emit_rnl( rnl );
  return gen.oss_.str();
}

} // namespace rnl
