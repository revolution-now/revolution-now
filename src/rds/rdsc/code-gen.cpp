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
  string res;
  if( tmpls.empty() ) return res;
  vector<string> names;
  string tp_name = put_typename ? "typename " : "";
  for( expr::TemplateParam const& param : tmpls )
    names.push_back( tp_name + param.param );
  string sep = ",";
  if( space ) sep += ' ';
  res = "<";
  res += base::str_join( names, sep );
  res += ">";
  return res;
}

template<typename T>
bool item_has_feature( T const& item, expr::e_feature feature ) {
  return item.features.has_value() &&
         item.features->contains( feature );
}

string trim_trailing_spaces( string s ) {
  string_view sv      = s;
  auto last_non_space = sv.find_last_not_of( ' ' );
  if( last_non_space != string_view::npos ) {
    auto trim_start = last_non_space + 1;
    sv.remove_suffix( sv.size() - trim_start );
  }
  return string( sv );
}

struct CodeGenerator {
  struct Options {
    int indent_level      = 0;
    bool quotes           = false;
    bool trailing_slash   = false;
    bool drop_empty_lines = false;

    bool operator==( Options const& ) const = default;
  };

  ostringstream oss_;
  maybe<string> curr_line_;
  Options default_options_ = {};

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

  AutoPopper drop_empty_lines() {
    push( options() );
    options().drop_empty_lines = true;
    return AutoPopper( *this );
  }

  AutoPopper enable_trailing_slashes() {
    push( options() );
    options().trailing_slash = true;
    return AutoPopper( *this );
  }

  AutoPopper disable_trailing_slashes() {
    push( options() );
    options().trailing_slash = false;
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
    if( options().drop_empty_lines && to_print.empty() ) return;
    // Only print empty strings if they are to be quoted.
    if( options().quotes )
      oss_ << indent << std::quoted( to_print );
    else if( !to_print.empty() )
      oss_ << indent << to_print;
    if( options().trailing_slash ) oss_ << " \\";
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
                       string_view sep ) {
    int count = lines.size();
    for( string const& l : lines ) {
      if( count-- == 1 ) sep = "";
      line( "{}{}", l, sep );
    }
  }

  void emit_vert_list_frag( vector<string> const& lines,
                            string_view sep ) {
    int count = lines.size();
    for( string const& l : lines ) {
      --count;
      bool const last = ( count == 0 );
      if( last ) sep = "";
      frag( "{}{}", l, sep );
      if( !last ) flush();
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

  void close_ns_no_flush( string_view ns,
                          string_view leaf = "" ) {
    pop();
    frag( "}" );
    if( !options().trailing_slash )
      frag( " // namespace {}", ns );
    if( !leaf.empty() ) frag( "::{}", leaf );
  }

  void close_ns( string_view ns, string_view leaf = "" ) {
    close_ns_no_flush( ns, leaf );
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
        int max_type_len =
            max_of( alt.members, L( _.type.size() ), 0 );
        for( expr::StructMember const& alt_mem : alt.members )
          line( "{: <{}} {} = {{}};", alt_mem.type, max_type_len,
                alt_mem.var );
        if( emit_equality ) {
          comment( "{}",
                   "This requires that the types of the member "
                   "variables " );
          comment( "{}", "also support equality." );
          // We need the 'struct' keyword in front of the alter-
          // native name to disambiguate in cases where there is
          // an alternative member with the same name as the al-
          // ternative.
          line(
              "bool operator==( struct {} const& ) const = "
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
    string const qualified_base_name =
        fmt::format( "{}::detail::{}Base{}", ns, sumtype.name,
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
          qualified_base_name );
    string const typen4me =
        sumtype.tmpl_params.empty() ? "" : "typename ";
    {
      auto _ = indent();
      line( "using type = {}{}::{}{}::e;", typen4me, ns,
            sumtype.name,
            template_params( sumtype.tmpl_params,
                             /*put_typename=*/false ) );
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
      string_view ns, string_view ns_display,
      vector<expr::TemplateParam> const& tmpl_params,
      string const& name,
      vector<expr::StructMember> const& members,
      bool wants_offsets, bool is_sumtype_alternative ) {
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
            ns_display );
      line( "static constexpr std::string_view name = \"{}\";",
            name );
      line( "static constexpr bool is_sumtype_alternative = {};",
            is_sumtype_alternative );
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
    bool has_members = !strukt.members.empty();
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
        ns, ns, strukt.tmpl_params, strukt.name, strukt.members,
        item_has_feature( strukt, expr::e_feature::offsets ),
        /*is_sumtype_alternative=*/false );
    newline();
    close_ns( "refl" );
  }

  void emit( string_view ns, expr::Sumtype const& sumtype ) {
    section( "Sum Type: "s + sumtype.name );
    open_ns( ns );
    open_ns( "detail" );
    bool const emit_equality =
        item_has_feature( sumtype, expr::e_feature::equality );
    bool const emit_validation =
        item_has_feature( sumtype, expr::e_feature::validation );
    string const nodiscard_str =
        item_has_feature( sumtype, expr::e_feature::nodiscard )
            ? "[[nodiscard]] "
            : "";
    string const alt_ns =
        fmt::format( "{}_alternatives", sumtype.name );
    if( !sumtype.alternatives.empty() ) {
      open_ns( alt_ns );
      for( expr::Alternative const& alt :
           sumtype.alternatives ) {
        emit_sumtype_alternative( sumtype.tmpl_params, alt,
                                  emit_equality,
                                  emit_validation );
        newline();
      }
      close_ns( alt_ns );
      newline();
    }
    emit_template_decl( sumtype.tmpl_params );
    line( "using {}Base = base::variant<", sumtype.name );
    vector<string> variants;
    for( expr::Alternative const& alt : sumtype.alternatives )
      variants.push_back(
          "  detail::"s + alt_ns + "::" + alt.name +
          template_params( sumtype.tmpl_params,
                           /*put_typename=*/false ) );
    emit_vert_list( variants, "," );
    line( ">;" );
    newline();
    close_ns( "detail" );
    newline();
    emit_template_decl( sumtype.tmpl_params );
    string const base_name =
        fmt::format( "detail::{}Base{}", sumtype.name,
                     template_params( sumtype.tmpl_params,
                                      /*put_typename=*/false ) );
    line( "struct {}{} : public {} {{", nodiscard_str,
          sumtype.name, base_name );
    {
      auto _ = indent();

      int const max_type_len =
          max_of( sumtype.alternatives, L( _.name.size() ), 0 );
      for( expr::Alternative const& alt : sumtype.alternatives )
        line( "using {: <{}} = detail::{}::{}{};", alt.name,
              max_type_len, alt_ns, alt.name,
              template_params( sumtype.tmpl_params,
                               /*put_typename=*/false ) );
      if( !sumtype.alternatives.empty() ) newline();

      emit_enum_for_sumtype( sumtype.alternatives );
      newline();

      line( "using i_am_rds_variant = void;" );
      line( "using Base = {};", base_name );
      line( "using Base::Base;" );
      line( "{}( Base&& b ) : Base( std::move( b ) ) {{}}",
            sumtype.name );
      line( "Base const& as_base() const& { return *this; }" );
      line( "Base&       as_base()      & { return *this; }" );

      // Emit operator==.
      if( emit_equality ) {
        newline();
        // These allow us to compare with an alternative object
        // directly, which std::variant does not allow natively
        // to avoid the ambiguity when two alternative types are
        // same, a problem that we don't have here.
        comment( "Comparison with alternatives." );
        for( expr::Alternative const& alt :
             sumtype.alternatives ) {
          line( "bool operator==( {} const& rhs ) const {{",
                alt.name );
          {
            auto _ = indent();
            line(
                "return this->template holds<{}>() && "
                "(this->template get<{}>() == rhs);",
                alt.name, alt.name );
          }
          line( "}" );
        }
      }
    }
    line( "};" );
    newline();
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
        string const sumtype_ns =
            fmt::format( "{}::detail::{}", ns, alt_ns );
        string const sumtype_ns_display =
            fmt::format( "{}::{}", ns, sumtype.name );
        emit_reflection_for_struct(
            sumtype_ns, sumtype_ns_display, sumtype.tmpl_params,
            alt.name, alt.members,
            item_has_feature( sumtype,
                              expr::e_feature::offsets ),
            /*is_sumtype_alternative=*/true );
        newline();
      }
      close_ns( "refl" );
    }
  }

  void emit( string_view ns, expr::Interface const& interface ) {
    section( "Interface: "s + interface.name );

    std::string const no_i_name =
        ( interface.name.size() > 1 &&
          interface.name[0] == 'I' && interface.name[1] >= 'A' &&
          interface.name[1] <= 'Z' )
            ? interface.name.substr( 1 )
            : interface.name;
    open_ns( ns );

    // Emits arguments in a function/method declaration verti-
    // cally.
    auto emit_arg_list_with_types =
        [&]( vector<expr::MethodArg> const& args ) {
          int const max_type_len =
              max_of( args, L( _.type.size() ), 0 );
          vector<string> arg_lines;
          for( auto& arg : args )
            arg_lines.push_back(
                fmt::format( "{: <{}} {}", arg.type,
                             max_type_len, arg.var ) );
          emit_vert_list_frag( arg_lines, "," );
        };

    // Emits the context member variables.
    auto emit_member_list_with_types =
        [&]( vector<expr::MethodArg> const& args ) {
          int const max_type_len =
              max_of( args, L( _.type.size() ), 0 );
          vector<string> arg_lines;
          for( auto& arg : args )
            arg_lines.push_back(
                fmt::format( "{: <{}} {}_", arg.type,
                             max_type_len, arg.var ) );
          emit_vert_list_frag( arg_lines, ";" );
          frag( ";" );
          flush();
        };

    // Emits initializers in a constructor after the ':' verti-
    // cally.
    auto emit_init_list_with_types =
        [&]( vector<expr::MethodArg> const& args ) {
          vector<string> arg_lines;
          string init = ": ";
          for( auto& arg : args ) {
            arg_lines.push_back( fmt::format(
                "{}{}_( {} )", init, arg.var, arg.var ) );
            init = "  ";
          }
          emit_vert_list_frag( arg_lines, "," );
        };

    // Generate the main interface struct.
    line( "struct {} {{", interface.name );
    {
      auto _ = indent();
      line( "virtual ~{}() = default;", interface.name );
      for( expr::Method const& mth : interface.methods ) {
        newline();
        frag( "virtual {} {}(", mth.return_type, mth.name );
        if( mth.args.empty() ) {
          frag( ") const = 0;" );
          flush();
        } else {
          flush();
          auto _ = indent( 2 );
          emit_arg_list_with_types( mth.args );
          frag( " ) const = 0;" );
          flush();
        }
      }
    }
    line( "};" );
    newline();

    // Generate the implementation struct.
    auto real_name = "Real" + no_i_name;
    frag( "struct {} final : public {} {{", real_name,
          interface.name );
    if( interface.methods.empty() &&
        interface.context.members.empty() )
      frag( "};" );
    flush();
    {
      auto _ = indent();
      // Emit constructor.
      if( !interface.context.members.empty() ) {
        line( "{}(", real_name );
        {
          auto _ = indent( 2 );
          emit_arg_list_with_types( interface.context.members );
          frag( " )" );
          flush();
        }
        {
          auto _ = indent();
          emit_init_list_with_types( interface.context.members );
          frag( " {}" );
          flush();
        }
      }

      // Emit methods.
      bool emit_newline_before_method =
          !interface.context.members.empty();
      for( expr::Method const& mth : interface.methods ) {
        if( emit_newline_before_method ) newline();
        emit_newline_before_method = true;
        frag( "{} {}(", mth.return_type, mth.name );
        if( mth.args.empty() ) {
          frag( ") const override {" );
          flush();
        } else {
          flush();
          auto _ = indent( 2 );
          emit_arg_list_with_types( mth.args );
          frag( " ) const override {" );
          flush();
        }
        {
          auto _ = indent();
          frag( "return ::{}::{}(", ns, mth.name );
          if( mth.args.empty() &&
              interface.context.members.empty() ) {
            frag( ");" );
            flush();
          } else {
            flush();
            auto _ = indent();
            vector<string> vars;
            for( expr::MethodArg const& arg :
                 interface.context.members )
              vars.push_back( arg.var + "_" );
            for( expr::MethodArg const& arg : mth.args )
              vars.push_back( arg.var );
            emit_vert_list_frag( vars, "," );
            frag( " );" );
            flush();
          }
        }
        line( "}" );
      }
    }

    // Emit the data members held by the implementation struct.
    if( !interface.context.members.empty() ) {
      newline();
      line( " private:" );
      auto _ = indent();
      emit_member_list_with_types( interface.context.members );
    }

    if( !interface.methods.empty() ||
        !interface.context.members.empty() )
      line( "};" );

    newline();
    close_ns( ns );

    // Emit the mock struct. We emit it within a macro so that we
    // don't have to include the mocking headers in the generated
    // files, otherwise normal code would be pulling them in.
    newline();
    string const mock_name = "Mock" + interface.name;
    comment( mock_name );
    {
      auto _  = enable_trailing_slashes();
      auto __ = drop_empty_lines();
      line( "#define RDS_DEFINE_MOCK_{}()", interface.name );
      {
        auto _ = indent();
        open_ns( ns );
        frag( "struct {} : public {} {{", mock_name,
              interface.name );
        if( interface.methods.empty() ) {
          frag( "};" );
          flush();
        } else {
          flush();
          auto _ = indent();
          for( expr::Method const& mth : interface.methods ) {
            frag( "MOCK_METHOD( {}, {}, (", mth.return_type,
                  mth.name );
            if( mth.args.empty() ) {
              frag( "), ( const ) );" );
              flush();
            } else {
              flush();
              auto _ = indent();
              vector<string> arg_names;
              for( expr::MethodArg const& arg : mth.args )
                arg_names.push_back( arg.type );
              emit_vert_list( arg_names, "," );
              line( "), ( const ) );" );
            }
          }
        }
        if( !interface.methods.empty() ) line( "};" );
        newline();
        close_ns_no_flush( ns );
        {
          auto _ = disable_trailing_slashes();
          flush();
        }
      }
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
    open_ns( "rds" );
    line(
        "detail::empty_registrar register_config( "
        "{}::config_{}_t* global );",
        ns, name );
    newline();
    open_ns( "detail" );
    comment(
        " This ensures that if anyone includes the header for "
        "this" );
    comment(
        " config file then it is guaranteed to be registered "
        "and" );
    comment( " populated." );
    line(
        "inline auto __config_{}_registration = "
        "register_config( &{}::detail::__config_{} );",
        name, ns, name );
    newline();
    close_ns( "detail" );
    newline();
    close_ns( "rds" );
  }

  void emit_item( expr::Item const& item ) {
    string cpp_ns =
        base::str_replace_all( item.ns, { { ".", "::" } } );
    auto visitor = [&]( auto const& v ) { emit( cpp_ns, v ); };
    for( expr::Construct const& construct : item.constructs ) {
      base::visit( visitor, construct );
      newline();
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

    // Note that expr::Interface is not in this list.
    bool const needs_refl_ext =
        rds_has_construct<expr::Sumtype>( rds ) ||
        rds_has_construct<expr::Enum>( rds ) ||
        rds_has_construct<expr::Struct>( rds ) ||
        rds_has_construct<expr::Config>( rds );
    // This works at the moment.
    bool const needs_string_view = needs_refl_ext;

    if( needs_refl_ext ) {
      comment( "refl" );
      line( "#include \"refl/ext.hpp\"" );
      newline();
    }

    if( rds_has_construct<expr::Sumtype>( rds ) ) {
      comment( "base" );
      line( "#include \"base/variant.hpp\"" );
      newline();
    }

    if( rds_has_construct<expr::Config>( rds ) ) {
      comment( "Rds helpers." );
      line( "#include \"rds/config-helper.hpp\"" );
      newline();
    }

    bool const needs_cpp_std_lib =
        rds_has_construct<expr::Enum>( rds ) ||
        rds_has_construct<expr::Struct>( rds ) ||
        needs_string_view;

    if( needs_cpp_std_lib ) {
      comment( "C++ standard library" );
      if( rds_has_construct<expr::Enum>( rds ) )
        line( "#include <array>" );
      if( needs_string_view ) line( "#include <string_view>" );
      if( rds_has_construct<expr::Struct>( rds ) )
        line( "#include <tuple>" );
      newline();
    }
  }

  void emit_rds( expr::Rds const& rds ) {
    emit_preamble();
    emit_includes( rds );

    for( expr::Item const& item : rds.items ) emit_item( item );

    comment( "The end." );
  }
};

} // namespace

maybe<string> generate_code( expr::Rds const& rds ) {
  CodeGenerator gen;
  gen.emit_rds( rds );
  return gen.result();
}

} // namespace rds
