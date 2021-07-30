#include "fmt-helper.hpp"

#include "base/co-compat.hpp"
#include "base/conv.hpp"
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/function-ref.hpp"
#include "base/io.hpp"
#include "base/unique-coro.hpp"
#include "base/variant.hpp"

using namespace std;
using namespace base;

template<typename... Args>
void trace( string_view sv, Args&&... args ) {
  fmt::print( sv, FWD( args )... );
  fmt::print( "\n" );
}

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

template<typename T>
struct promise_type;

template<typename T = std::monostate>
struct parser {
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

DEFINE_FORMAT( value, "{}", o.as_base() );
DEFINE_FORMAT( Error, "{}", o.what() );
DEFINE_FORMAT( table, "{}", rn::FmtJsonStyleList{ o.members } );
DEFINE_FORMAT_T( ( T ), (shared_ptr<T>), "{}",
                 ( o ? fmt::format( "{}", *o )
                     : string( "nullptr" ) ) );
DEFINE_FORMAT_T( ( T ), (parser<T>), "{}",
                 o.has_value() ? fmt::format( "{}", o.get() )
                               : o.error() );

parser<value> parse_value();

struct get_next_char {
  bool consume = false;
};

template<typename T>
struct try_ {
  try_( parser<T> p_ ) : p( std::move( p_ ) ) {}
  parser<T> p;
};

template<typename T = std::monostate>
struct promise_type {
  maybe<expect<T, Error>> o_;
  std::string_view        in_ = "";

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

  void return_value( T const& val ) {
    CHECK( !o_ );
    o_.emplace( val );
  }

  void return_value() requires(
      std::is_same_v<T, std::monostate> ) {
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
        // We only consume chars upon success.
        promise_->in_.remove_prefix( chars_consumed );
        CHECK( promise_->in_.size() >= 0 );
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
        // We only consume chars upon success.
        promise_->in_.remove_prefix( chars_consumed );
        CHECK( promise_->in_.size() >= 0 );
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
          if( o_.consume ) p_->in_.remove_prefix( 1 );
        }
        return res;
      }
    };
    return awaitable{ this, o };
  }
};

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::parser<T>, Args...> {
  using promise_type = ::promise_type<T>;
};

} // namespace CORO_NS

parser<maybe<char>> next_char() {
  co_return co_await get_next_char{ /*consume=*/false };
}

parser<maybe<char>> next_char_consume() {
  co_return co_await get_next_char{ /*consume=*/true };
}

parser<char> parse_exact_char( char c ) {
  maybe<char> next = co_await next_char_consume();
  if( !next || *next != c )
    co_await Error( "expected character {}, but found {}", c,
                    next );
  co_return *next;
}

parser<char> parse_space_char() {
  maybe<char> next = co_await next_char_consume();
  if( !next || ( *next != ' ' && *next != '\n' ) )
    co_await Error( "expected space character" );
  co_return *next;
}

parser<char> parse_any_identifier_char() {
  maybe<char> c = co_await next_char_consume();
  if( !c || !( ( *c >= 'a' && *c <= 'z' ) || *c == '_' ) )
    co_await Error( "expected identifier char" );
  co_return *c;
}

parser<char> parse_any_number_char() {
  trace( "parse_any_number_char" );
  maybe<char> c = co_await next_char_consume();
  trace( "  => {}", c );
  if( !c || !( *c >= '0' && *c <= '9' ) )
    co_await Error( "expected number char, got {}.", c );
  co_return *c;
}

template<typename T>
struct Repeated {
  parser<vector<T>> operator()(
      function_ref<parser<T>()> f ) const {
    trace( "repeated" );
    vector<T> res;
    while( true ) {
      auto m = co_await try_{ f() };
      if( !m ) break;
      res.push_back( std::move( *m ) );
    }
    co_return res;
  }
};
template<typename T>
inline constexpr Repeated<T> repeated{};

template<typename T>
struct Some {
  parser<vector<T>> operator()(
      function_ref<parser<T>()> f ) const {
    trace( "some" );
    vector<T> res = co_await repeated<T>( f );
    if( res.empty() )
      co_await Error( "failed to find at least one." );
    co_return res;
  }
};
template<typename T>
inline constexpr Some<T> some{};

struct Trimmed {
  template<typename Func>
  auto operator()( Func&& f ) const -> invoke_result_t<Func> {
    trace( "trimmed" );
    co_await repeated<char>( parse_space_char );
    auto res = co_await f();
    co_await repeated<char>( parse_space_char );
    co_return res;
  }
};
inline constexpr Trimmed trimmed{};

parser<> eat_spaces() {
  co_await repeated<char>( parse_space_char );
  co_return {};
}

parser<int> parse_int() {
  trace( "parse_int" );
  vector<char> chars =
      co_await some<char>( parse_any_number_char );

  string     num_str = string( chars.begin(), chars.end() );
  maybe<int> i       = base::stoi( num_str );
  if( !i )
    co_await Error( "could not convert {} to int.", num_str );
  co_return *i;
}

parser<string> parse_string() {
  trace( "parse_string" );
  vector<char> chars =
      co_await some<char>( parse_any_identifier_char );
  co_return string( chars.begin(), chars.end() );
}

parser<pair<string, value>> parse_kv() {
  trace( "parse_kv" );
  string k = co_await parse_string();
  co_await parse_exact_char( ':' );
  co_await eat_spaces();
  value v = co_await parse_value();
  co_return pair{ k, v };
}

parser<vector<pair<string, value>>> parse_inner_table() {
  trace( "inner_table" );
  return repeated<pair<string, value>>(
      [] { return trimmed( parse_kv ); } );
}

parser<vector<pair<string, value>>> nonempty_inner_table() {
  trace( "nonempty_inner_table" );
  return some<pair<string, value>>(
      [] { return trimmed( parse_kv ); } );
}

table table_from_kv_pairs(
    vector<pair<string, value>> const& kv_pairs ) {
  table res;
  for( auto const& [k, v] : kv_pairs )
    res.members.push_back( pair{ k, make_shared<value>( v ) } );
  return res;
}

parser<table> parse_table() {
  trace( "parse_table" );
  co_await parse_exact_char( '{' );
  auto kv_pairs = co_await parse_inner_table();
  co_await parse_exact_char( '}' );
  co_return table_from_kv_pairs( kv_pairs );
}

parser<value> parse_value() {
  trace( "parse_value" );
  value res;

  if( auto r = co_await try_{ nonempty_inner_table() }; r )
    co_return value{ table_from_kv_pairs( *r ) };

  if( auto r = co_await try_{ parse_int() }; r )
    co_return value{ *r };

  if( auto r = co_await try_{ parse_string() }; r )
    co_return value{ *r };

  if( auto r = co_await try_{ parse_table() }; r )
    co_return value{ *r };

  co_await Error( "failed to parse value." );
  SHOULD_NOT_BE_HERE;
}

int main( int /*unused*/, char** /*unused*/ ) {
  UNWRAP_CHECK( in, read_text_file_as_string( "input.txt" ) );

  parser<value> res = trimmed( parse_value );
  res.promise_->in_ = in;
  res.h_.resource().resume();
  CHECK( res.finished() );
  if( !res.is_good() )
    fmt::print( "failed at character {}.\n",
                in.size() - res.promise_->in_.size() );
  else {
    fmt::print( "success: {}\n", res.get() );
    fmt::print( "(top-level: {})\n", res.get().index() );
  }
  return 0;
}
