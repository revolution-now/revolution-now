// Revolution Now
#include "error.hpp"
#include "fmt-helper.hpp"
#include "variant.hpp"

// base
#include "base/co-compat.hpp"
#include "base/conv.hpp"
#include "base/expect.hpp"
#include "base/fmt.hpp"
#include "base/io.hpp"
#include "base/unique-coro.hpp"
#include "base/variant.hpp"

#include "base-util/stopwatch.hpp"

using namespace std;

/****************************************************************
** Document Model
*****************************************************************/
namespace cl {

struct value;

struct table {
  vector<pair<string, shared_ptr<value>>> members;
  std::string pretty_print( std::string_view indent = "" ) const;
};

using ValueT = base::variant<int, string, table>;

struct value : public ValueT {
  using base_type = ValueT;

  value( base_type&& b ) : base_type( std::move( b ) ) {}

  using ValueT::ValueT;
  ValueT const& as_base() const {
    return static_cast<ValueT const&>( *this );
  }

  std::string pretty_print( std::string_view indent = "" ) const;
};

struct doc {
  table tbl;
};

} // namespace cl

/****************************************************************
** String formatting / Pretty Printing
*****************************************************************/
namespace cl {

string value::pretty_print( string_view indent ) const {
  return rn::overload_visit(
      *this, []( int n ) { return fmt::to_string( n ); },
      []( string const& s ) { return s; },
      [&]( table const& tbl ) {
        return tbl.pretty_print( indent );
      } );
}

string table::pretty_print( string_view indent ) const {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = members.size();
  for( auto& [k, v] : members ) {
    oss << fmt::format(
        "{}{}: {}\n", indent, k,
        v->pretty_print( string( indent ) + "  " ) );
    if( is_top_level && n-- > 1 ) oss << "\n";
  }

  if( !is_top_level ) {
    indent.remove_suffix( 2 );
    oss << fmt::format( "{}}}", indent );
  }

  return oss.str();
}

} // namespace cl

DEFINE_FORMAT( cl::value, "{}", o.as_base() );
DEFINE_FORMAT( cl::table, "{}",
               rn::FmtJsonStyleList{ o.members } );
DEFINE_FORMAT( cl::doc, "{}", o.tbl.pretty_print() );
DEFINE_FORMAT_T( ( T ), (shared_ptr<T>), "{}",
                 ( o ? fmt::format( "{}", *o )
                     : string( "nullptr" ) ) );

/****************************************************************
** Parser Error/Result
*****************************************************************/
namespace parz {

struct error {
  string msg_;

  explicit error() : msg_( "" ) {}
  explicit error( string_view m ) : msg_( m ) {}

  template<typename... Args>
  explicit error( string_view m, Args&&... args )
    : msg_( fmt::format( m, FWD( args )... ) ) {}

  string const& what() const { return msg_; }

  operator string() const { return msg_; }
};

template<typename T>
using result = base::expect<T, error>;

} // namespace parz

DEFINE_FORMAT( parz::error, "{}", o.what() );

/****************************************************************
** Parser
*****************************************************************/
namespace parz {

template<typename T>
struct promise_type;

template<typename T = std::monostate>
struct parser {
  using value_type  = T;
  using result_type = result<T>;

  parser( promise_type<T>* p )
    : promise_( p ),
      h_( coro::coroutine_handle<promise_type<T>>::from_promise(
          *promise_ ) ) {}

  void resume( std::string_view buffer ) {
    promise_->in_ = buffer;
    h_.resource().resume();
  }

  bool finished() const { return promise_->o_.has_value(); }

  bool is_good() const {
    return promise_->o_.has_value() && promise_->o_->has_value();
  }

  bool is_error() const {
    return promise_->o_.has_value() && promise_->o_->has_error();
  }

  result_type& result() {
    DCHECK( finished() );
    return *promise_->o_;
  }

  result_type const& result() const {
    DCHECK( finished() );
    return *promise_->o_;
  }

  int consumed() const { return promise_->consumed_; }

  int farthest() const { return promise_->farthest_; }

  T& get() {
    DCHECK( is_good() );
    return **promise_->o_;
  }

  T const& get() const {
    DCHECK( is_good() );
    return **promise_->o_;
  }

  error& error() {
    DCHECK( is_error() );
    return promise_->o_->error();
  }

  parz::error const& error() const {
    DCHECK( is_error() );
    return promise_->o_->error();
  }

  parser( parser const& ) = delete;
  parser& operator=( parser const& ) = delete;
  parser( parser&& )                 = default;
  parser& operator=( parser&& ) = default;

  promise_type<T>*  promise_;
  base::unique_coro h_;
};

} // namespace parz

DEFINE_FORMAT_T( ( T ), (parz::parser<T>), "{}",
                 ( o.is_good() ? fmt::format( "{}", o.get() )
                   : o.is_error()
                       ? fmt::format( "{}", o.error() )
                       : string( "unfinished" ) ) );

/****************************************************************
** "Magic" Parser Awaitables
*****************************************************************/
namespace parz {

// This gets the next character from the buffer and fails if
// there are no more characters.
struct next_char {};

// When a parser is wrapped in this object it will return a may-
// be<T> instead of a T and it will thus be allowed to fail
// without failing the entire parsing operation.
template<typename T>
struct Try {
  parser<T> p;
};

template<typename P>
Try( P ) -> Try<typename P::value_type>;

} // namespace parz

/****************************************************************
** Parser Promise
*****************************************************************/
namespace parz {

// We put the return_value and return_void in these two structs
// so that we can decide based on the type of T which one to in-
// clude (we are only allowed to have one in a promise type).
// When T is std::monostate we want the void one, otherwise we
// want the value one.
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
  base::maybe<base::expect<T, error>> o_;
  std::string_view                    in_       = "";
  int                                 consumed_ = 0;
  int                                 farthest_ = 0;

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

  // Note this ends in an underscore; the real return_value, if
  // present, is provided by a base class so that we can control
  // when it appears.
  void return_value_( T const& val ) {
    DCHECK( !o_ );
    o_.emplace( val );
  }

  // Note this ends in an underscore; the real return_void, if
  // present, is provided by a base class so that we can control
  // when it appears.
  void return_void_() {
    DCHECK( !o_ );
    o_.emplace( T{} );
  }

  auto await_transform( error const& w ) noexcept {
    o_ = w;
    return base::suspend_always{};
  }

  template<typename U>
  struct awaitable {
    parser<U>     parser_;
    promise_type* promise_;

    // Promise pointer so that template type can be inferred.
    awaitable( promise_type* p, parser<U> parser )
      : parser_( std::move( parser ) ), promise_( p ) {}

    bool await_ready() noexcept {
      // At this point parser_.h_ should be at its initial sus-
      // pend point, waiting to be run.
      DCHECK( !parser_.finished() );
      // Now we need to give the parser's promise object the
      // buffer to be parsed.
      parser_.resume( promise_->in_ );
      // parser should be at its final suspend point now, waiting
      // to be destroyed by the parser object that owns it.
      DCHECK( parser_.finished() );
      promise_->farthest_ =
          std::max( promise_->farthest_,
                    promise_->consumed_ + parser_.farthest() );
      return parser_.is_good();
    }

    void await_suspend( coro::coroutine_handle<> ) noexcept {
      // A parse failed.
      promise_->o_.emplace( std::move( parser_.error() ) );
    }

    U await_resume() {
      DCHECK( parser_.is_good() );
      int chars_consumed =
          promise_->in_.size() - parser_.promise_->in_.size();
      DCHECK( chars_consumed >= 0 );
      DCHECK( promise_->in_.size() >= size_t( chars_consumed ) );
      // We only consume chars upon success.
      promise_->in_.remove_prefix( chars_consumed );
      promise_->consumed_ += chars_consumed;
      return parser_.get();
    }
  };

  template<typename U>
  auto await_transform( parser<U> p ) noexcept {
    return awaitable<U>( this, std::move( p ) );
  }

  // This parser is allowed to fail.
  template<typename U>
  auto await_transform( Try<U> t ) noexcept {
    // Slight modification to awaitable to allow it to fail.
    struct tryable_awaitable : awaitable<U> {
      using Base = awaitable<U>;
      using Base::Base;
      using Base::parser_;
      using Base::promise_;

      bool await_ready() noexcept {
        Base::await_ready();
        // This is a parser that is allowed to fail, so always
        // return true;
        return true;
      }

      base::expect<U, error> await_resume() {
        if( parser_.is_error() ) return parser_.error();
        return Base::await_resume();
      }
    };
    return tryable_awaitable( this, std::move( t.p ) );
  }

  auto await_transform( next_char ) noexcept {
    struct awaitable {
      promise_type* p_;

      bool await_ready() noexcept {
        bool ready = ( p_->in_.size() > 0 );
        if( !ready ) p_->o_.emplace( error( "EOF" ) );
        return ready;
      }

      void await_suspend( coro::coroutine_handle<> ) noexcept {
        // ran out of characters in input buffer.
      }

      char await_resume() noexcept {
        CHECK_GE( int( p_->in_.size() ), 1 );
        char res = p_->in_[0];
        p_->in_.remove_prefix( 1 );
        p_->consumed_++;
        p_->farthest_ = p_->consumed_;
        return res;
      }
    };
    return awaitable{ this };
  }
};

} // namespace parz

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::parz::parser<T>, Args...> {
  using promise_type = ::parz::promise_type<T>;
};

} // namespace CORO_NS

/****************************************************************
** Primitive Parsers
*****************************************************************/
namespace parz {

parser<> parse_exact_char( char c ) {
  char next = co_await next_char{};
  if( next != c ) co_await error( "" );
}

parser<> parse_space_char() {
  char next = co_await next_char{};
  if( next != ' ' && next != '\n' ) co_await parz::error( "" );
}

parser<char> parse_any_identifier_char() {
  char c = co_await next_char{};
  if( !( ( c >= 'a' && c <= 'z' ) || c == '_' ) )
    co_await parz::error( "" );
  co_return c;
}

parser<char> parse_any_number_char() {
  char c = co_await next_char{};
  if( !( c >= '0' && c <= '9' ) ) co_await parz::error( "" );
  co_return c;
}

} // namespace parz

/****************************************************************
** Combinators
*****************************************************************/
namespace parz {

struct Repeated {
  template<typename Func>
  auto operator()( Func&& f ) const -> parser<
      vector<typename std::invoke_result_t<Func>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func>::value_type;
    vector<res_t> res;
    while( true ) {
      auto m = co_await Try{ f() };
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
    if( res.empty() ) co_await parz::error( "" );
    co_return res;
  }
};
inline constexpr Some some{};

parser<> eat_spaces() { co_await repeated( parse_space_char ); }

} // namespace parz

/****************************************************************
** Extension point.
*****************************************************************/
namespace parz {

template<typename T>
struct tag {};

template<typename T>
parser<T> parse() {
  return parser_for( tag<T>{} );
}

parser<int> parser_for( tag<int> ) {
  co_await eat_spaces();
  vector<char> chars   = co_await some( parse_any_number_char );
  string       num_str = string( chars.begin(), chars.end() );
  base::maybe<int> i   = base::stoi( num_str );
  if( !i ) co_await parz::error( "" );
  co_return *i;
}

parser<string> parser_for( tag<string> ) {
  co_await eat_spaces();
  vector<char> chars =
      co_await some( parse_any_identifier_char );
  co_return string( chars.begin(), chars.end() );
}

struct VariantParser {
  template<typename... Args>
  parser<base::variant<Args...>> operator()(
      tag<base::variant<Args...>> ) const {
    using res_t = base::variant<Args...>;
    base::maybe<res_t> res;

    auto one = [&]<typename Alt>( Alt* ) -> parser<> {
      if( res.has_value() ) co_return;
      auto exp = co_await Try{ parse<Alt>() };
      if( !exp ) co_return;
      res.emplace( std::move( *exp ) );
    };
    ( co_await one( (Args*)nullptr ), ... );

    if( !res )
      co_return parz::error();
    else
      co_return *res;
  }
};
inline constexpr VariantParser variant_parser;

} // namespace parz

/****************************************************************
** Doc Parsers
*****************************************************************/
namespace cl {

parz::parser<pair<string, value>> parse_kv() {
  string k = co_await parz::parse<string>();
  co_await parz::parse_exact_char( ':' );
  value v = co_await parz::parse<value>();
  co_return pair{ k, v };
}

table table_from_kv_pairs(
    vector<pair<string, value>> const& kv_pairs ) {
  table res;
  for( auto const& [k, v] : kv_pairs )
    res.members.push_back( pair{ k, make_shared<value>( v ) } );
  return res;
}

parz::parser<table> parser_for( parz::tag<table> ) {
  co_await parz::eat_spaces();
  co_await parz::parse_exact_char( '{' );
  auto kv_pairs = co_await parz::repeated( parse_kv );
  co_await parz::eat_spaces();
  co_await parz::parse_exact_char( '}' );
  co_return table_from_kv_pairs( kv_pairs );
}

parz::parser<value> parser_for( parz::tag<value> ) {
  co_return co_await parz::variant_parser(
      parz::tag<value::base_type>{} );
}

parz::parser<doc> parser_for( parz::tag<doc> ) {
  vector<pair<string, value>> kvs =
      co_await parz::repeated( parse_kv );
  doc res;
  res.tbl = table_from_kv_pairs( kvs );
  co_return res;
}

} // namespace cl

/****************************************************************
** Parser Runners
*****************************************************************/
struct ErrorPos {
  static ErrorPos from_index( string_view in, int idx ) {
    CHECK_LT( idx, int( in.size() ) );
    ErrorPos res{ 1, 1 };
    for( int i = 0; i < idx; ++i ) {
      ++res.col;
      if( in[i] == '\n' ) {
        ++res.line;
        res.col = 1;
      }
    }
    return res;
  }

  int line;
  int col;
};

template<typename T>
base::expect<T, parz::error> parse_from_string(
    string_view filename, string_view in ) {
  parz::parser<T> p = parz::parse<T>();
  p.promise_->in_   = in;
  p.h_.resource().resume();
  DCHECK( p.finished() );

  if( p.is_error() || p.consumed() != int( in.size() ) ) {
    // It's always one too far, not sure why.
    ErrorPos ep = ErrorPos::from_index( in, p.farthest() - 1 );
    p.result()  = parz::error( fmt::format(
         "{}:error:{}:{}: {}\n", filename, ep.line, ep.col,
        p.is_error() ? p.error()
                      : parz::error( "unexpected character" ) ) );
  }
  return p.result();
}

template<typename T>
base::expect<T, parz::error> parse_from_file(
    string_view filename ) {
  UNWRAP_CHECK( buffer,
                base::read_text_file_as_string( filename ) );
  return parse_from_string<T>( filename, buffer );
}

/****************************************************************
** main
*****************************************************************/
int main( int /*unused*/, char** /*unused*/ ) {
  constexpr string_view filename = "input.txt";
  rn::linker_dont_discard_module_error();

  util::StopWatch watch;
  watch.start( "parsing" );
  UNWRAP_CHECK( res, parse_from_file<cl::doc>( filename ) );
  watch.stop( "parsing" );

  fmt::print( "{}", res );
  fmt::print( "\n\n" );
  fmt::print( "parsing took {}.\n", watch.human( "parsing" ) );
  return 0;
}
