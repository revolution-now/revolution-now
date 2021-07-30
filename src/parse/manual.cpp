#include "fmt-helper.hpp"

#include "base/conv.hpp"
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/function-ref.hpp"
#include "base/io.hpp"
#include "base/variant.hpp"

// #define TRACE 1

using namespace std;
using namespace base;

void remove_prefix( string_view& sv, int n ) {
  CHECK( sv.size() >= size_t( n ) );
  sv.remove_prefix( n );
}

template<typename... Args>
void trace( string_view sv, Args&&... args ) {
  (void)sv;
  ( (void)args, ... );
#if TRACE
  fmt::print( "[debug] {}", fmt::format( sv, FWD( args )... ) );
#endif
}

struct value;

struct table {
  vector<pair<string, shared_ptr<value>>> members;
};

using ValueT = base::variant<int, string, table>;

struct value : public ValueT {
  using ValueT::ValueT;
  ValueT const& as_base() const {
    return static_cast<ValueT const&>( *this );
  }
};

struct Error {
  string msg_;

  template<typename... Args>
  explicit Error( string_view m, Args&&... args )
    : msg_( fmt::format( m, FWD( args )... ) ) {}

  string const& what() const { return msg_; }

  operator string() const { return msg_; }
};

DEFINE_FORMAT( value, "{}", o.as_base() );
DEFINE_FORMAT( Error, "{}", o.what() );
DEFINE_FORMAT( table, "{}", rn::FmtJsonStyleList{ o.members } );
DEFINE_FORMAT_T( ( T ), (shared_ptr<T>), "{}",
                 ( o ? fmt::format( "{}", *o )
                     : string( "nullptr" ) ) );

template<typename T>
using M = expect<pair<T, int>, Error>;

M<value> parse_value( string_view in );

M<char> parse_char( string_view in, char c ) {
  char s[2] = { c, '\0' };
  trace( "parsing '{}'.\n", c == '\n' ? "\\n" : string( s ) );
  if( in.size() == 0 || in[0] != c )
    return Error( "expected {}", c );
  return pair{ c, 1 };
}

M<char> parse_space_char( string_view in ) {
  if( in.size() == 0 || ( in[0] != ' ' && in[0] != '\n' ) )
    return Error( "expected space character" );
  M<char> s = parse_char( in, ' ' );
  if( s ) return s;
  M<char> nl = parse_char( in, '\n' );
  if( nl ) return nl;
  return Error( "expecting space character" );
}

M<char> parse_any_identifier_char( string_view in ) {
  trace( "parsing any identifier char.\n" );
  if( in.size() == 0 ) return Error( "expected non-space char" );
  if( ( in[0] >= 'a' && in[0] <= 'z' ) || in[0] == '_' )
    return pair{ in[0], 1 };
  return Error( "expected non-space char, got {}.", in[0] );
}

M<char> parse_any_number_char( string_view in ) {
  trace( "parsing any number char.\n" );
  if( in.size() == 0 ) return Error( "expected number char" );
  if( ( in[0] >= '0' && in[0] <= '9' ) ) return pair{ in[0], 1 };
  return Error( "expected number char, got {}.", in[0] );
}

template<typename T>
M<vector<T>> repeated( string_view                       in,
                       function_ref<M<T>( string_view )> f ) {
  size_t    original_size = in.size();
  vector<T> res;
  while( in.size() > 0 ) {
    auto m = f( in );
    if( !m ) break;
    res.push_back( m->first );
    remove_prefix( in, m->second );
  }
  return pair{ res, original_size - in.size() };
}

template<typename T>
M<vector<T>> some( string_view                       in,
                   function_ref<M<T>( string_view )> f ) {
  M<vector<T>> res = repeated<T>( in, f );
  CHECK( res, "repeated should never fail." );
  if( res->first.empty() )
    return Error( "failed to find at least one." );
  return res;
}

template<typename T>
M<T> trimmed( string_view                       in,
              function_ref<M<T>( string_view )> f ) {
  size_t original_size = in.size();

  auto _1 = repeated<char>( in, parse_space_char );
  if( _1 ) remove_prefix( in, _1->second );

  auto res = f( in );
  if( !res ) return res.error();
  remove_prefix( in, res->second );

  auto _2 = repeated<char>( in, parse_space_char );
  if( _2 ) remove_prefix( in, _2->second );

  res->second = ( original_size - in.size() );
  return res;
}

M<int> parse_int( string_view in ) {
  trace( "parsing int.\n" );
  size_t          original_size = in.size();
  string          res;
  M<vector<char>> chars =
      some<char>( in, parse_any_number_char );
  if( !chars ) return chars.error();
  remove_prefix( in, chars->second );
  string num_str =
      string( chars->first.begin(), chars->first.end() );
  maybe<int> i = base::stoi( num_str );
  if( !i )
    return Error( "could not convert {} to int.", num_str );
  return pair{ *i, original_size - in.size() };
}

M<string> parse_string( string_view in ) {
  trace( "parsing string.\n" );
  size_t          original_size = in.size();
  string          res;
  M<vector<char>> chars =
      some<char>( in, parse_any_identifier_char );
  if( !chars ) return chars.error();
  remove_prefix( in, chars->second );
  return pair{
      string( chars->first.begin(), chars->first.end() ),
      original_size - in.size() };
}

M<pair<string, value>> parse_kv( string_view in ) {
  trace( "parsing kv.\n" );
  size_t original_size = in.size();

  M<string> k = trimmed<string>( in, parse_string );
  if( !k ) return k.error();
  remove_prefix( in, k->second );
  trace( "parsed key: {}\n", k->first );

  M<char> c = trimmed<char>( in, []( string_view in ) {
    return parse_char( in, ':' );
  } );
  if( !c ) return c.error();
  remove_prefix( in, c->second );

  M<value> v = trimmed<value>( in, parse_value );
  if( !v ) return v.error();
  remove_prefix( in, v->second );
  trace( "received v={}\n", v->first );

  pair<string, value> res{ k->first, v->first };
  return pair{ res, original_size - in.size() };
}

M<vector<pair<string, value>>> parse_inner_table(
    string_view in ) {
  trace( "parsing inner table.\n" );
  size_t original_size = in.size();

  M<vector<pair<string, value>>> kv_pairs =
      repeated<pair<string, value>>( in, []( string_view in ) {
        return trimmed<pair<string, value>>( in, parse_kv );
      } );
  if( !kv_pairs ) return kv_pairs.error();
  remove_prefix( in, kv_pairs->second );
  trace( "received kv_pairs={}\n",
         rn::FmtJsonStyleList{ kv_pairs->first } );

  return pair{ kv_pairs->first, original_size - in.size() };
}

M<vector<pair<string, value>>> parse_nonempty_inner_table(
    string_view in ) {
  trace( "parsing inner table.\n" );
  size_t original_size = in.size();

  M<vector<pair<string, value>>> kv_pairs =
      some<pair<string, value>>( in, []( string_view in ) {
        return trimmed<pair<string, value>>( in, parse_kv );
      } );
  if( !kv_pairs ) return kv_pairs.error();
  remove_prefix( in, kv_pairs->second );
  trace( "received kv_pairs={}\n",
         rn::FmtJsonStyleList{ kv_pairs->first } );

  return pair{ kv_pairs->first, original_size - in.size() };
}

table table_from_kv_pairs(
    vector<pair<string, value>> const& kv_pairs ) {
  table res;
  for( auto const& [k, v] : kv_pairs )
    res.members.push_back( pair{ k, make_shared<value>( v ) } );
  return res;
}

M<table> parse_table( string_view in ) {
  trace( "parsing table.\n" );
  size_t original_size = in.size();

  M<char> c = trimmed<char>( in, []( string_view in ) {
    return parse_char( in, '{' );
  } );
  if( !c ) return c.error();
  remove_prefix( in, c->second );

  M<vector<pair<string, value>>> kv_pairs =
      trimmed<vector<pair<string, value>>>(
          in, []( string_view in ) {
            return parse_inner_table( in );
          } );
  if( !kv_pairs ) return kv_pairs.error();
  remove_prefix( in, kv_pairs->second );
  trace( "received kv_pairs={}\n",
         rn::FmtJsonStyleList{ kv_pairs->first } );

  M<char> c2 = trimmed<char>( in, []( string_view in ) {
    return parse_char( in, '}' );
  } );
  if( !c2 ) return c2.error();
  remove_prefix( in, c2->second );

  return pair{ table_from_kv_pairs( kv_pairs->first ),
               original_size - in.size() };
}

M<value> parse_value( string_view in ) {
  trace( "parsing value.\n" );
  size_t original_size = in.size();
  value  res;
  {
    trace( "trying inner table.\n" );
    M<vector<pair<string, value>>> it =
        parse_nonempty_inner_table( in );
    if( it ) {
      res = value{ table_from_kv_pairs( it->first ) };
      remove_prefix( in, it->second );
      trace( "*** parsed inner table: {}\n", it->first );
      return pair{ res, original_size - in.size() };
    }
  }
  {
    trace( "trying int.\n" );
    M<int> i = parse_int( in );
    if( i ) {
      res = value{ i->first };
      remove_prefix( in, i->second );
      trace( "*** parsed int: {}\n", i->first );
      return pair{ res, original_size - in.size() };
    }
  }
  {
    trace( "trying string.\n" );
    M<string> s = parse_string( in );
    if( s ) {
      res = value{ s->first };
      remove_prefix( in, s->second );
      trace( "*** parsed string: {}\n", s->first );
      return pair{ res, original_size - in.size() };
    }
  }
  {
    trace( "trying table.\n" );
    M<table> t = parse_table( in );
    if( t ) {
      res = value{ t->first };
      remove_prefix( in, t->second );
      trace( "*** parsed table: {}\n", t->first );
      return pair{ res, original_size - in.size() };
    }
  }
  trace( "failed parsing value..\n" );
  return Error( "failed to parse value." );
}

int main( int /*unused*/, char** /*unused*/ ) {
  UNWRAP_CHECK( in, read_text_file_as_string( "input.txt" ) );

  M<value> res = trimmed<value>( in, parse_value );
  if( res ) {
    if( int( in.size() ) > res->second )
      fmt::print( "failed at character {}.\n", res->second );
    else {
      fmt::print( "success: {}\n", res->first );
      fmt::print( "(top-level: {})\n", res->first.index() );
    }
  } else
    fmt::print( "failed: {}\n", res.error() );
  return 0;
}
