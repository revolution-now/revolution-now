#include "fmt-helper.hpp"

#include "error.hpp"
#include "variant.hpp"

#include "base/co-compat.hpp"
#include "base/conv.hpp"
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/function-ref.hpp"
#include "base/io.hpp"
#include "base/scope-exit.hpp"
#include "base/unique-coro.hpp"
#include "base/variant.hpp"

#include "base-util/stopwatch.hpp"

using namespace std;
using namespace base;

void remove_prefix( string_view& sv, int n ) {
  CHECK( sv.size() >= size_t( n ) );
  sv.remove_prefix( n );
}

struct value;

struct table {
  vector<pair<string, shared_ptr<value>>> members;
};

using ValueT = base::variant<int, string, table>;

struct value : public ValueT {
  using base_type = ValueT;

  value( base_type&& b ) : base_type( std::move( b ) ) {}

  using ValueT::ValueT;
  ValueT const& as_base() const {
    return static_cast<ValueT const&>( *this );
  }
};

struct doc {
  table tbl;
};

string pretty_print_table( table const& tbl,
                           string_view  indent = "" );

string pretty_print_value( value const& v,
                           string_view  indent = "" ) {
  return rn::overload_visit(
      v, []( int n ) { return fmt::to_string( n ); },
      []( string const& s ) { return s; },
      [&]( table const& tbl ) {
        return pretty_print_table( tbl, indent );
      } );
}

string pretty_print_table( table const& tbl,
                           string_view  indent ) {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = tbl.members.size();
  for( auto& [k, v] : tbl.members ) {
    oss << fmt::format(
        "{}{}: {}\n", indent, k,
        pretty_print_value( *v, string( indent ) + "  " ) );
    if( is_top_level && n-- > 1 ) oss << "\n";
  }

  if( !is_top_level ) {
    indent.remove_suffix( 2 );
    oss << fmt::format( "{}}}", indent );
  }

  return oss.str();
}

struct Error {
  string msg_;

  explicit Error() : msg_( "" ) {}
  explicit Error( string_view m ) : msg_( m ) {}

  template<typename... Args>
  explicit Error( string_view m, Args&&... args )
    : msg_( fmt::format( m, FWD( args )... ) ) {}

  string const& what() const { return msg_; }

  operator string() const { return msg_; }
};

template<typename T>
struct promise_type;

template<typename T = std::monostate>
struct parser {
  using value_type = T;

  parser( promise_type<T>* p )
    : promise_( p ),
      h_( coro::coroutine_handle<promise_type<T>>::from_promise(
          *promise_ ) ) {}

  bool finished() const { return promise_->o_.has_value(); }

  bool is_good() const {
    return promise_->o_.has_value() && promise_->o_->has_value();
  }

  bool is_error() const {
    return promise_->o_.has_value() && promise_->o_->has_error();
  }

  int consumed() const { return promise_->consumed_; }

  int farthest() const { return promise_->farthest_; }

  T& get() {
    CHECK( is_good() );
    return **promise_->o_;
  }

  T const& get() const {
    CHECK( is_good() );
    return **promise_->o_;
  }

  Error& error() {
    CHECK( is_error() );
    return promise_->o_->error();
  }

  Error const& error() const {
    CHECK( is_error() );
    return promise_->o_->error();
  }

  parser( parser const& ) = delete;
  parser& operator=( parser const& ) = delete;
  parser( parser&& )                 = default;
  parser& operator=( parser&& ) = default;

  promise_type<T>*  promise_;
  base::unique_coro h_;
};

DEFINE_FORMAT_T( ( T ), (parser<T>), "{}",
                 ( o.is_good() ? fmt::format( "{}", o.get() )
                   : o.is_error()
                       ? fmt::format( "{}", o.error() )
                       : string( "unfinished" ) ) );
DEFINE_FORMAT( value, "{}", o.as_base() );
DEFINE_FORMAT( Error, "{}", o.what() );
DEFINE_FORMAT( table, "{}", rn::FmtJsonStyleList{ o.members } );
DEFINE_FORMAT_T( ( T ), (shared_ptr<T>), "{}",
                 ( o ? fmt::format( "{}", *o )
                     : string( "nullptr" ) ) );
DEFINE_FORMAT( doc, "{}", pretty_print_table( o.tbl ) );

struct get_next_char {};
struct get_farthest {};

template<typename T>
struct try_ {
  try_( parser<T> p_ ) : p( std::move( p_ ) ) {}
  parser<T> p;
};

template<typename Derived, typename T>
struct promise_value_returner {
  void return_value( T const& val ) {
    static_cast<Derived&>( *this ).return_value_( val );
  }
};

template<typename Derived>
struct promise_void_returner {
  void return_void() {
    static_cast<Derived&>( *this ).return_void_();
  }
};

template<typename T>
struct promise_type
  : std::conditional_t<
        std::is_same_v<T, std::monostate>,
        promise_void_returner<promise_type<T>>,
        promise_value_returner<promise_type<T>, T>> {
  maybe<expect<T, Error>> o_;
  std::string_view        in_       = "";
  int                     consumed_ = 0;
  int                     farthest_ = 0;

  promise_type() = default;

  // Always suspend initially because the coroutine is unable to
  // do anything immediately because it does not have access to
  // the buffer to be parsed. It only gets that when it is
  // awaited upon.
  auto initial_suspend() const { return base::suspend_always{}; }

  auto final_suspend() const noexcept {
    // Always suspend because the parser object owns the corou-
    // tine and will be the one to destroy it when it goes out of
    // scope.
    return base::suspend_always{};
  }

  // Ensure that this is not copyable. See
  // https://devblogs.microsoft.com/oldnewthing/20210504-00/?p=105176
  // for the arcane reason as to why.
  promise_type( promise_type const& ) = delete;
  void operator=( promise_type const& ) = delete;

  auto get_return_object() { return parser<T>( this ); }

  void unhandled_exception() { SHOULD_NOT_BE_HERE; }

  void return_value_( T const& val ) {
    CHECK( !o_ );
    o_.emplace( val );
  }

  void return_void_() {
    CHECK( !o_ );
    o_.emplace( T{} );
  }

  auto await_transform( Error const& w ) noexcept {
    o_ = w;
    return base::suspend_always{};
  }

  template<typename U>
  auto await_transform( parser<U> p ) noexcept {
    struct awaitable {
      parser<U>     parser_;
      promise_type* promise_;

      // Promise pointer so that template type can be inferred.
      awaitable( promise_type* p, parser<U> parser )
        : parser_( std::move( parser ) ), promise_( p ) {}

      bool await_ready() noexcept {
        // At this point parser_.h_ should be at its initial sus-
        // pend point, waiting to be run.
        CHECK( !parser_.finished() );
        // Now we need to give parser_.h_'s promise object the
        // buffer to be parsed.
        parser_.promise_->in_ = promise_->in_;
        parser_.h_.resource().resume();
        // parser_.h_ should be at its final suspend point now,
        // waiting to be destroyed by the parser object that owns
        // it.
        CHECK( parser_.finished() );
        if( !parser_.is_good() )
          promise_->o_.emplace( std::move( parser_.error() ) );
        promise_->farthest_ =
            std::max( promise_->farthest_,
                      promise_->consumed_ + parser_.farthest() );
        return parser_.is_good();
      }

      void await_suspend( coro::coroutine_handle<> ) noexcept {
        // A parse failed.
      }

      U await_resume() {
        CHECK( parser_.is_good() );
        int chars_consumed =
            promise_->in_.size() - parser_.promise_->in_.size();
        CHECK( chars_consumed >= 0 );
        CHECK( promise_->in_.size() >=
               size_t( chars_consumed ) );
        // We only consume chars upon success.
        promise_->in_.remove_prefix( chars_consumed );
        promise_->consumed_ += chars_consumed;
        return parser_.get();
      }
    };
    return awaitable( this, std::move( p ) );
  }

  // This parser is allowed to fail.
  template<typename U>
  auto await_transform( try_<U> t ) noexcept {
    struct awaitable {
      parser<U>     parser_;
      promise_type* promise_;

      // Promise pointer so that template type can be inferred.
      awaitable( promise_type* p, parser<U> parser )
        : parser_( std::move( parser ) ), promise_( p ) {}

      bool await_ready() noexcept {
        // At this point parser_.h_ should be at its initial sus-
        // pend point, waiting to be run.
        CHECK( !parser_.finished() );
        // Now we need to give parser_.h_'s promise object the
        // buffer to be parsed.
        parser_.promise_->in_ = promise_->in_;
        parser_.h_.resource().resume();
        // parser_.h_ should be at its final suspend point now,
        // waiting to be destroyed by the parser object that owns
        // it.
        CHECK( parser_.finished() );
        promise_->farthest_ =
            std::max( promise_->farthest_,
                      promise_->consumed_ + parser_.farthest() );
        // This is a parser that is allowed to fail, so always
        // return true;
        return true;
      }

      void await_suspend( coro::coroutine_handle<> ) noexcept {
        // A parse failed.
      }

      expect<U, Error> await_resume() {
        if( parser_.is_error() ) return parser_.error();
        CHECK( parser_.is_good() );
        int chars_consumed =
            promise_->in_.size() - parser_.promise_->in_.size();
        CHECK( chars_consumed >= 0 );
        CHECK( promise_->in_.size() >=
               size_t( chars_consumed ) );
        // We only consume chars upon success.
        promise_->in_.remove_prefix( chars_consumed );
        promise_->consumed_ += chars_consumed;
        return parser_.get();
      }
    };
    return awaitable( this, std::move( t.p ) );
  }

  auto await_transform( get_next_char const& o ) noexcept {
    struct awaitable {
      promise_type* p_;
      get_next_char o_;

      bool await_ready() noexcept { return true; }

      void await_suspend( coro::coroutine_handle<> ) noexcept {
        SHOULD_NOT_BE_HERE;
      }

      maybe<char> await_resume() noexcept {
        maybe<char> res;
        if( p_->in_.size() != 0 ) {
          res = p_->in_[0];
          p_->in_.remove_prefix( 1 );
          p_->consumed_++;
          p_->farthest_ = p_->consumed_;
        }
        return res;
      }
    };
    return awaitable{ this, o };
  }

  auto await_transform( get_farthest ) noexcept {
    struct awaitable {
      int farthest_;
      awaitable( int farthest ) : farthest_( farthest ) {}
      bool await_ready() noexcept { return true; }
      void await_suspend( coro::coroutine_handle<> ) noexcept {}
      int  await_resume() noexcept { return farthest_; }
    };
    return awaitable{ farthest_ };
  }
};

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::parser<T>, Args...> {
  using promise_type = ::promise_type<T>;
};

} // namespace CORO_NS

template<typename T>
struct tag {};

template<typename T>
parser<T> parse() {
  return parser_for( tag<T>{} );
}

parser<maybe<char>> next_char_consume() {
  co_return co_await get_next_char{};
}

parser<> parse_exact_char( char c ) {
  maybe<char> next = co_await next_char_consume();
  if( !next || *next != c ) co_await Error( "" );
}

parser<> parse_space_char() {
  maybe<char> next = co_await next_char_consume();
  if( !next || ( *next != ' ' && *next != '\n' ) )
    co_await Error( "" );
}

parser<char> parse_any_identifier_char() {
  maybe<char> c = co_await next_char_consume();
  if( !c || !( ( *c >= 'a' && *c <= 'z' ) || *c == '_' ) )
    co_await Error( "" );
  co_return *c;
}

parser<char> parse_any_number_char() {
  maybe<char> c = co_await next_char_consume();
  if( !c || !( *c >= '0' && *c <= '9' ) ) co_await Error( "" );
  co_return *c;
}

struct Repeated {
  template<typename Func>
  auto operator()( Func&& f ) const -> parser<
      vector<typename std::invoke_result_t<Func>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func>::value_type;
    vector<res_t> res;
    while( true ) {
      auto m = co_await try_{ f() };
      if( !m ) break;
      res.push_back( std::move( *m ) );
    }
    co_return res;
  }
};
inline constexpr Repeated repeated{};

struct Some {
  template<typename Func>
  auto operator()( Func&& f ) const -> parser<
      vector<typename std::invoke_result_t<Func>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func>::value_type;
    vector<res_t> res = co_await repeated( f );
    if( res.empty() ) co_await Error( "" );
    co_return res;
  }
};
inline constexpr Some some{};

parser<> eat_spaces() { co_await repeated( parse_space_char ); }

parser<int> parser_for( tag<int> ) {
  co_await eat_spaces();
  vector<char> chars   = co_await some( parse_any_number_char );
  string       num_str = string( chars.begin(), chars.end() );
  maybe<int>   i       = base::stoi( num_str );
  if( !i ) co_await Error( "" );
  co_return *i;
}

parser<string> parser_for( tag<string> ) {
  co_await eat_spaces();
  vector<char> chars =
      co_await some( parse_any_identifier_char );
  co_return string( chars.begin(), chars.end() );
}

parser<pair<string, value>> parse_kv() {
  string k = co_await parse<string>();
  co_await parse_exact_char( ':' );
  value v = co_await parse<value>();
  co_return pair{ k, v };
}

parser<vector<pair<string, value>>> parse_inner_table() {
  return repeated( parse_kv );
}

table table_from_kv_pairs(
    vector<pair<string, value>> const& kv_pairs ) {
  table res;
  for( auto const& [k, v] : kv_pairs )
    res.members.push_back( pair{ k, make_shared<value>( v ) } );
  return res;
}

parser<table> parser_for( tag<table> ) {
  co_await eat_spaces();
  co_await parse_exact_char( '{' );
  auto kv_pairs = co_await parse_inner_table();
  co_await eat_spaces();
  co_await parse_exact_char( '}' );
  co_return table_from_kv_pairs( kv_pairs );
}

template<typename... Args>
parser<base::variant<Args...>> parser_for(
    tag<base::variant<Args...>> ) {
  using res_t = base::variant<Args...>;
  maybe<res_t> res;

  auto one = [&]<typename Alt>( Alt* ) -> parser<> {
    if( res.has_value() ) co_return;
    auto exp = co_await try_{ parse<Alt>() };
    if( !exp ) co_return;
    res.emplace( std::move( *exp ) );
  };
  ( co_await one( (Args*)nullptr ), ... );

  if( !res )
    co_return Error();
  else
    co_return *res;
}

parser<value> parser_for( tag<value> ) {
  co_return co_await parse<value::base_type>();
}

parser<doc> parser_for( tag<doc> ) {
  vector<pair<string, value>> kvs =
      co_await repeated( parse_kv );
  doc res;
  res.tbl = table_from_kv_pairs( kvs );
  co_return res;
}

struct ErrorPos {
  int line;
  int col;
};

ErrorPos error_pos( string_view in, int pos ) {
  CHECK_LT( pos, int( in.size() ) );
  ErrorPos res{ 1, 1 };
  for( int i = 0; i < pos; ++i ) {
    ++res.col;
    if( in[i] == '\n' ) {
      ++res.line;
      res.col = 1;
    }
  }

  return res;
}

int main( int /*unused*/, char** /*unused*/ ) {
  rn::linker_dont_discard_module_error();
  UNWRAP_CHECK( in, read_text_file_as_string( "input.txt" ) );

  util::StopWatch watch;

  parser<doc> res   = parse<doc>();
  res.promise_->in_ = in;

  watch.start( "parsing" );
  res.h_.resource().resume();
  watch.stop( "parsing" );

  CHECK( res.finished() );
  if( !res.is_good() || res.consumed() != int( in.size() ) ) {
    // It's always one too far, not sure why.
    ErrorPos ep = error_pos( in, res.farthest() - 1 );
    fmt::print( "input.txt:error:{}:{}: {}\n", ep.line, ep.col,
                res.is_error() ? res.error()
                               : Error( "unknown error" ) );
  } else {
    fmt::print( "{}", res );
    fmt::print( "\n\nparsing took {}.\n",
                watch.human( "parsing" ) );
  }
  return 0;
}
