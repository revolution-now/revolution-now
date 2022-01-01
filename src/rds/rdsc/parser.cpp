/****************************************************************
**rds-parser.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-21.
*
* Description: RDS Parser.
*
*****************************************************************/
#include "parser.hpp"

// rcl
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// base
#include "base/error.hpp"
#include "base/fs.hpp"
#include "base/maybe.hpp"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rds {

namespace {

using ::base::maybe;

constexpr string_view kTemplateKey  = "_template";
constexpr string_view kFeaturesKey  = "_features";
constexpr string_view kSumtypeKey   = "sumtype";
constexpr string_view kStructKey    = "struct";
constexpr string_view kEnumKey      = "enum";
constexpr string_view kNamespaceKey = "namespace";
constexpr string_view kIncludeKey   = "include";

bool reserved_name( string_view sv ) {
  return false                  //
         || sv == kTemplateKey  //
         || sv == kFeaturesKey  //
         || sv == kSumtypeKey   //
         || sv == kStructKey    //
         || sv == kEnumKey      //
         || sv == kNamespaceKey //
         || sv == kIncludeKey;  //
}

void parse_enum( vector<string> const& parent_namespaces,
                 string_view name, rcl::list const& enum_names,
                 expr::Rds& rds ) {
  expr::Item item;
  item.ns =
      fmt::format( "{}", fmt::join( parent_namespaces, "." ) );

  expr::Enum enum_;
  enum_.name = name;
  for( rcl::value const& v : enum_names ) {
    UNWRAP_CHECK_MSG( e, v.get_if<string>(),
                      "enum values must be strings." );
    enum_.values.push_back( e );
  }

  item.constructs.push_back( enum_ );

  rds.items.push_back( item );
}

void parse_enums( vector<string> const& parent_namespaces,
                  rcl::table const& tbl, expr::Rds& rds ) {
  for( auto& [k, v] : tbl ) {
    CHECK( !reserved_name( k ),
           "expected enum type name but instead found reserved "
           "word {}.",
           k );
    UNWRAP_CHECK_MSG( l, v.get_if<unique_ptr<rcl::list>>(),
                      "value of enum key {} must be a list.",
                      k );
    parse_enum( parent_namespaces, k, *l, rds );
  }
}

void parse_sumtype( vector<string> const& parent_namespaces,
                    string_view name, rcl::table const& tbl,
                    expr::Rds& rds ) {
  expr::Item item;
  item.ns =
      fmt::format( "{}", fmt::join( parent_namespaces, "." ) );

  expr::Sumtype sumtype;
  sumtype.name = name;

  if( tbl.has_key( kTemplateKey ) ) {
    UNWRAP_CHECK_MSG(
        tmpl, tbl[kTemplateKey].get_if<unique_ptr<rcl::list>>(),
        "value of {} must be a list.", kTemplateKey );
    for( rcl::value const& v : *tmpl ) {
      UNWRAP_CHECK_MSG(
          t_arg, v.get_if<string>(),
          "template argument list must consist of strings." );
      expr::TemplateParam tmpl_param;
      tmpl_param.param = t_arg;
      sumtype.tmpl_params.push_back( tmpl_param );
    }
  }

  if( tbl.has_key( kFeaturesKey ) ) {
    sumtype.features.emplace();
    UNWRAP_CHECK_MSG(
        features,
        tbl[kFeaturesKey].get_if<unique_ptr<rcl::list>>(),
        "value of {} must be a list.", kFeaturesKey );
    for( rcl::value const& v : *features ) {
      UNWRAP_CHECK_MSG(
          feature_name, v.get_if<string>(),
          "features list must consist of strings." );
      maybe<expr::e_feature> feat =
          expr::feature_from_str( feature_name );
      CHECK( feat, "unknown feature name: {}", feature_name );
      sumtype.features->push_back( *feat );
    }
  }

  for( auto& [k, v] : tbl ) {
    if( k == kFeaturesKey || k == kTemplateKey ) continue;
    string            alt_name = k;
    expr::Alternative alt;
    alt.name = alt_name;

    UNWRAP_CHECK_MSG(
        alt_members, v.get_if<unique_ptr<rcl::table>>(),
        "value of sumtype alternative {} must be a table.", k );
    for( auto& [var_name, var_v] : *alt_members ) {
      UNWRAP_CHECK_MSG( var_type, var_v.get_if<string>(),
                        "type of variable {} must be a string.",
                        var_name );
      expr::AlternativeMember alt_member{ .type = var_type,
                                          .var  = var_name };
      alt.members.push_back( alt_member );
    }
    sumtype.alternatives.push_back( alt );
  }

  item.constructs.push_back( sumtype );
  rds.items.push_back( item );
}

void parse_struct( vector<string> const& parent_namespaces,
                   string_view name, rcl::table const& tbl,
                   expr::Rds& rds ) {
  expr::Item item;
  item.ns =
      fmt::format( "{}", fmt::join( parent_namespaces, "." ) );

  expr::Struct strukt;
  strukt.name = name;

  if( tbl.has_key( kTemplateKey ) ) {
    UNWRAP_CHECK_MSG(
        tmpl, tbl[kTemplateKey].get_if<unique_ptr<rcl::list>>(),
        "value of {} must be a list.", kTemplateKey );
    for( rcl::value const& v : *tmpl ) {
      UNWRAP_CHECK_MSG(
          t_arg, v.get_if<string>(),
          "template argument list must consist of strings." );
      expr::TemplateParam tmpl_param;
      tmpl_param.param = t_arg;
      strukt.tmpl_params.push_back( tmpl_param );
    }
  }

  if( tbl.has_key( kFeaturesKey ) ) {
    strukt.features.emplace();
    UNWRAP_CHECK_MSG(
        features,
        tbl[kFeaturesKey].get_if<unique_ptr<rcl::list>>(),
        "value of {} must be a list.", kFeaturesKey );
    for( rcl::value const& v : *features ) {
      UNWRAP_CHECK_MSG(
          feature_name, v.get_if<string>(),
          "features list must consist of strings." );
      maybe<expr::e_feature> feat =
          expr::feature_from_str( feature_name );
      CHECK( feat, "unknown feature name: {}", feature_name );
      strukt.features->push_back( *feat );
    }
  }

  for( auto& [k, v] : tbl ) {
    if( k == kFeaturesKey || k == kTemplateKey ) continue;
    string             member_name = k;
    expr::StructMember member;
    member.var = member_name;

    UNWRAP_CHECK_MSG(
        member_type, v.get_if<string>(),
        "value of struct member {} must be a string.", k );
    member.type = member_type;
    strukt.members.push_back( member );
  }

  item.constructs.push_back( strukt );
  rds.items.push_back( item );
}

void parse_sumtypes( vector<string> const& parent_namespaces,
                     rcl::table const& tbl, expr::Rds& rds ) {
  for( auto& [k, v] : tbl ) {
    CHECK( !reserved_name( k ),
           "expected sumtype name but instead found reserved "
           "word {}.",
           k );
    UNWRAP_CHECK_MSG( t, v.get_if<unique_ptr<rcl::table>>(),
                      "value of sumtype key {} must be a table.",
                      k );
    parse_sumtype( parent_namespaces, k, *t, rds );
  }
}

void parse_structs( vector<string> const& parent_namespaces,
                    rcl::table const& tbl, expr::Rds& rds ) {
  for( auto& [k, v] : tbl ) {
    CHECK( !reserved_name( k ),
           "expected struct name but instead found reserved "
           "word {}.",
           k );
    UNWRAP_CHECK_MSG( t, v.get_if<unique_ptr<rcl::table>>(),
                      "value of struct key {} must be a table.",
                      k );
    parse_struct( parent_namespaces, k, *t, rds );
  }
}

void parse_namespaces( vector<string> const& parent_namespaces,
                       rcl::table const& tbl, expr::Rds& rds );

void parse_namespace( vector<string> const& parent_namespaces,
                      string_view           ns_name,
                      rcl::table const&     ns_tbl,
                      expr::Rds&            rds ) {
  vector<string> namespaces = parent_namespaces;
  namespaces.push_back( string( ns_name ) );

  // Make sure we only have keywords.
  for( auto& [k, v] : ns_tbl ) {
    bool valid = ( k == kEnumKey ) || ( k == kSumtypeKey ) ||
                 ( k == kStructKey ) || ( k == kNamespaceKey );
    CHECK( valid, "invalid/unexpected keyword {}.", k );
  }

  // Enums.
  if( ns_tbl.has_key( kEnumKey ) ) {
    UNWRAP_CHECK_MSG(
        t, ns_tbl[kEnumKey].get_if<unique_ptr<rcl::table>>(),
        "value of key {} must be a table.", kEnumKey );
    parse_enums( namespaces, *t, rds );
  }

  // Sumtypes.
  if( ns_tbl.has_key( kSumtypeKey ) ) {
    UNWRAP_CHECK_MSG(
        t, ns_tbl[kSumtypeKey].get_if<unique_ptr<rcl::table>>(),
        "value of key {} must be a table.", kSumtypeKey );
    parse_sumtypes( namespaces, *t, rds );
  }

  // Structs.
  if( ns_tbl.has_key( kStructKey ) ) {
    UNWRAP_CHECK_MSG(
        t, ns_tbl[kStructKey].get_if<unique_ptr<rcl::table>>(),
        "value of key {} must be a table.", kStructKey );
    parse_structs( namespaces, *t, rds );
  }

  // Sub-namespaces.
  if( ns_tbl.has_key( kNamespaceKey ) ) {
    UNWRAP_CHECK_MSG(
        t,
        ns_tbl[kNamespaceKey].get_if<unique_ptr<rcl::table>>(),
        "value of key {} must be a table.", kNamespaceKey );
    parse_namespaces( namespaces, *t, rds );
  }
}

void parse_namespaces( vector<string> const& parent_namespaces,
                       rcl::table const& tbl, expr::Rds& rds ) {
  for( auto& [ns_name, ns_val] : tbl ) {
    CHECK( !reserved_name( ns_name ),
           "expected namespace name but found reserved name {}.",
           ns_name );
    CHECK( ns_val.holds<unique_ptr<rcl::table>>(),
           "namespace name {} expected to have table value.",
           ns_name );
    rcl::table const& ns_tbl =
        *ns_val.as<unique_ptr<rcl::table>>();
    parse_namespace( parent_namespaces, ns_name, ns_tbl, rds );
  }
}

void parse_includes( rcl::list const& includes,
                     expr::Rds&       rds ) {
  for( rcl::value const& v : includes ) {
    UNWRAP_CHECK_MSG( name, v.get_if<string>(),
                      "elements of `{}` must be strings.",
                      kIncludeKey );
    if( name.ends_with( ">" ) )
      rds.includes.push_back( name );
    else
      rds.includes.push_back( fmt::format( "\"{}\"", name ) );
  }
}

} // namespace

expr::Rds parse( string_view filename ) {
  UNWRAP_CHECK( doc, rcl::parse_file( filename ) );
  expr::Rds rds;

  rds.meta = expr::Metadata{
      .module_name = fs::path( filename ).filename().stem(),
  };

  rcl::table const& top = doc.top_tbl();
  if( top.size() == 0 ) return rds;

  for( auto& [k, v] : top ) {
    if( k == kIncludeKey ) {
      UNWRAP_CHECK_MSG(
          l, top[kIncludeKey].get_if<unique_ptr<rcl::list>>(),
          "`{}` must be a list.", kIncludeKey );
      parse_includes( *l, rds );
    } else if( k == kNamespaceKey ) {
      UNWRAP_CHECK_MSG( ns, v.get_if<unique_ptr<rcl::table>>(),
                        "the `{}` item must be a table.",
                        kNamespaceKey );
      parse_namespaces( /*parent_namespaces=*/{}, *ns, rds );
    } else {
      FATAL( "unrecognized/misplaced top-level keyword {}.", k );
    }
  }

  return rds;
}

} // namespace rds
