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

// base
#include "base/lambda.hpp"
#include "base/maybe.hpp"
#include "base/meta.hpp"

// {fmt}
#include "fmt/format.h"

// Abseil
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

// c++ standard library
#include <iomanip>
#include <sstream>
#include <stack>

using namespace std;

namespace rnl {

namespace {

using ::base::maybe;

// Parameters:
//   - member_var_name
constexpr string_view kSumtypeAlternativeMemberSerial = R"xyz(
    auto s_{member_var_name} = serialize<::rn::serial::fb_serialize_hint_t<
        decltype( std::declval<fb_target_t>().{member_var_name}() )>>(
        builder, {member_var_name}, ::rn::serial::ADL{{}} );
)xyz";

// Parameters:
//   - member_var_name
constexpr string_view kSumtypeAlternativeMemberDeserial = R"xyz(
    HAS_VALUE_OR_RET( deserialize(
        ::rn::serial::detail::to_const_ptr( src.{member_var_name}() ),
        &dst->{member_var_name}, ::rn::serial::ADL{{}} ) );
)xyz";

// Parameters:
//   - sumtype_name
//   - alt_name
//   - members_serialization
//   - members_deserialization
//   - members_s_get:
//       Vertical comma-separated list of "s_<member>.get()"
constexpr string_view kSumtypeAlternativeSerial = R"xyz(
  using fb_target_t = fb::{sumtype_name}::{alt_name};

  rn::serial::FBOffset<fb::{sumtype_name}::{alt_name}> serialize_table(
      rn::serial::FBBuilder& builder ) const {{
    using ::rn::serial::serialize;
    {members_serialization}
    // We must always serialize this table even if it is
    // empty/default-valued because, for variants, its presence
    // indicates that it is the active alternative.
    return fb::{sumtype_name}::Create{alt_name}( builder
        {members_s_get}
    );
  }}

  static ::rn::valid_deserial_t deserialize_table(
      fb::{sumtype_name}::{alt_name} const& src,
      {alt_name}* dst ) {{
    (void)src;
    (void)dst;
    DCHECK( dst );
    using ::rn::serial::deserialize;
    {members_deserialization}
    return ::rn::valid;
  }}

  ::rn::valid_deserial_t check_invariants_safe() const {{
    return ::rn::valid;
  }}
)xyz";

void remove_common_space_prefix( vector<string>* lines ) {
  if( lines->empty() ) return;
  size_t min_spaces = 10000000;
  for( string_view sv : *lines ) {
    size_t first = sv.find_first_not_of( ' ' );
    if( first == string_view::npos ) continue;
    min_spaces = std::min( first, min_spaces );
  }
  for( string& s : *lines ) {
    if( string_view( s ).find_first_not_of( ' ' ) ==
        string_view::npos )
      // Either empty or just spaces.
      continue;
    string new_s( s.begin() + min_spaces, s.end() );
    s = std::move( new_s );
  }
}

template<typename Range, typename Projection, typename Default>
auto max_of( Range&& rng, Projection&& proj, Default value )
    -> invoke_result_t<Projection,
                       typename decay_t<Range>::value_type> {
  if( rng.empty() ) return value;
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

string all_int_tmpl_params( int count ) {
  vector<expr::TemplateParam> params(
      count, expr::TemplateParam{ "int" } );
  return template_params( params, /*put_typename=*/false,
                          /*space=*/true );
}

string template_params_type_names(
    vector<expr::TemplateParam> const& tmpls ) {
  string params = template_params( tmpls, /*put_typename=*/false,
                                   /*space=*/true );
  return "::base::type_list_to_names"s + params + "()";
}

bool sumtype_has_feature( expr::Sumtype const&    sumtype,
                          expr::e_sumtype_feature feature ) {
  if( !sumtype.features.has_value() ) return false;
  for( auto type : *sumtype.features ) {
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

    bool operator==( Options const& ) const = default;
    bool operator!=( Options const& ) const = default;
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
  template<typename Arg1, typename... Args>
  void line( string_view fmt_str, Arg1&& arg1, Args&&... args ) {
    assert( !curr_line_.has_value() );
    assert( fmt_str.find_first_of( "\n" ) == string_view::npos );
    string indent( options().indent_level * 2, ' ' );
    string to_print = trim_trailing_spaces(
        fmt::format( fmt_str, forward<Arg1>( arg1 ),
                     forward<Args>( args )... ) );
    // Only print empty strings if they are to be quoted.
    if( options().quotes )
      oss_ << indent << std::quoted( to_print );
    else if( !to_print.empty() )
      oss_ << indent << to_print;
    oss_ << "\n";
  }

  // Braces {} do NOT have to be escaped for this one.
  void line( string_view l ) { line( "{}", l ); }

  template<typename... Args>
  void frag( string_view fmt_str, Args&&... args ) {
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
    line( to_write );
  }

  void newline() { line( "" ); }

  template<typename... Args>
  void comment( string_view fmt_str, Args&&... args ) {
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

  template<typename... Args>
  void emit_code_block( string_view fmt_str, Args&&... args ) {
    string formatted =
        fmt::format( fmt_str, forward<Args>( args )... );
    vector<string> lines = absl::StrSplit( formatted, "\n" );
    remove_common_space_prefix( &lines );
    if( lines.empty() ) return;
    int i = 0;
    // Remove the first line if it's empty.
    if( lines[0].empty() ) i = 1;
    for( ; i < int( lines.size() ); ++i ) line( lines[i] );
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
      line( "}}" );
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
    line( "}" );
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
      line( ": formatter_base {" );
      emit_fmt_format_method( alt, full_alt_name, tmpls,
                              sumtype_name );
    }
    line( "};" );
  }

  void emit( vector<expr::TemplateParam> const& tmpls,
             expr::Alternative const&           alt,
             string_view sumtype_name, bool emit_equality,
             bool emit_serialization ) {
    emit_template_decl( tmpls );
    if( alt.members.empty() && !emit_equality &&
        !emit_serialization ) {
      line( "struct {} {{}};", alt.name );
    } else {
      line( "struct {} {{", alt.name );
      {
        auto cleanup = indent();
        int  max_type_len =
            max_of( alt.members, L( _.type.size() ), 0 );
        for( expr::AlternativeMember const& alt_mem :
             alt.members )
          line( "{: <{}} {};", alt_mem.type, max_type_len,
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
        if( emit_serialization ) {
          string member_serials;
          string member_deserials;
          string members_s_get;
          for( expr::AlternativeMember const& alt_mem :
               alt.members ) {
            member_serials += fmt::format(
                kSumtypeAlternativeMemberSerial,
                fmt::arg( "member_var_name", alt_mem.var ) );
            member_deserials += fmt::format(
                kSumtypeAlternativeMemberDeserial,
                fmt::arg( "member_var_name", alt_mem.var ) );
            members_s_get +=
                fmt::format( ", s_{}.get()", alt_mem.var );
          }

          emit_code_block(
              kSumtypeAlternativeSerial,
              fmt::arg( "sumtype_name", sumtype_name ),
              fmt::arg( "alt_name", alt.name ),
              fmt::arg( "members_serialization",
                        member_serials ),
              fmt::arg( "members_deserialization",
                        member_deserials ),
              fmt::arg( "members_s_get", members_s_get ) );
        }
      }
      line( "};" );
    }
  }

  void emit_enum_for_sumtype(
      vector<expr::Alternative> const& alternatives ) {
    assert( !alternatives.empty() );
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
    line( "enum class {} {{", e.name );
    {
      auto _ = indent();
      emit_vert_list( e.values, "," );
    }
    line( "};" );
    // Emit the traits.
    newline();
    close_ns( ns );
    newline();
    open_ns( "rn" );
    comment( "Reflection info for enum {}.", e.name );
    line( "template<>" );
    line( "struct enum_traits<{}::{}> {{", ns, e.name );
    {
      auto _ = indent();
      line( "using type = {}::{};", ns, e.name );
      line( "static constexpr int count = {};",
            e.values.size() );
      line(
          "static constexpr std::string_view type_name = "
          "\"{}\";",
          e.name );
      line( "static constexpr std::array<type, {}> values{{",
            e.values.size() );
      {
        auto           _       = indent();
        vector<string> with_ns = e.values;
        for( string& s : with_ns ) s = "type::" + s;
        emit_vert_list( with_ns, "," );
      }
      line( "};" );
      if( !e.values.empty() ) {
        line(
            "static constexpr std::string_view value_name( type "
            "val ) {" );
        {
          auto _ = indent();
          line( "switch( val ) {" );
          {
            auto _ = indent();
            for( string const& s : e.values )
              line( "case type::{}: return \"{}\";", s, s );
          }
          line( "}" );
        }
        line( "}" );
      }
      line( "template<typename Int>" );
      line(
          "static constexpr maybe<type> from_integral( Int {}) "
          "{{",
          e.values.empty() ? "" : "val " );
      {
        auto _ = indent();
        line( "maybe<type> res;" );
        if( !e.values.empty() ) {
          line( "int intval = static_cast<int>( val );" );
          line( "if( intval < 0 || intval >= {} ) return res;",
                e.values.size() );
          line( "res = static_cast<type>( intval );" );
        }
        line( "return res;" );
      }
      line( "}" );
      line(
          "static constexpr maybe<type> from_string( "
          "std::string_view {}) {{",
          e.values.empty() ? "" : "name " );
      {
        auto _ = indent();
        line( "return" );
        {
          auto _ = indent();
          for( string const& val : e.values )
            line( "name == \"{}\" ? maybe<type>( type::{} ) :",
                  val, val );
          line( "maybe<type>{};" );
        }
      }
      line( "}" );
    }
    line( "};" );
    newline();
    close_ns( "rn" );
  }

  void emit( string_view ns, expr::Sumtype const& sumtype ) {
    section( "Sum Type: "s + sumtype.name );
    open_ns( ns );
    if( !sumtype.alternatives.empty() ) {
      open_ns( sumtype.name );
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        bool emit_equality = sumtype_has_feature(
            sumtype, expr::e_sumtype_feature::equality );
        bool emit_serialization = sumtype_has_feature(
            sumtype, expr::e_sumtype_feature::serializable );
        emit( sumtype.tmpl_params, alt, sumtype.name,
              emit_equality, emit_serialization );
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
        variants.push_back( absl::StrCat(
            "  ", sumtype.name, "::", alt.name,
            template_params( sumtype.tmpl_params,
                             /*put_typename=*/false ) ) );
      emit_vert_list( variants, "," );
      line( ">;" );
      // Ensure that the variant is nothrow move'able since this
      // makes code more efficient that uses it.
      line( "NOTHROW_MOVE( {}_t{} );", sumtype.name,
            all_int_tmpl_params( sumtype.tmpl_params.size() ) );
    }
    newline();
    close_ns( ns );
    emit_variant_to_enum_specialization( ns, sumtype );
    // Global namespace.
    if( sumtype_has_feature(
            sumtype, expr::e_sumtype_feature::formattable ) ) {
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

  bool rnl_has_sumtype_feature(
      expr::Rnl const&        rnl,
      expr::e_sumtype_feature target_feature ) {
    for( expr::Item const& item : rnl.items ) {
      for( expr::Construct const& construct : item.constructs ) {
        bool has_feature = visit(
            mp::overload{ [&]( expr::Sumtype const& sumtype ) {
                           return sumtype_has_feature(
                               sumtype, target_feature );
                         },
                          []( auto const& ) { return false; } },
            construct );
        if( has_feature ) return true;
      }
    }
    return false;
  }

  bool rnl_has_sumtype( expr::Rnl const& rnl ) {
    for( expr::Item const& item : rnl.items ) {
      for( expr::Construct const& construct : item.constructs ) {
        bool has_sumtype = visit(
            mp::overload{
                [&]( expr::Sumtype const& ) { return true; },
                []( auto const& ) { return false; } },
            construct );
        if( has_sumtype ) return true;
      }
    }
    return false;
  }

  bool rnl_has_enum( expr::Rnl const& rnl ) {
    for( expr::Item const& item : rnl.items ) {
      for( expr::Construct const& construct : item.constructs ) {
        bool has_enum =
            visit( mp::overload{
                       [&]( expr::Enum const& ) { return true; },
                       []( auto const& ) { return false; } },
                   construct );
        if( has_enum ) return true;
      }
    }
    return false;
  }

  bool rnl_needs_serial_header( expr::Rnl const& rnl ) {
    return rnl_has_sumtype_feature(
        rnl, expr::e_sumtype_feature::serializable );
  }

  bool rnl_needs_fmt_headers( expr::Rnl const& rnl ) {
    return rnl_has_sumtype_feature(
        rnl, expr::e_sumtype_feature::formattable );
  }

  void emit_includes( expr::Rnl const& rnl ) {
    section( "Includes" );
    if( !rnl.includes.empty() ) {
      comment( "Includes specified in rnl file." );
      for( string const& include : rnl.includes )
        line( "#include {}", include );
      newline();
    }

    comment( "Revolution Now" );
    line( "#include \"core-config.hpp\"" );
    if( rnl_has_sumtype( rnl ) )
      line( "#include \"rnl/helper/sumtype-helper.hpp\"" );
    if( rnl_has_enum( rnl ) )
      line( "#include \"rnl/helper/enum.hpp\"" );
    if( rnl_needs_fmt_headers( rnl ) )
      line( "#include \"fmt-helper.hpp\"" );
    if( rnl_needs_serial_header( rnl ) ) {
      line( "#include \"error.hpp\"" );
      line( "#include \"fb.hpp\"" );
    }
    if( rnl_has_enum( rnl ) ) line( "#include \"maybe.hpp\"" );
    line( "" );
    comment( "base" );
    line( "#include \"base/cc-specific.hpp\"" );
    line( "#include \"base/variant.hpp\"" );
    line( "" );
    comment( "base-util" );
    line( "#include \"base-util/mp.hpp\"" );
    if( rnl_needs_fmt_headers( rnl ) ) {
      line( "" );
      comment( "{{fmt}}" );
      line( "#include \"fmt/format.h\"" );
    }
    line( "" );
    comment( "C++ standard library" );
    if( rnl_has_enum( rnl ) ) line( "#include <array>" );
    line( "#include <string_view>" );
    newline();
  }

  void emit_metadata( expr::Rnl const& rnl ) {
    section( "Global Vars" );
    string stem_to_var = absl::StrReplaceAll(
        rnl.meta.module_name, { { "-", "_" } } );
    open_ns( "rn" );
    comment(
        "This will be the naem of this header, not the file "
        "that it" );
    comment( "is include in." );
    line(
        "inline constexpr std::string_view rnl_{}_genfile = "
        "__FILE__;",
        stem_to_var );
    newline();
    close_ns( "rn" );
  }

  void emit_rnl( expr::Rnl const& rnl ) {
    emit_preamble();
    emit_imports( rnl.imports );
    emit_includes( rnl );
    emit_metadata( rnl );

    for( expr::Item const& item : rnl.items ) emit( item );
  }
};

} // namespace

maybe<string> generate_code( expr::Rnl const& rnl ) {
  CodeGenerator gen;
  gen.emit_rnl( rnl );
  return gen.result();
}

} // namespace rnl
