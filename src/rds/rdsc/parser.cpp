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
#include "base/fmt.hpp"
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/maybe.hpp"

using namespace std;

namespace rds {

namespace {

using ::base::maybe;

constexpr string_view features_key = "_features";
constexpr string_view context_key  = "_context";
constexpr string_view template_key = "_template";

/****************************************************************
** Comments Blankifier
*****************************************************************/
// This will keep the string the same length but will ovewrite
// all comments (comment delimiters and comment contents) with
// spaces).
void blankify_comments( string& text ) {
  bool in_comment     = false;
  bool in_dquoted_str = false;
  bool in_squoted_str = false;
  int  i              = 0;
  for( ; i < int( text.size() ); ++i ) {
    char c = text[i];
    if( c == '\n' || c == '\r' ) {
      in_comment     = false;
      in_dquoted_str = false;
      in_squoted_str = false;
      continue;
    }
    if( in_dquoted_str ) {
      if( c != '"' ) continue;
      in_dquoted_str = false;
      continue;
    }
    if( in_squoted_str ) {
      if( c != '\'' ) continue;
      in_squoted_str = false;
      continue;
    }
    if( in_comment ) {
      text[i] = ' ';
      continue;
    }
    // We're not in a comment or string.
    if( c == '#' ) {
      // start comment.
      in_comment = true;
      text[i]    = ' ';
      continue;
    }
    if( c == '"' ) {
      // start double quoted string[
      in_dquoted_str = true;
      continue;
    }
    if( c == '\'' ) {
      // start single quoted string[
      in_squoted_str = true;
      continue;
    }
  }
}

/****************************************************************
** Enum
*****************************************************************/
void add_enum( string_view ns, string_view name, lua::table tbl,
               expr::Rds& rds ) {
  expr::Item item;
  item.ns = ns;

  expr::Enum enum_;
  enum_.name = name;
  for( int i = 1; tbl[i] != lua::nil; ++i ) {
    if( lua::type_of( tbl[i] ) == lua::type::table &&
        tbl[i]["name"] == features_key ) {
      enum_.features.emplace();
      lua::table features = tbl[i]["obj"].as<lua::table>();
      for( int j = 1; features[j] != lua::nil; ++j ) {
        string feature_name = features[j]
                                  .as<lua::rfunction>()()["name"]
                                  .as<string>();
        maybe<expr::e_feature> feat =
            expr::feature_from_str( feature_name );
        CHECK( feat, "unknown feature name: {}", feature_name );
        enum_.features->insert( *feat );
      }
      continue;
    }

    lua::any   item_fn = tbl[i].as<lua::rfunction>();
    lua::table item_tbl =
        item_fn.as<lua::rfunction>()().as<lua::table>();
    lua::any name = item_tbl["name"];
    CHECK( name != lua::nil );
    if( lua::type_of( name ) == lua::type::string )
      enum_.values.push_back( name.as<string>() );
    else if( lua::type_of( name ) == lua::type::function )
      enum_.values.push_back(
          name.as<lua::rfunction>()()["name"].as<string>() );
    else {
      FATAL( "unrecognized type for enum value: type={}.",
             lua::type_of( name ) );
    }
  }

  item.constructs.push_back( enum_ );

  rds.items.push_back( item );
}

/****************************************************************
** Sumtype
*****************************************************************/
void add_sumtype( string_view ns, string_view name,
                  lua::table obj, expr::Rds& rds ) {
  expr::Item item;
  item.ns = ns;

  expr::Sumtype sumtype;
  sumtype.name = name;

  for( int i = 1; obj[i] != lua::nil; ++i ) {
    lua::table alt_tbl = obj[i].as<lua::table>();

    if( alt_tbl["name"] == template_key ) {
      lua::table tmpl = alt_tbl["obj"].as<lua::table>();
      for( int j = 1; tmpl[j] != lua::nil; ++j ) {
        expr::TemplateParam tmpl_param;
        tmpl_param.param =
            tmpl[j].as<lua::rfunction>()()["name"].as<string>();
        sumtype.tmpl_params.push_back( tmpl_param );
      }
      continue;
    }

    if( alt_tbl["name"] == features_key ) {
      sumtype.features.emplace();
      lua::table features = alt_tbl["obj"].as<lua::table>();
      for( int j = 1; features[j] != lua::nil; ++j ) {
        string feature_name = features[j]
                                  .as<lua::rfunction>()()["name"]
                                  .as<string>();
        maybe<expr::e_feature> feat =
            expr::feature_from_str( feature_name );
        CHECK( feat, "unknown feature name: {}", feature_name );
        sumtype.features->insert( *feat );
      }
      continue;
    }

    expr::Alternative alt;

    string     alt_name    = alt_tbl["name"].as<string>();
    lua::table alt_members = alt_tbl["obj"].as<lua::table>();
    alt.name               = alt_name;
    for( int j = 1; alt_members[j] != lua::nil; ++j ) {
      lua::table         var = alt_members[j].as<lua::table>();
      string             var_name = var["name"].as<string>();
      string             var_type = var["obj"].as<string>();
      expr::StructMember alt_member{ .type = var_type,
                                     .var  = var_name };
      alt.members.push_back( alt_member );
    }
    sumtype.alternatives.push_back( alt );
  }

  item.constructs.push_back( sumtype );
  rds.items.push_back( item );
}

/****************************************************************
** Interface
*****************************************************************/
void add_interface( string_view ns, string_view name,
                    lua::table obj, expr::Rds& rds ) {
  expr::Item item;
  item.ns = ns;

  expr::Interface interface;
  interface.name = name;

  for( int i = 1; obj[i] != lua::nil; ++i ) {
    lua::table method_tbl = obj[i].as<lua::table>();

    if( method_tbl["name"] == context_key ) {
      lua::table context = method_tbl["obj"].as<lua::table>();
      for( int j = 1; context[j] != lua::nil; ++j ) {
        expr::MethodArg method_arg;
        lua::table      var = context[j].as<lua::table>();
        method_arg.var      = var["name"].as<string>();
        method_arg.type     = var["obj"].as<string>();
        interface.context.members.push_back( method_arg );
      }
      continue;
    }

    if( method_tbl["name"] == features_key ) {
      interface.features.emplace();
      lua::table features = method_tbl["obj"].as<lua::table>();
      for( int j = 1; features[j] != lua::nil; ++j ) {
        string feature_name = features[j]
                                  .as<lua::rfunction>()()["name"]
                                  .as<string>();
        maybe<expr::e_feature> feat =
            expr::feature_from_str( feature_name );
        CHECK( feat, "unknown feature name: {}", feature_name );
        interface.features->insert( *feat );
      }
      continue;
    }

    expr::Method method;

    string     method_name = method_tbl["name"].as<string>();
    lua::table method_members =
        method_tbl["obj"].as<lua::table>();
    method.name = method_name;
    for( int j = 1; method_members[j] != lua::nil; ++j ) {
      lua::table var      = method_members[j].as<lua::table>();
      string     var_name = var["name"].as<string>();
      string     var_type = var["obj"].as<string>();
      if( var_name == "returns" ) {
        method.return_type = var_type;
        continue;
      }
      expr::MethodArg method_arg{ .type = var_type,
                                  .var  = var_name };
      method.args.push_back( method_arg );
    }
    CHECK( !method.return_type.empty(),
           "missing return type on method {} in interface {}.",
           method_name, interface.name );
    interface.methods.push_back( method );
  }

  item.constructs.push_back( interface );
  rds.items.push_back( item );
}

/****************************************************************
** Struct
*****************************************************************/
void add_struct( string_view ns, string_view name,
                 lua::table obj, expr::Rds& rds ) {
  expr::Item item;
  item.ns = ns;

  expr::Struct strukt;
  strukt.name = name;

  obj[template_key] = lua::nil;
  obj[features_key] = lua::nil;

  for( int i = 1; obj[i] != lua::nil; ++i ) {
    lua::table var = obj[i].as<lua::table>();

    if( var["name"] == template_key ) {
      lua::table tmpl = var["obj"].as<lua::table>();
      for( int i = 1; tmpl[i] != lua::nil; ++i ) {
        expr::TemplateParam tmpl_param;
        tmpl_param.param =
            tmpl[i].as<lua::rfunction>()()["name"].as<string>();
        strukt.tmpl_params.push_back( tmpl_param );
      }
      continue;
    }

    if( var["name"] == features_key ) {
      strukt.features.emplace();
      lua::table features = var["obj"].as<lua::table>();
      for( int i = 1; features[i] != lua::nil; ++i ) {
        string feature_name = features[i]
                                  .as<lua::rfunction>()()["name"]
                                  .as<string>();
        maybe<expr::e_feature> feat =
            expr::feature_from_str( feature_name );
        CHECK( feat, "unknown feature name: {}", feature_name );
        strukt.features->insert( *feat );
      }
      continue;
    }

    strukt.members.push_back(
        expr::StructMember{ .type = var["obj"].as<string>(),
                            .var  = var["name"].as<string>() } );
  }

  item.constructs.push_back( std::move( strukt ) );
  rds.items.push_back( std::move( item ) );
}

/****************************************************************
** Config
*****************************************************************/
void add_config( string_view ns, string_view name,
                 expr::Rds& rds ) {
  expr::Item item;
  item.ns = ns;

  expr::Config config;
  config.name = name;

  item.constructs.push_back( std::move( config ) );
  rds.items.push_back( std::move( item ) );
}

} // namespace

/****************************************************************
** Rds
*****************************************************************/
expr::Rds parse( string_view filename,
                 string_view preamble_file ) {
  lua::state st;
  st.lib.open_all();
  st.script.run_file( preamble_file );

  UNWRAP_CHECK( rds_file,
                base::read_text_file_as_string( filename ) );
  blankify_comments( rds_file );
  st.script.run( rds_file );

  expr::Rds rds;

  lua::table entities = st["rds"]["items"].as<lua::table>();
  for( int i = 1; entities[i] != lua::nil; ++i ) {
    lua::any v = entities[i];
    CHECK( lua::type_of( v ) == lua::type::table,
           "type of entity is not table." );
    lua::table tbl  = v.as<lua::table>();
    string     name = tbl["name"].as<string>();
    string     type = tbl["type"].as<string>();
    string     ns   = tbl["ns"].as<string>();
    lua::table obj  = tbl["obj"].as<lua::table>();
    // fmt::print( "name: {}, type: {}\n", name, type );
    if( type == "enum" )
      add_enum( ns, name, obj, rds );
    else if( type == "sumtype" )
      add_sumtype( ns, name, obj, rds );
    else if( type == "interface" )
      add_interface( ns, name, obj, rds );
    else if( type == "struct" )
      add_struct( ns, name, obj, rds );
    else if( type == "config" )
      add_config( ns, name, rds );
    else { FATAL( "unknown type: {}", type ); }
  }

  lua::table includes = st["rds"]["includes"].as<lua::table>();
  for( int i = 1; includes[i] != lua::nil; ++i ) {
    string name = includes[i].as<string>();
    if( name.ends_with( ">" ) )
      rds.includes.push_back( name );
    else
      rds.includes.push_back( fmt::format( "\"{}\"", name ) );
  }

  return rds;
}

} // namespace rds
