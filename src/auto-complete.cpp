/****************************************************************
**auto-complete.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-31.
*
* Description: Auto-complete for Lua in the game terminal.
*
*****************************************************************/
#include "auto-complete.hpp"

// luapp
#include "luapp/iter.hpp"
#include "luapp/ruserdata.hpp"
#include "luapp/state.hpp"

// base
#include "base/range-lite.hpp"
#include "base/string.hpp"
#include "base/to-str-tags.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rl = ::base::rl;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;

template<typename... Args>
void trace( Args&&... ) {
  // TODO
}

} // namespace

vector<string> autocomplete( lua::state& st,
                             string_view fragment ) {
  vector<string> res;
  auto is_autocomplete_char = []( char c ) {
    return std::isalnum( c ) || ( c == ':' ) || ( c == '.' ) ||
           ( c == '_' );
  };
  trace( "fragment: {}", fragment );
  auto to_autocomplete = rl::rall( fragment )
                             .take_while( is_autocomplete_char )
                             .to_string();
  reverse( to_autocomplete.begin(), to_autocomplete.end() );
  trace( "to_autocomplete: {}", to_autocomplete );
  auto prefix = rl::rall( fragment )
                    .drop( to_autocomplete.size() )
                    .to_string();
  reverse( prefix.begin(), prefix.end() );
  trace( "prefix: {}", prefix );
  // Stop here otherwise we'll be looking through all globals.
  if( to_autocomplete.empty() ) return res;
  // range-v3 split doesn't do the right thing here if the string
  // ends in a dot.
  vector<string> segments =
      base::str_split_on_any( to_autocomplete, ".:" );
  CHECK( segments.size() > 0 );
  trace( "segments.size(): {}", segments.size() );
  // This is a "drop last".
  vector<string> initial_segments(
      segments.begin(),
      segments.empty() ? segments.end() : segments.end() - 1 );

  auto sep_for_parent_child = [&]( lua::any parent,
                                   string const& key ) {
    if( lua::type_of( parent ) == lua::type::userdata &&
        lua::type_of( parent[key] ) == lua::type::function ) {
      return ':';
    }
    return '.';
  };

  auto table_for_object = []( lua::any o ) {
    maybe<lua::table> res;
    trace( "table_for_object type: {}", lua::type_of( o ) );
    if( lua::type_of( o ) == lua::type::table )
      res = lua::as<lua::table>( o );
    if( lua::type_of( o ) == lua::type::userdata ) {
      lua::userdata ud = lua::as<lua::userdata>( o );
      // FIXME: rework this to handle userdata types by using lu-
      // a::pairs to iterate over their methods (userdata will
      // need to be given a __pairs metamethod).
      res = ud[lua::metatable_key]["member_types"]
                .as<lua::table>();
    }
    return res;
  };

  auto table_count_if = [&]( lua::table t, auto&& func ) {
    int size = 0;
    // Need pairs because some tables might have been frozen
    // which means that they can only be iterated via __pairs.
    for( auto p : lua::pairs( t ) )
      if( func( lua::any( t ), p.first ) ) //
        ++size;
    return size;
  };

  auto table_size_non_meta = [&]( lua::table t ) {
    return table_count_if(
        t, []( lua::any /*parent*/, lua::any key_obj ) {
          if( lua::type_of( key_obj ) != lua::type::string )
            return false;
          return !util::starts_with( lua::as<string>( key_obj ),
                                     "__" );
        } );
  };

  auto table_fn_members_non_meta = [&]( lua::table t ) {
    return table_count_if(
        t, [&]( lua::any parent, lua::any key_obj ) {
          if( lua::type_of( key_obj ) != lua::type::string )
            return false;
          auto key = lua::as<string>( key_obj );
          if( util::starts_with( key, "__" ) ) return false;
          return ( lua::type_of( parent[key] ) ==
                   lua::type::function );
        } );
  };

  lua::table curr_table = st.table.global();
  lua::any curr_obj     = lua::any( st.table.global() );

  for( auto piece : initial_segments ) {
    trace( "piece: {}", piece );
    if( piece.empty() ) return res;
    auto maybe_table = table_for_object( curr_obj[piece] );
    // trace( "maybe_table: {}", maybe_table.has_value() );
    if( !maybe_table ) return res;
    auto size = table_size_non_meta( *maybe_table );
    trace( "table_size_non_meta: {}", size );
    if( size == 0 ) return res;
    curr_obj   = curr_obj[piece];
    curr_table = *maybe_table;
  }
  auto last = segments.back();
  trace( "last: {}", last );
  string initial = base::str_join( initial_segments, "." );

  auto add_keys = [&]( lua::any parent, auto kv ) {
    lua::any o = kv.first;
    if( lua::type_of( o ) == lua::type::string ) {
      string key = lua::as<string>( o );
      trace( "does {} start with last?", key, last );
      if( util::starts_with( key, "__" ) ) return;
      if( util::starts_with( key, last ) ) {
        // trace( "  key: {}", key );
        if( initial.empty() )
          res.push_back( key );
        else {
          auto last_sep = sep_for_parent_child( parent, key );
          trace( "last_sep ({}): {}", key, last_sep );
          res.push_back( initial + last_sep + key );
        }
        res.back() = prefix + res.back();
        trace( "  push_back: {}", res.back() );
      }
    }
  };
  // Need pairs because some tables might have been frozen which
  // means that they can only be iterated via __pairs.
  for( auto p : lua::pairs( curr_table ) )
    add_keys( curr_obj, p );

  sort( res.begin(), res.end() );
  trace( "sorted; size: {}", res.size() );

  for( auto const& match : res ) {
    auto match_dots =
        base::str_replace_all( match, { { ":", "." } } );
    auto fragment_dots =
        base::str_replace_all( fragment, { { ":", "." } } );
    CHECK( util::starts_with( match_dots, fragment_dots ),
           "`{}` does not start with `{}`", match_dots,
           fragment_dots );
  }

  if( res.size() == 0 ) {
    trace( "returning: {}", base::FmtJsonStyleList{ res } );
    return res;
  } else if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = { *prefix };
    trace( "returning: {}", base::FmtJsonStyleList{ res } );
    return res;
  }

  CHECK( res.size() == 1 );
  if( res[0] == fragment ) {
    trace( "res[0], fragment: {},{}", res[0], fragment );
    // Need to use curr_obj instead of curr_table because we need
    // the real type of the last thing. For userdata it wouldn't
    // be the real type because the type of each thing in the
    // member_types table (from which the keys of the userdata
    // are extracted) are always bools (see luapp/userdata).
    lua::any o = curr_obj[last];
    CHECK( o != lua::nil );
    if( lua::type_of( o ) == lua::type::table ) {
      UNWRAP_CHECK( t, table_for_object( o ) );
      auto size = table_size_non_meta( t );
      trace( "table size: {}", size );
      if( size > 0 ) res[0] += '.';
    }
    if( lua::type_of( o ) == lua::type::userdata ) {
      UNWRAP_CHECK( t, table_for_object( o ) );
      auto fn_count = table_fn_members_non_meta( t );
      auto size     = table_size_non_meta( t );
      trace( "table size: {}", size );
      trace( "fn count: {}", fn_count );
      if( size > 0 ) {
        if( fn_count == size )
          res[0] += ':';
        else if( fn_count == 0 )
          res[0] += '.';
        else
          res[0] += '.';
      }
    }
    if( lua::type_of( o ) == lua::type::function ) res[0] += '(';
    trace( "final res[0]: {}", res[0] );
  }
  trace( "returning: {}", base::FmtJsonStyleList{ res } );
  return res;
}

vector<string> autocomplete_iterative( lua::state& st,
                                       string_view fragment ) {
  vector<string> res;
  string single_result( fragment );
  do {
    trace( "single_result: {}", single_result );
    auto try_res = autocomplete( st, single_result );
    if( try_res.empty() ) break;
    res = std::move( try_res );
    trace( "  res: {}", base::FmtJsonStyleList{ res } );
    if( res.size() == 1 ) {
      trace( "  size is 1" );
      if( single_result == res[0] ) break;
      trace( "  not equal" );
      single_result = res[0];
    }
  } while( res.size() == 1 );
  if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = { *prefix };
  }
  return res;
}

} // namespace rn
