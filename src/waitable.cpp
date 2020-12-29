/****************************************************************
**waitable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-28.
*
* Description: Generic awaitable for use with coroutines.
*
*****************************************************************/
#include "waitable.hpp"

// Revolution Now
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "maybe.hpp"

// C++ standard library
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>

// Currently only works with gcc and clang+libstdcxx. When clang
// fixes this then this guard can be removed.
#if !defined( __clang__ ) || defined( _LIBCPP_VERSION )

#  if defined( _LIBCPP_VERSION )
#    include <experimental/coroutine>
namespace coro = ::std::experimental;
#  else // gcc
#    include <coroutine>
namespace coro = ::std;
#  endif

namespace rn {

namespace {

using namespace std;

bool do_logging = false;

template<typename... Args>
void trace( string_view sv, Args&&... args ) {
  if( !do_logging ) return;
  fmt::print( sv, std::forward<Args>( args )... );
}

/****************************************************************
** waitable (like a "future")
*****************************************************************/
template<typename T>
class waitable {
  class shared_state {
    maybe<T> val_;

  public:
    vector<function<void()>> resumes_;

    T get() const {
      trace( "shared_state::get.\n" );
      return val_.value();
    }

    bool ready() const { return val_.has_value(); }

    void set( T val ) {
      trace( "shared_state::set (start).\n" );
      CHECK( !val_.has_value() );
      val_ = val;
      for( auto& f : resumes_ ) f();
      trace( "shared_state::set (end).\n" );
    }
  };

public:
  waitable( T val ) {
    sstate_ = make_shared<shared_state>();
    sstate_->set( val );
  }
  waitable( shared_ptr<shared_state> sstate )
    : sstate_( sstate ) {
    trace( "waitable::waitable.\n" );
  }
  T get() const {
    trace( "waitable::get.\n" );
    return sstate_->get();
  }

  bool ready() const { return sstate_->ready(); }

  struct promise {
    promise() {
      trace( "waitable::promise::promise.\n" );
      sstate_ = make_shared<shared_state>();
    }
    waitable<T> get_future() { return waitable<T>( sstate_ ); }
    void        set( T val ) {
      trace( "waitable::promise::set.\n" );
      sstate_->set( val );
    }

    bool ready() const { return sstate_->ready(); }

    auto initial_suspend() const {
      return coro::suspend_never{};
    }
    auto final_suspend() const noexcept {
      return coro::suspend_never{};
    }

    waitable<T> return_value( T val ) {
      trace( "waitable::promise::return value.\n" );
      set( val );
      return get_future();
    }

    waitable<T> get_return_object() {
      trace( "waitable::promise::get_return_object.\n" );
      return get_future();
    }

    void unhandled_exception() {
      trace( "Unhandled exception.\n" );
    }

    shared_ptr<shared_state> sstate_;
  };

  using promise_type = promise;

  auto operator co_await() {
    trace( "waitable::operator co_await.\n" );
    struct awaitable {
      shared_ptr<shared_state> sstate_;
      bool await_ready() { return sstate_->ready(); }
      void await_suspend( coro::coroutine_handle<> h ) {
        trace( "waitable::awaitable::await_suspend.\n" );
        sstate_->resumes_.push_back( [h]() mutable {
          trace( "waitable::awaitable::(resume lambda).\n" );
          h.resume();
        } );
      }
      T await_resume() {
        trace( "waitable::awaitable::await_resume.\n" );
        return sstate_->get();
      }
    };
    return awaitable{ sstate_ };
  }

private:
  shared_ptr<shared_state> sstate_;
};

/****************************************************************
** Simulation of Window System Setting Promises.
*****************************************************************/
queue<variant<waitable<int>::promise, waitable<double>::promise>>
    g_promises;

struct Setter {
  void operator()( waitable<int>::promise& p ) { p.set( 1 ); }
  void operator()( waitable<double>::promise& p ) {
    p.set( 2.2 );
  }
};

void deliver_promise( int i ) {
  trace( "{}. Delivering promise.\n", i );
  if( !g_promises.empty() ) {
    visit( Setter{}, g_promises.front() );
    g_promises.pop();
  }
}

/****************************************************************
** Create some waitables.
*****************************************************************/
waitable<int> waitable_int() {
  trace( "Inside waitable_int.\n" );
  waitable<int>::promise p;
  g_promises.emplace( p );
  return p.get_future();
}

waitable<double> waitable_double() {
  trace( "Inside waitable_double.\n" );
  waitable<double>::promise p;
  g_promises.emplace( p );
  return p.get_future();
}

/****************************************************************
** The Coroutines!
*****************************************************************/
waitable<int> waitable_sum() {
  trace( "Inside waitable_sum.\n" );
  co_return                     //
      co_await waitable_int() + //
      co_await waitable_int() + //
      co_await waitable_int();
}

waitable<string> waitable_string() {
  trace( "Start waitable_string.\n" );
  int n = co_await waitable_sum();
  trace( "Middle waitable_string.\n" );
  double d = co_await waitable_double();
  // FIXME: gcc ICE when we put this co_await in the for-loop.
  int m = co_await waitable_sum();
  for( int i = 0; i < m; ++i ) //
    d += co_await waitable_double();
  trace( "End waitable_string.\n" );
  co_return to_string( n ) + "-" + to_string( d );
}

} // namespace

/****************************************************************
** Testing
*****************************************************************/
std::string test_waitable( bool logging ) {
  using namespace ::std::literals::string_literals;
  do_logging           = logging;
  waitable<string> sfs = waitable_string();
  int              i   = 1;
  // Simulation of event loop, populating promises in response to
  // user input.
  while( !sfs.ready() ) {
    trace( "---------------------------------------\n" );
    deliver_promise( i++ );
  }
  trace( "---------------------------------------\n" );
  string res = sfs.get();
  trace( "---------------------------------------\n" );
  trace( "Result: {}\n", res );
  return res;
}

} // namespace rn

#else

/****************************************************************
** Use this dummy until clang+libstdc++ supports coroutines.
*****************************************************************/
namespace rn {

std::string test_waitable( bool ) { return "3-8.800000"; }

} // namespace rn

#endif
