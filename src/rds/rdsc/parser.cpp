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

// luapp
#include "luapp/state.hpp"

// base
#include "base/fs.hpp"
#include "base/maybe.hpp"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace rds {

namespace {

using ::base::maybe;

constexpr string_view meta_key     = "__meta";
constexpr string_view features_key = "__features";

void add_enum( string_view name, lua::table tbl,
               expr::Rds& rds ) {
  expr::Item item;
  item.ns = lua::cast<string>( tbl[meta_key]["namespace"] );

  expr::Enum enum_;
  enum_.name = name;
  for( int i = 1; tbl[i] != lua::nil; ++i )
    enum_.values.push_back( lua::cast<string>( tbl[i] ) );

  item.constructs.push_back( enum_ );

  rds.items.push_back( item );
}

void add_sumtype( lua::state& st, string_view name,
                  lua::table tbl, expr::Rds& rds ) {
  expr::Item item;
  item.ns = lua::cast<string>( tbl[meta_key]["namespace"] );

  expr::Sumtype sumtype;
  sumtype.name = name;

  if( tbl[meta_key]["template"] != lua::nil ) {
    lua::table tmpl =
        lua::cast<lua::table>( tbl[meta_key]["template"] );
    for( int i = 1; tmpl[i] != lua::nil; ++i ) {
      expr::TemplateParam tmpl_param;
      tmpl_param.param = lua::cast<string>( tmpl[i] );
      sumtype.tmpl_params.push_back( tmpl_param );
    }
  }

  if( tbl[features_key] != lua::nil ) {
    sumtype.features.emplace();
    lua::table features =
        lua::cast<lua::table>( tbl[features_key] );
    for( int i = 1; features[i] != lua::nil; ++i ) {
      string feature_name = lua::cast<string>( features[i] );
      maybe<expr::e_sumtype_feature> feat =
          expr::feature_from_str( feature_name );
      CHECK( feat, "unknown feature name: {}", feature_name );
      sumtype.features->push_back( *feat );
    }
  }

  for( int i = 1; tbl[i] != lua::nil; ++i ) {
    lua::table alt_tbl =
        lua::cast<lua::table>( st["alt"]( tbl[i] ) );
    lua::any k        = alt_tbl["alt_name"];
    lua::any v        = alt_tbl["alt_vars"];
    string   alt_name = lua::cast<string>( k );
    if( alt_name.starts_with( "__" ) ) continue;
    expr::Alternative alt;
    alt.name = alt_name;

    CHECK( lua::type_of( v ) == lua::type::table,
           "type of alternative {} is not table.", alt_name );
    lua::table alt_members = lua::cast<lua::table>( v );
    for( int j = 1; alt_members[j] != lua::nil; ++j ) {
      lua::table var_tbl =
          lua::cast<lua::table>( st["var"]( alt_members[j] ) );
      string var_name = lua::cast<string>( var_tbl["var_name"] );
      string var_type = lua::cast<string>( var_tbl["var_type"] );
      expr::AlternativeMember alt_member{ .type = var_type,
                                          .var  = var_name };
      alt.members.push_back( alt_member );
    }
    sumtype.alternatives.push_back( alt );
  }

  item.constructs.push_back( sumtype );
  rds.items.push_back( item );
}

} // namespace

maybe<expr::Rds> parse( string_view preamble_filename,
                        string_view filename ) {
  lua::state st;
  st.lib.open_all();
  st.script.run_file( preamble_filename );
  st.script.run_file( filename );

  expr::Rds rds;

  lua::table entities = st["rds"]["entities"].cast<lua::table>();
  for( int i = 1; entities[i] != lua::nil; ++i ) {
    lua::any v = entities[i];
    CHECK( lua::type_of( v ) == lua::type::table,
           "type of entity is not table." );
    lua::table tbl = lua::cast<lua::table>( v );
    if( lua::type_of( tbl[meta_key] ) != lua::type::table )
      continue;
    string name = lua::cast<string>( tbl[meta_key]["name"] );
    string type = lua::cast<string>( tbl[meta_key]["type"] );
    // fmt::print( "name: {}, type: {}\n", name, type );
    if( type == "enum" )
      add_enum( name, tbl, rds );
    else if( type == "sumtype" )
      add_sumtype( st, name, tbl, rds );
    else {
      FATAL( "unknown type: {}", type );
    }
  }

  lua::table includes = st["rds"]["includes"].cast<lua::table>();
  for( int i = 1; includes[i] != lua::nil; ++i ) {
    string name = lua::cast<string>( includes[i] );
    if( name.ends_with( ">" ) )
      rds.includes.push_back( name );
    else
      rds.includes.push_back( fmt::format( "\"{}\"", name ) );
  }

  rds.meta = expr::Metadata{
      .module_name = fs::path( filename ).filename().stem(),
  };

  return rds;
}

} // namespace rds
