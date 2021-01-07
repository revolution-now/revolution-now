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
#include "error.hpp"
#include "fmt-helper.hpp"
#include "maybe.hpp"

// base
#include "base/co-compat.hpp"

// C++ standard library
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <variant>
#include <vector>

namespace rn {
namespace {

using namespace std;

/****************************************************************
** waitable (like a "future")
*****************************************************************/
template<typename T>
class waitable {
  class shared_state {
    maybe<T> val_;

  public:
    vector<function<void()>> resumes_;

    T get() const { return val_.value(); }

    bool ready() const { return val_.has_value(); }

    void set( T val ) {
      CHECK( !val_.has_value() );
      val_ = val;
      for( auto& f : resumes_ ) f();
    }
  };

public:
  using value_type = T;

  waitable( T val ) {
    sstate_ = make_shared<shared_state>();
    sstate_->set( val );
  }
  waitable( shared_ptr<shared_state> sstate )
    : sstate_( sstate ) {}
  T get() const { return sstate_->get(); }

  bool ready() const { return sstate_->ready(); }

  struct promise;
  using promise_type = promise;

  struct awaitable;

  auto operator co_await() { return awaitable{ sstate_ }; }

private:
  shared_ptr<shared_state> sstate_;
};

template<typename T>
struct waitable<T>::awaitable {
  shared_ptr<shared_state> sstate_;
  bool await_ready() { return sstate_->ready(); }
  void await_suspend( coro::coroutine_handle<> h ) {
    sstate_->resumes_.push_back( [h]() mutable { h.resume(); } );
  }
  T await_resume() { return sstate_->get(); }
};

template<typename T>
struct waitable<T>::promise {
  promise() { sstate_ = make_shared<shared_state>(); }
  waitable<T> get_future() { return waitable<T>( sstate_ ); }
  void        set( T val ) { sstate_->set( val ); }

  bool ready() const { return sstate_->ready(); }

  auto initial_suspend() const { return base::suspend_never{}; }
  auto final_suspend() const noexcept {
    return base::suspend_never{};
  }

  void return_value( T val ) { set( val ); }

  waitable<T> get_return_object() { return get_future(); }

  void unhandled_exception() { SHOULD_NOT_BE_HERE; }

  shared_ptr<shared_state> sstate_;
};

/****************************************************************
** Simulation of Window System Setting Promises.
*****************************************************************/
queue<variant<waitable<int>::promise, waitable<double>::promise>>
    g_promises;

void deliver_promise() {
  struct Setter {
    void operator()( waitable<int>::promise& p ) { p.set( 1 ); }
    void operator()( waitable<double>::promise& p ) {
      p.set( 2.2 );
    }
  };
  if( !g_promises.empty() ) {
    visit( Setter{}, g_promises.front() );
    g_promises.pop();
  }
}

/****************************************************************
** Create some waitables.
*****************************************************************/
waitable<int> waitable_int() {
  waitable<int>::promise p;
  g_promises.emplace( p );
  return p.get_future();
}

waitable<double> waitable_double() {
  waitable<double>::promise p;
  g_promises.emplace( p );
  return p.get_future();
}

/****************************************************************
** The Coroutines!
*****************************************************************/
waitable<int> waitable_sum() {
  co_return                     //
      co_await waitable_int() + //
      co_await waitable_int() + //
      co_await waitable_int();
}

template<typename Func, typename... Args>
auto co_invoke( Func&& func, Args... args )
    -> waitable<decltype( std::forward<Func>( func )(
        std::declval<typename Args::value_type>()... ) )> {
  co_return std::forward<Func>( func )( ( co_await args )... );
}

template<typename Func>
struct co_lift {
  Func func_;
  co_lift( Func&& func ) : func_( std::move( func ) ) {}
  co_lift( Func const& func ) : func_( func ) {}
  template<typename... Args>
  auto operator()( Args... args ) {
    return co_invoke( func_, args... );
  }
};

waitable<string> waitable_string() {
  int    n = co_await waitable_sum();
  double d = co_await waitable_double();

  int m = co_await waitable_sum();
  for( int i = 0; i < m; ++i ) //
    d += co_await waitable_double();

  int sum = co_await co_lift{ std::plus<>{} }( waitable_sum(),
                                               waitable_sum() );

  auto f = [&]() -> waitable<int> {
    int res = co_await waitable_sum() *
              int( co_await waitable_double() );
    co_return res;
  };
  int z = co_await f() + sum;

  co_return to_string( n ) + "-" + to_string( z ) + "-" +
      to_string( d );
}

} // namespace

/****************************************************************
** Testing
*****************************************************************/
std::string test_waitable() {
  waitable<string> sfs = waitable_string();
  while( !sfs.ready() ) deliver_promise();
  return sfs.get();
}

} // namespace rn
