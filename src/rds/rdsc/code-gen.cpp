/****************************************************************
**code-gen.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Code generator for the RDS language.
*
*****************************************************************/
#include "code-gen.hpp"

// rdsc
#include "rds-util.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/lambda.hpp"
#include "base/maybe.hpp"
#include "base/meta.hpp"
#include "base/string.hpp"

// c++ standard library
#include <iomanip>
#include <sstream>
#include <stack>

using namespace std;

namespace rds {

namespace {

using ::base::maybe;

template<typename Range, typename Projection, typename Default>
auto max_of( Range&& rng, Projection&& proj, Default value )
    -> invoke_result_t<Projection,
                       typename decay_t<Range>::value_type> {
  if( rng.empty() ) return value;
  auto t = proj( *rng.begin() );
  for( auto&& elem : std::forward<Range>( rng ) ) {
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
  string res = "<";
  res += base::str_join( names, sep );
  res += ">";
  return res;
}

string all_int_tmpl_params( int count ) {
  vector<expr::TemplateParam> params(
      count, expr::TemplateParam{ "int" } );
  return template_params( params, /*put_typename=*/false,
                          /*space=*/true );
}

template<typename T>
bool item_has_feature( T const& item, expr::e_feature feature ) {
  return item.features.has_value() &&
         item.features->contains( feature );
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

    bool operator==( Options const& ) const = default;
  };

  ostringstream oss_;
  maybe<string> curr_line_;
  Options       default_options_ = {};

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
    options_.push( std::move( options ) );
  }

  void pop() {
    CHECK( !options_.empty() );
    options_.pop();
  }

  struct [[nodiscard]] AutoPopper {
    CodeGenerator* gen_;
    AutoPopper( CodeGenerator& gen ) : gen_( &gen ) {
      CHECK( gen_ != nullptr );
    }
    AutoPopper( AutoPopper const& )            = delete;
    AutoPopper& operator=( AutoPopper const& ) = delete;
    AutoPopper& operator=( AutoPopper&& )      = delete;
    AutoPopper( AutoPopper&& rhs )             = delete;
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

  string result() const {
    CHECK( !curr_line_.has_value() );
    CHECK( options_.empty() );
    CHECK( options() == Options{} );
    return oss_.str();
  }

  // Format string should not contain any new lines in it. This
  // is the only function that should be using oss_.
  template<typename Arg1, typename... Args>
  void line( string_view fmt_str, Arg1&& arg1, Args&&... args ) {
    CHECK( !curr_line_.has_value() );
    CHECK( fmt_str.find_first_of( "\n" ) == string_view::npos );
    string indent( options().indent_level * 2, ' ' );
    string to_print = trim_trailing_spaces( fmt::format(
        fmt::runtime( fmt_str ), std::forward<Arg1>( arg1 ),
        std::forward<Args>( args )... ) );
    // Only print empty strings if they are to be quoted.
    if( options().quotes )
      oss_ << indent << std::quoted( to_print );
    else if( !to_print.empty() )
      oss_ << indent << to_print;
    oss_ << "\n";
  }

  // Braces {} do NOT have to be escaped for this one.
  void line( string_view l ) { line( "{}", l ); }

  template<typename Arg1, typename... Args>
  void frag( string_view fmt_str, Arg1&& arg1, Args&&... args ) {
    CHECK( fmt_str.find_first_of( "\n" ) == string_view::npos );
    if( !curr_line_.has_value() ) curr_line_.emplace();
    curr_line_ = *curr_line_ +
                 fmt::format( fmt::runtime( fmt_str ),
                              std::forward<Arg1>( arg1 ),
                              std::forward<Args>( args )... );
  }

  // Braces {} do NOT have to be escaped for this one.
  void frag( string_view l ) { frag( "{}", l ); }

  void flush() {
    if( !curr_line_.has_value() ) return;
    string to_write = std::move( *curr_line_ );
    curr_line_.reset();
    line( to_write );
  }

  void newline() { line( "" ); }

  template<typename... Args>
  void comment( string_view fmt_str, Args&&... args ) {
    frag( "// " );
    frag( "{}", fmt::format( fmt::runtime( fmt_str ),
                             std::forward<Args>( args )... ) );
    flush();
  }

  void section( string_view section ) {
    const int line_width = 65;
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
    frag( " {" );
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

  void emit_sumtype_alternative(
      vector<expr::TemplateParam> const& tmpls,
      expr::Alternative const& alt, bool emit_equality,
      bool emit_validation ) {
    emit_template_decl( tmpls );
    if( alt.members.empty() && !emit_equality &&
        !emit_validation ) {
      line( "struct {} {{}};", alt.name );
    } else {
      line( "struct {} {{", alt.name );
      {
        auto cleanup = indent();
        int  max_type_len =
            max_of( alt.members, L( _.type.size() ), 0 );
        for( expr::StructMember const& alt_mem : alt.members )
          line( "{: <{}} {} = {{}};", alt_mem.type, max_type_len,
                alt_mem.var );
        if( emit_equality ) {
          comment( "{}",
                   "This requires that the types of the member "
                   "variables " );
          comment( "{}", "also support equality." );
          // We need the 'struct' keyword in fron of the
          // alternative name to disambiguate in cases where
          // there is an alternative member with the same name as
          // the alternative.
          line(
              "bool operator==( struct {} const& ) const = "
              "default;",
              alt.name );
          line(
              "bool operator!=( struct {} const& ) const = "
              "default;",
              alt.name );
        }
        if( emit_validation ) {
          newline();
          comment( "Validates invariants among members." );
          comment( "defined in some translation unit." );
          line(
              "base::valid_or<std::string> validate() const;" );
        }
      }
      line( "};" );
    }
  }

  void emit_enum_for_sumtype(
      vector<expr::Alternative> const& alternatives ) {
    CHECK( !alternatives.empty() );
    line( "enum class e {" );
    {
      auto _ = indent();
      for( expr::Alternative const& alternative : alternatives )
        line( "{},", alternative.name );
    }
    line( "};" );
  }

  void emit_variant_to_enum_specialization(
      string_view ns, expr::Sumtype const& sumtype ) {
    if( sumtype.alternatives.empty() ) return;
    string full_sumtype_name =
        fmt::format( "{}::{}_t{}", ns, sumtype.name,
                     template_params( sumtype.tmpl_params,
                                      /*put_typename=*/false ) );
    newline();
    comment(
        "This gives us the enum to use in a switch "
        "statement." );
    if( sumtype.tmpl_params.empty() )
      line( "template<>" );
    else
      emit_template_decl( sumtype.tmpl_params );
    line( "struct base::variant_to_enum<{}> {{",
          full_sumtype_name );
    {
      auto _ = indent();
      line( "using type = {}::{}::e;", ns, sumtype.name );
    }
    line( "};" );
  }

  void emit( string_view ns, expr::Enum const& e ) {
    section( "Enum: "s + e.name );
    open_ns( ns );
    string const nodiscard_str =
        item_has_feature( e, expr::e_feature::nodiscard )
            ? "[[nodiscard]] "
            : "";
    line( "enum class {}{} {{", nodiscard_str, e.name );
    {
      auto _ = indent();
      emit_vert_list( e.values, "," );
    }
    line( "};" );
    newline();
    close_ns( ns );
    // Emit the reflection traits.
    newline();
    open_ns( "refl" );
    comment( "Reflection info for enum {}.", e.name );
    line( "template<>" );
    line( "struct traits<{}::{}> {{", ns, e.name );
    {
      auto _ = indent();
      line( "using type = {}::{};", ns, e.name );
      newline();
      line(
          "static constexpr type_kind kind        = "
          "type_kind::enum_kind;" );
      line( "static constexpr std::string_view ns   = \"{}\";",
            ns );
      line( "static constexpr std::string_view name = \"{}\";",
            e.name );
      newline();
      frag(
          "static constexpr std::array<std::string_view, {}> "
          "value_names{{",
          e.values.size() );
      if( e.values.empty() ) {
        frag( "};" );
        flush();
      } else {
        flush();
        {
          auto _ = indent();
          for( string const& s : e.values ) line( "\"{}\",", s );
        }
        line( "};" );
      }
    }
    line( "};" );
    newline();
    close_ns( "refl" );
  }

  void emit_reflection_for_struct(
      string_view                        ns,
      vector<expr::TemplateParam> const& tmpl_params,
      string const&                      name,
      vector<expr::StructMember> const&  members,
      bool                               wants_offsets ) {
    comment( "Reflection info for struct {}.", name );
    string tmpl_brackets =
        tmpl_params.empty()
            ? "<>"
            : template_params( tmpl_params,
                               /*put_typename=*/false );
    string tmpl_brackets_typename =
        tmpl_params.empty()
            ? "<>"
            : template_params( tmpl_params,
                               /*put_typename=*/true );
    line( "template{}", tmpl_brackets_typename );
    string name_w_tmpl =
        fmt::format( "{}{}", name,
                     template_params( tmpl_params,
                                      /*put_typename=*/false ) );
    string full_name_w_tmpl =
        fmt::format( "{}::{}", ns, name_w_tmpl );
    line( "struct traits<{}> {{", full_name_w_tmpl );
    {
      auto _ = indent();
      line( "using type = {};", full_name_w_tmpl );
      newline();
      line(
          "static constexpr type_kind kind        = "
          "type_kind::struct_kind;" );
      line( "static constexpr std::string_view ns   = \"{}\";",
            ns );
      line( "static constexpr std::string_view name = \"{}\";",
            name );
      newline();
      line( "using template_types = std::tuple{};",
            tmpl_brackets );
      newline();
      frag( "static constexpr std::tuple fields{" );
      if( members.empty() ) {
        frag( "};" );
        flush();
      } else {
        flush();
        {
          auto _ = indent();
          for( expr::StructMember const& sm : members ) {
            string offset = "/*offset=*/base::nothing";
            if( wants_offsets )
              offset =
                  fmt::format( "offsetof( type, {} )", sm.var );
            line( "refl::StructField{{ \"{}\", &{}::{}, {} }},",
                  sm.var, full_name_w_tmpl, sm.var, offset );
          }
        }
        line( "};" );
      }
    }
    line( "};" );
  }

  void emit( string_view ns, expr::Struct const& strukt ) {
    section( "Struct: "s + strukt.name );
    open_ns( ns );
    emit_template_decl( strukt.tmpl_params );
    bool comparable =
        item_has_feature( strukt, expr::e_feature::equality );
    bool         has_members = !strukt.members.empty();
    string const nodiscard_str =
        item_has_feature( strukt, expr::e_feature::nodiscard )
            ? "[[nodiscard]] "
            : "";
    if( !has_members && !comparable ) {
      line( "struct {}{} {{}};", nodiscard_str, strukt.name );
    } else {
      line( "struct {}{} {{", nodiscard_str, strukt.name );
      int max_type_len =
          max_of( strukt.members, L( _.type.size() ), 0 );
      int max_var_len =
          max_of( strukt.members, L( _.var.size() ), 0 );
      {
        auto _ = indent();
        for( expr::StructMember const& member : strukt.members )
          line( "{: <{}} {: <{}} = {{}};", member.type,
                max_type_len, member.var, max_var_len );
        if( comparable ) {
          if( has_members ) newline();
          line( "bool operator==( {} const& ) const = default;",
                strukt.name );
        }
        if( item_has_feature( strukt,
                              expr::e_feature::validation ) ) {
          newline();
          comment( "Validates invariants among members." );
          comment( "defined in some translation unit." );
          line(
              "base::valid_or<std::string> validate() const;" );
        }
      }
      line( "};" );
    }
    newline();
    close_ns( ns );
    // Emit the reflection traits.
    newline();
    open_ns( "refl" );
    emit_reflection_for_struct(
        ns, strukt.tmpl_params, strukt.name, strukt.members,
        item_has_feature( strukt, expr::e_feature::offsets ) );
    newline();
    close_ns( "refl" );
  }

  void emit( string_view ns, expr::Sumtype const& sumtype ) {
    section( "Sum Type: "s + sumtype.name );
    open_ns( ns );
    if( !sumtype.alternatives.empty() ) {
      open_ns( sumtype.name );
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        bool emit_equality = item_has_feature(
            sumtype, expr::e_feature::equality );
        bool emit_validation = item_has_feature(
            sumtype, expr::e_feature::validation );
        emit_sumtype_alternative( sumtype.tmpl_params, alt,
                                  emit_equality,
                                  emit_validation );
        newline();
      }
      emit_enum_for_sumtype( sumtype.alternatives );
      newline();
      close_ns( sumtype.name );
      newline();
    }
    emit_template_decl( sumtype.tmpl_params );
    if( sumtype.alternatives.empty() ) {
      line( "using {}_t = std::monostate;", sumtype.name );
    } else {
      line( "using {}_t = base::variant<", sumtype.name );
      vector<string> variants;
      for( expr::Alternative const& alt : sumtype.alternatives )
        variants.push_back(
            "  "s + sumtype.name + "::" + alt.name +
            template_params( sumtype.tmpl_params,
                             /*put_typename=*/false ) );
      emit_vert_list( variants, "," );
      line( ">;" );
      // Ensure that the variant is nothrow move'able since this
      // makes code more efficient that uses it.
      line( "NOTHROW_MOVE( {}_t{} );", sumtype.name,
            all_int_tmpl_params( sumtype.tmpl_params.size() ) );
    }
    newline();
    close_ns( ns );
    // Global namespace.
    emit_variant_to_enum_specialization( ns, sumtype );
    // Emit the reflection traits.
    if( !sumtype.alternatives.empty() ) {
      newline();
      comment( "Reflection traits for alternatives." );
      open_ns( "refl" );
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        string sumtype_ns =
            fmt::format( "{}::{}", ns, sumtype.name );
        emit_reflection_for_struct(
            sumtype_ns, sumtype.tmpl_params, alt.name,
            alt.members,
            item_has_feature( sumtype,
                              expr::e_feature::offsets ) );
        newline();
      }
      close_ns( "refl" );
    }
  }

  void emit( string_view ns, expr::Config const& config ) {
    string_view name = config.name;
    section( "Config: "s + config.name );
    open_ns( ns );
    open_ns( "detail" );
    line( "inline config_{}_t __config_{} = {{}};", name, name );
    newline();
    close_ns( "detail" );
    newline();
    line(
        "inline config_{}_t const& config_{} = "
        "detail::__config_{};",
        name, name, name );
    newline();
    close_ns( ns );
    newline();
    open_ns( "rds::detail" );
    line(
        "inline auto __config_{}_registration = "
        "register_config( \"{}\", &{}::detail::__config_{} );",
        name, name, ns, name );
    newline();
    close_ns( "rds::detail" );
  }

  void emit_item( expr::Item const& item ) {
    string cpp_ns =
        base::str_replace_all( item.ns, { { ".", "::" } } );
    auto visitor = [&]( auto const& v ) { emit( cpp_ns, v ); };
    for( expr::Construct const& construct : item.constructs ) {
      newline();
      base::visit( visitor, construct );
    }
  }

  void emit_preamble() {
    line( "#pragma once" );
    newline();
  }

  template<typename T>
  bool rds_has_construct( expr::Rds const& rds ) {
    for( expr::Item const& item : rds.items ) {
      for( expr::Construct const& construct : item.constructs ) {
        bool has_construct = base::visit(
            mp::overload{ [&]( T const& ) { return true; },
                          []( auto const& ) { return false; } },
            construct );
        if( has_construct ) return true;
      }
    }
    return false;
  }

  void emit_includes( expr::Rds const& rds ) {
    section( "Includes" );
    if( !rds.includes.empty() ) {
      comment( "Includes specified in rds file." );
      for( string const& include : rds.includes )
        line( "#include {}", include );
      newline();
    }

    comment( "Revolution Now" );
    line( "#include \"core-config.hpp\"" );
    line( "" );
    comment( "refl" );
    line( "#include \"refl/ext.hpp\"" );
    line( "" );
    if( rds_has_construct<expr::Sumtype>( rds ) ) {
      comment( "base" );
      line( "#include \"base/variant.hpp\"" );
    }

    if( rds_has_construct<expr::Config>( rds ) ) {
      comment( "Rds helpers." );
      line( "#include \"rds/config-helper.hpp\"" );
      line( "" );
    }

    // line( "" );
    // comment( "base-util" );
    // line( "#include \"base-util/mp.hpp\"" );
    line( "" );
    comment( "C++ standard library" );
    if( rds_has_construct<expr::Enum>( rds ) )
      line( "#include <array>" );
    line( "#include <string_view>" );
    if( rds_has_construct<expr::Struct>( rds ) )
      line( "#include <tuple>" );
    newline();
  }

  void emit_rds( expr::Rds const& rds ) {
    emit_preamble();
    emit_includes( rds );

    for( expr::Item const& item : rds.items ) emit_item( item );
  }
};

} // namespace

maybe<string> generate_code( expr::Rds const& rds ) {
  CodeGenerator gen;
  gen.emit_rds( rds );
  return gen.result();
}

} // namespace rds
