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

// base-util
#include "base-util/misc.hpp"

// {fmt}
#include "fmt/format.h"

// Abseil
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"

// c++ standard library
#include <iomanip>
#include <sstream>
#include <stack>

using namespace std;

namespace rnl {

namespace {

template<typename Range, typename Projection>
auto max_of( Range&& rng, Projection&& proj ) {
  assert( !rng.empty() );
  auto t = proj( *rng.begin() );
  for( auto&& elem : forward<Range>( rng ) ) {
    auto p = proj( elem );
    if( p > t ) t = p;
  }
  return t;
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

bool sumtype_has_feature( expr::Sumtype const& sumtype,
                          expr::e_feature      feature ) {
  for( auto type : sumtype.features ) {
    if( type == feature ) return true;
  }
  return false;
}

string trim_trailing_spaces( string s ) {
  string_view sv             = s;
  auto        last_non_space = sv.find_last_not_of( ' ' );
  if( last_non_space != string_view::npos ) {
    auto trim_start = last_non_space + 1;
    sv.remove_suffix( sv.size() - trim_start );
  }
  return string( sv );
}

struct CodeGenerator {
  struct Options {
    int  indent_level = 0;
    bool quotes       = false;

    auto operator<=>( Options const& ) const = default;
  };

  ostringstream    oss_;
  optional<string> curr_line_;
  Options          default_options_ = {};

  stack<Options> options_;

  Options& options() {
    if( options_.empty() ) return default_options_;
    return options_.top();
  }

  Options const& options() const {
    if( options_.empty() ) return default_options_;
    return options_.top();
  }

  void push( Options options ) {
    options_.push( move( options ) );
  }

  void pop() {
    assert( !options_.empty() );
    options_.pop();
  }

  struct [[nodiscard]] AutoPopper {
    CodeGenerator* gen_;
    AutoPopper( CodeGenerator& gen ) : gen_( &gen ) {
      assert( gen_ != nullptr );
    }
    AutoPopper( AutoPopper const& ) = delete;
    AutoPopper& operator=( AutoPopper const& ) = delete;
    AutoPopper& operator=( AutoPopper&& ) = delete;
    AutoPopper( AutoPopper&& rhs ) {
      gen_     = rhs.gen_;
      rhs.gen_ = nullptr;
    }
    ~AutoPopper() {
      if( gen_ ) gen_->pop();
    }
    void cancel() { gen_ = nullptr; }
  };

  AutoPopper indent( int levels = 1 ) {
    push( options() );
    options().indent_level += levels;
    return AutoPopper( *this );
  }

  AutoPopper quoted() {
    push( options() );
    options().quotes = true;
    return AutoPopper( *this );
  }

  string result() const {
    assert( !curr_line_.has_value() );
    assert( options_.empty() );
    assert( options() == Options{} );
    return oss_.str();
  }

  // Format string should not contain any new lines in it. This
  // is the only function that should be using oss_.
  template<typename... Args>
  void line( string_view fmt_str, Args... args ) {
    assert( !curr_line_.has_value() );
    assert( fmt_str.find_first_of( "\n" ) == string_view::npos );
    string indent( options().indent_level * 2, ' ' );
    string to_print = trim_trailing_spaces(
        fmt::format( fmt_str, forward<Args>( args )... ) );
    // Only print empty strings if they are to be quoted.
    if( options().quotes )
      oss_ << indent << std::quoted( to_print );
    else if( !to_print.empty() )
      oss_ << indent << to_print;
    oss_ << "\n";
  }

  template<typename... Args>
  void frag( string_view fmt_str, Args... args ) {
    assert( fmt_str.find_first_of( "\n" ) == string_view::npos );
    if( !curr_line_.has_value() ) curr_line_.emplace();
    curr_line_ = absl::StrCat(
        *curr_line_,
        fmt::format( fmt_str, forward<Args>( args )... ) );
  }

  void flush() {
    if( !curr_line_.has_value() ) return;
    string to_write = move( *curr_line_ );
    curr_line_.reset();
    line( "{}", to_write );
  }

  void newline() { line( "" ); }

  template<typename... Args>
  void comment( string_view fmt_str, Args... args ) {
    frag( "// " );
    frag( fmt_str, forward<Args>( args )... );
    flush();
  }

  void section( string_view section ) {
    const int line_width = 65;
    char      c          = '-';
    string    bar( line_width - 3, c );
    line( "/{}", string( line_width - 1, '*' ) );
    line( "*{: ^{}}", section, line_width - 2 );
    line( "{}/", string( line_width, '*' ) );
  }

  void emit_vert_list( vector<string> const& lines,
                       string_view           sep ) {
    int count = lines.size();
    for( string const& l : lines ) {
      if( count-- == 1 ) sep = "";
      line( "{}{}", l, sep );
    }
  }

  void open_ns( string_view ns, string_view leaf = "" ) {
    frag( "namespace {}", ns );
    if( !leaf.empty() ) frag( "::{}", leaf );
    frag( " {{" );
    flush();
    newline();
    indent().cancel();
  }

  void close_ns( string_view ns, string_view leaf = "" ) {
    pop();
    frag( "}} // namespace {}", ns );
    if( !leaf.empty() ) frag( "::{}", leaf );
    flush();
  }

  void emit_template_decl(
      vector<expr::TemplateParam> const& tmpls ) {
    if( tmpls.empty() ) return;
    line( "template{}",
          template_params( tmpls, /*put_typename=*/true ) );
  }

  void emit_format_str_for_formatting_alternative(
      expr::Alternative const&           alt,
      vector<expr::TemplateParam> const& tmpls,
      string_view                        sumtype_name ) {
    auto _ = quoted();
    if( tmpls.empty() )
      frag( "{}::{}", sumtype_name, alt.name );
    else
      frag( "{}::{}<{{}}>", sumtype_name, alt.name );
    if( !alt.members.empty() ) frag( "{{{{" );
    flush();
    if( !alt.members.empty() ) {
      vector<string> fmt_members;
      for( expr::AlternativeMember const& member : alt.members )
        fmt_members.push_back(
            fmt::format( "{}={{}}", member.var ) );
      {
        auto _ = indent();
        emit_vert_list( fmt_members, "," );
      }
      line( "}}}}" );
    }
  }

  void emit_fmt_format_method(
      expr::Alternative const& alt, string_view full_alt_name,
      vector<expr::TemplateParam> const& tmpls,
      string_view                        sumtype_name ) {
    line( "template<typename Context>" );
    string maybe_o = alt.members.empty() ? "" : " o";
    line( "auto format( {} const&{}, Context& ctx ) {{",
          full_alt_name, maybe_o );
    {
      auto _ = indent();
      line( "return formatter_base::format( fmt::format(" );
      {
        auto _ = indent();
        emit_format_str_for_formatting_alternative(
            alt, tmpls, sumtype_name );
        if( !alt.members.empty() || !tmpls.empty() )
          frag( ", " );
        vector<string> fmt_args;
        if( !tmpls.empty() )
          fmt_args.push_back(
              template_params_type_names( tmpls ) );
        for( expr::AlternativeMember const& member :
             alt.members )
          fmt_args.push_back(
              fmt::format( "o.{}", member.var ) );
        frag( "{} ), ctx );", absl::StrJoin( fmt_args, ", " ) );
        flush();
      }
    }
    line( "}}" );
  }

  void emit_fmt_for_alternative(
      string_view ns, string_view sumtype_name,
      vector<expr::TemplateParam> const& tmpls,
      expr::Alternative const&           alt ) {
    if( !tmpls.empty() )
      emit_template_decl( tmpls );
    else
      line( "template<>" );
    string full_alt_name = fmt::format(
        "{}::{}::{}{}", ns, sumtype_name, alt.name,
        template_params( tmpls, /*put_typename=*/false ) );
    line( "struct fmt::formatter<{}>", full_alt_name );
    {
      auto _ = indent();
      line( ": formatter_base {{" );
      emit_fmt_format_method( alt, full_alt_name, tmpls,
                              sumtype_name );
    }
    line( "}};" );
  }

  void emit( vector<expr::TemplateParam> const& tmpls,
             expr::Alternative const&           alt ) {
    emit_template_decl( tmpls );
    if( alt.members.empty() ) {
      line( "struct {} {{}};", alt.name );
    } else {
      line( "struct {} {{", alt.name );
      {
        auto cleanup = indent();
        int  max_type_len =
            max_of( alt.members, L( _.type.size() ) );
        for( expr::AlternativeMember const& alt_mem :
             alt.members )
          line( "{: <{}} {};", alt_mem.type, max_type_len,
                alt_mem.var );
      }
      line( "}};" );
    }
  }

  void emit( string_view ns, expr::Sumtype const& sumtype ) {
    section( "Sum Type: "s + sumtype.name );
    open_ns( ns );
    if( !sumtype.alternatives.empty() ) {
      open_ns( sumtype.name );
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        emit( sumtype.tmpl_params, alt );
        newline();
      }
      close_ns( sumtype.name );
      newline();
    }
    emit_template_decl( sumtype.tmpl_params );
    if( sumtype.alternatives.empty() ) {
      line( "using {}_t = std::monostate;", sumtype.name );
    } else {
      line( "using {}_t = std::variant<", sumtype.name );
      vector<string> variants;
      for( expr::Alternative const& alt : sumtype.alternatives )
        variants.push_back( absl::StrCat(
            "  ", sumtype.name, "::", alt.name,
            template_params( sumtype.tmpl_params,
                             /*put_typename=*/false ) ) );
      emit_vert_list( variants, "," );
      line( ">;" );
    }
    newline();
    close_ns( ns );
    // Global namespace.
    if( sumtype_has_feature( sumtype,
                             expr::e_feature::formattable ) ) {
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        newline();
        string alt_name = fmt::format( "{}::{}::{}", ns,
                                       sumtype.name, alt.name );
        comment( "{}", alt_name );
        emit_fmt_for_alternative( ns, sumtype.name,
                                  sumtype.tmpl_params, alt );
      }
    }
  }

  void emit( expr::Item const& item ) {
    string cpp_ns =
        absl::StrReplaceAll( item.ns, { { ".", "::" } } );
    auto visitor = [&]( auto const& v ) { emit( cpp_ns, v ); };
    for( expr::Construct const& construct : item.constructs ) {
      newline();
      visit( visitor, construct );
    }
  }

  void emit_preamble() {
    line( "#pragma once" );
    newline();
  }

  void emit_imports( vector<string> const& imports ) {
    if( imports.empty() ) return;
    section( "Imports" );
    for( string const& import : imports )
      line( "#include \"rnl/{}.hpp\"", import );
    newline();
  }

  void emit_includes( vector<string> const& includes ) {
    if( includes.empty() ) return;
    section( "Includes" );
    for( string const& include : includes )
      line( "#include {}", include );
    newline();

    line( "// Revolution Now" );
    line( "#include \"cc-specific.hpp\"" );
    line( "" );
    line( "// base-util" );
    line( "#include \"base-util/mp.hpp\"" );
    line( "" );
    line( "// {{fmt}}" );
    line( "#include \"fmt/format.h\"" );
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
  return gen.result();
}

} // namespace rnl
