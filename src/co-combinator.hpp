/****************************************************************
**co-combinator.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Combinators for waitables.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "maybe.hpp"
#include "waitable-coro.hpp"

// base
#include "base/meta.hpp"
#include "base/unique-func.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <concepts>
#include <queue>
#include <vector>

namespace rn::co {

/****************************************************************
** Disjunctive Links
*****************************************************************/
// This is a specialized function for use in chaining together
// waitables manually (meaning not through coroutines) in a
// many-to-one, logically disjunctive manner, i.e. for situations
// in which a single waitable needs to wait on any of multiple
// other waitables. In other words, we want to detect when the
// first of them finishes. It is mostly used in coroutine combi-
// nators, you probably should not be using this outside of those
// implementations.
template<typename T, typename U>
void disjunctive_link_to_promise( waitable<T>&        w,
                                  waitable_promise<U> wp ) {
  w.shared_state()->add_callback(
      [wp]( typename waitable<T>::value_type const& o ) {
        // The "if-not-set" reflects the intended disjunctive na-
        // ture of the use of this function: we are only taking
        // the first result available and ignoring the rest.
        wp.set_value_emplace_if_not_set( o );
      } );
  w.shared_state()->set_exception_callback(
      [wp]( std::exception_ptr eptr ) {
        wp.set_exception( eptr );
      } );
}

// Same as above but works when the value type is a variant. Ac-
// tually, this specialized one is only needed for variants that
// have some duplicate types where the value has to be set on the
// variant using a specified index, although it will work for
// variants with non-duplicate types as well.
template<int Index, typename T, typename U>
void disjunctive_link_to_variant_promise(
    waitable<T>& w, waitable_promise<U> wp ) {
  static_assert( base::is_base_variant_v<U> );
  w.shared_state()->add_callback(
      [wp]( typename waitable<T>::value_type const& o ) {
        // The "if-not-set" reflects the intended disjunctive na-
        // ture of the use of this function: we are only taking
        // the first result available and ignoring the rest.
        wp.set_value_emplace_if_not_set(
            std::in_place_index_t<Index>{}, o );
      } );
  w.shared_state()->set_exception_callback(
      [wp]( std::exception_ptr eptr ) {
        wp.set_exception( eptr );
      } );
}

/****************************************************************
** any
*****************************************************************/
// Returns a waitable that will be ready when (and as soon as)
// the first waitable becomes ready. Since this function takes
// ownership of all of the waitables, they will be gone when this
// function becomes ready, and thus any that are not ready will
// be cancelled.
waitable<> any( std::vector<waitable<>> ws );

waitable<> any( waitable<>&& w );
waitable<> any( waitable<>&& w1, waitable<>&& w2 );
waitable<> any( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 );

/****************************************************************
** all
*****************************************************************/
// FIXME: add unit tests for this.
waitable<> all( std::vector<waitable<>> ws );

waitable<> all( waitable<>&& w );
waitable<> all( waitable<>&& w1, waitable<>&& w2 );
waitable<> all( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 );

/****************************************************************
** first
*****************************************************************/
struct First {
  // Run the waitables ws in parallel, then return the result of
  // the first one that finishes. NOTE: The values of any other
  // waitables that become ready at the same time will be lost.
  template<size_t... Idxs, typename... Ts>
  waitable<base::variant<Ts...>> operator()(
      std::index_sequence<Idxs...>, waitable<Ts>... ws ) const {
    waitable_promise<base::variant<Ts...>> wp;
    ( disjunctive_link_to_variant_promise<Idxs>( ws, wp ), ... );
    // !! Need to co_await instead of just returning the waitable
    // because we need to keep the waitables alive.
    co_return co_await wp.waitable();
  }

  template<typename... Ts>
  waitable<base::variant<Ts...>> operator()(
      waitable<Ts>... ws ) const {
    return (*this)( std::make_index_sequence<sizeof...( Ts )>(),
                    std::move( ws )... );
  }
};

inline constexpr First first{};

/****************************************************************
** background
*****************************************************************/
struct WithBackground {
  // Run the waitable w in parallel with the background task,
  // until w becomes ready, at which point return w's value. It
  // is inconsequential whether the background task finishes
  // early or not. If background is still running when w fin-
  // ishes, it will naturally be cancelled as it will go out of
  // scope.
  //
  // The value of using this function is that:
  //
  //   1. It does a co_await, so that way the calling function
  //      can just use a plain `return` if it wants to, without
  //      then causing the background thread to go out of scope
  //      and terminate prematurely.
  //   2. It will force the background thread to terminate when w
  //      terminates, without the caller having to manage that.
  //
  template<typename T>
  waitable<T> operator()( waitable<T> w,
                          waitable<> /*background*/ ) const {
    waitable_promise<T> wp;
    // Need to do w first so that if both are ready already then
    // w will take precedence and return its value.
    disjunctive_link_to_promise( w, wp );
    // !! Need to co_await instead of just returning the waitable
    // because we need to keep the waitables alive.
    co_return co_await wp.waitable();
  }
};

inline constexpr WithBackground background{};

/****************************************************************
** fmap
*****************************************************************/
struct Fmap {
  // Needs to take Func by value because it needs to keep it
  // around until the waitable w is ready.
  template<typename Func, typename T>
  auto operator()( Func f, waitable<T> w ) const
      -> waitable<std::invoke_result_t<Func, T>> {
    co_return f( co_await std::move( w ) );
  }
};

inline constexpr Fmap fmap{};

/****************************************************************
** try
*****************************************************************/
template<typename Exception>
struct Try {
  // Call the function given by the first argument in a try/catch
  // block that catches exceptions of type Exception. In the
  // event of an exception, the function given by the second ar-
  // gument is called with the exception object as an argument.
  //
  // Note: the reason that we take a function as the first argu-
  // ment and not a waitable is because if we took a waitable
  // then it would have to be created in the caller's frame,
  // which means that we wouldn't be able to catch exceptions
  // that happen before the first suspension point (since our
  // coroutines start running eagerly).
  //
  // Example:
  //
  //   maybe<int> m = co_await co::try_<runtime_error>(
  //       /*try=*/[] {
  //         ...
  //       },
  //       /*catch=*/[]( runtime_error const& e ) {
  //         ...
  //       } );
  //
  template<typename TryFunc, typename CatchFunc>
  auto operator()( TryFunc&& body, CatchFunc&& catcher ) const
      -> waitable<maybe<
          typename std::invoke_result_t<TryFunc>::value_type>> {
    using result_t = maybe<
        typename std::invoke_result_t<TryFunc>::value_type>;
    result_t res;
    try {
      // Must co_await here instead of just returning since oth-
      // erwise we will not catch exceptions that are thrown
      // after the first suspension.
      res = co_await std::forward<TryFunc>( body )();
    } catch( Exception const& e ) {
      std::forward<CatchFunc>( catcher )( e );
    }
    co_return res;
  }

  // A version that does nothing when an exception is caught.
  template<typename TryFunc>
  auto operator()( TryFunc&& body ) const {
    return operator()( std::forward<TryFunc>( body ),
                       []( Exception const& ) {} );
  }
};

template<typename Exception>
inline constexpr Try<Exception> try_{};

/****************************************************************
** Erase
*****************************************************************/
// Wait for a waitable but ignore the result.
struct Erase {
  template<typename T>
  waitable<> operator()( waitable<T> w ) const {
    (void)co_await std::move( w );
  }
};

inline constexpr Erase erase{};

/****************************************************************
** loop
*****************************************************************/
waitable<> loop(
    base::unique_func<waitable<>() const> coroutine );

/****************************************************************
** repeater
*****************************************************************/
// Given a function that produces a waitable, this object will
// take ownership of the function, then repeatedly call the func-
// tion to obtain a waitable.
template<typename T>
struct repeater {
  using value_type = T;

  repeater( base::unique_func<waitable<T>() const> producer )
    : producer_( std::move( producer ) ) {}

  // Implement the Streamable concept interface.
  waitable<T> next() { return producer_(); }

  base::unique_func<waitable<T>() const> producer_;
};

template<typename Func>
repeater( Func&& )
    -> repeater<typename std::invoke_result_t<Func>::value_type>;

/****************************************************************
** latch
*****************************************************************/
// A latch is meant to be a singlton object (per use-case) that
// can be awaited on by multiple things. When it is set, all
// awaiters will be resumed, and it will remain set until reset.
struct latch {
  void set() { p.set_value_emplace_if_not_set(); }
  void reset() { p = {}; }

  auto waitable() const { return p.waitable(); }

  auto operator co_await() const noexcept {
    return detail::awaitable(
        static_cast<detail::promise_type<std::monostate>*>(
            nullptr ),
        waitable() );
  }

  waitable_promise<> p;
};

/****************************************************************
** ticker
*****************************************************************/
// A ticker is meant to be a singlton object (per use-case) that
// can be awaited on by multiple things. When it is ticked, all
// the awaiters will be resumed, and it will be reset, so anyone
// who then starts newly awaiting on it will have to wait until
// the next tick.
struct ticker {
  void tick() {
    p.set_value_emplace();
    p = {};
  }

  waitable<> wait() const { return p.waitable(); }

private:
  waitable_promise<> p;
};

/****************************************************************
** stream
*****************************************************************/
template<typename T>
struct stream {
  using value_type = T;

  // Implement the Streamable concept interface.
  waitable<T> next() {
    T res = co_await p.waitable();
    p     = {};
    update();
    co_return res;
  }

  void send( T const& t ) {
    q.push( t );
    update();
  }

  void send( T&& t ) {
    q.emplace( std::move( t ) );
    update();
  }

  bool ready() const { return p.has_value(); }

  void reset() {
    *this = {};
    // Not necessary, but for consistency.
    update();
  }

  // For testing; not sure if this would be useful otherwise.
  void set_exception() {
    p.set_exception( std::runtime_error( "co::stream" ) );
  }

  stream()                = default;
  stream( stream const& ) = delete;
  stream& operator=( stream const& ) = delete;
  stream( stream&& )                 = default;
  stream& operator=( stream&& ) = default;

private:
  void update() {
    if( !p.has_value() && !q.empty() ) {
      p.set_value_emplace( std::move( q.front() ) );
      q.pop();
    }
  }

  waitable_promise<T> p;
  std::queue<T>       q;
};

/****************************************************************
** finite_stream
*****************************************************************/
template<typename T>
struct finite_stream {
  using value_type = maybe<T>;

  // Implement the Streamable concept interface.
  waitable<value_type> next() {
    if( ended ) co_return nothing;
    maybe<T> res = co_await s.next();
    if( !res ) {
      ended = true;
      s.reset();
      co_return nothing;
    }
    co_return res;
  }

  void send( T const& t ) { s.send( t ); }
  void send( T&& t ) { s.send( std::move( t ) ); }
  void finish() { s.send( nothing ); }

  void reset() { *this = {}; }

  // For testing; not sure if this would be useful otherwise.
  void set_exception() { s.set_exception(); }

private:
  bool               ended = false;
  stream<value_type> s;
};

/****************************************************************
** Adapter: waitable to streamable
*****************************************************************/
// This is an adapter that takes a waitable and makes it into
// something streamable (implementing the Streamble concept).
// However, the resulting "stream" will only produce one object,
// so the resulting object is a kind of latch but that cannot be
// manually set.
template<typename T>
struct one_shot_stream_adapter {
  using value_type = T;

  one_shot_stream_adapter( waitable<T>&& w )
    : w_( std::move( w ) ) {}

  // Implement the Streamable concept interface.
  waitable<T> next() {
    if( retrieved_ )
      // A waitable that will never be fulfilled.
      return waitable_promise<T>().waitable();
    retrieved_ = true;
    return std::move( w_ );
  }

private:
  waitable<T> w_;
  bool        retrieved_ = false;
};

/****************************************************************
** make_streamable
*****************************************************************/
// These funcctions will take non-streamable objects and adapt
// them to have the streamable interface.

template<typename T>
auto make_streamable( waitable<T>&& w ) {
  return one_shot_stream_adapter( std::move( w ) );
}

/****************************************************************
** Stream
*****************************************************************/
template<typename T>
concept Streamable = requires( T s ) {
  typename T::value_type;
  { s.next() } -> std::same_as<waitable<typename T::value_type>>;
};

/****************************************************************
** interleave
*****************************************************************/
// This takes a series of streamable things (i.e., things that
// conform to the Streamable concept) and it will interleave
// their results (without dropping any values) int a single
// stream. The order in which two results from different streams
// are interleaved in the output stream is unspecified if those
// values become ready simultaneously, though neither will get
// dropped.
//
// NOTE: this interleaver guarantees that no values from any
// stream will be dropped, so long as the interleave object isn't
// destroyed while any of the input streams have upcoming values.
//
// Example:
//
//   // These don't have to be all co::stream.
//   co::stream<int> s1;
//   co::stream<double> s2;
//   co::stream<string> s3;
//
//   co::interleave il( s1, s2, s3 );
//
//   ... send data into s1, s2, s3 ...
//
//   waitable<base::variant<int, double, string>> w = il.next();
//
// NOTE: duplicate types are supported.
//
template<Streamable... Ss>
struct interleave {
  using value_type = base::variant<typename Ss::value_type...>;

  // Implement the Streamable concept interface.
  waitable<value_type> next() { return output_stream.next(); }

  explicit interleave( Ss&... ss ) : streamables{ &ss... } {
    // This lambda needs access to `this`, but we can't capture
    // it because we need it to be captureless, because it goes
    // out of scope at the end of this function, but we need it
    // to outlive this function. So therefore we just pass in
    // this as an argument.
    auto forwarder = []<size_t Index>(
                         std::integral_constant<size_t, Index>,
                         auto* that ) -> waitable<> {
      while( true )
        that->output_stream.send( value_type(
            std::in_place_index_t<Index>{},
            co_await std::get<Index>( that->streamables )
                ->next() ) );
    };
    // Start N coroutines and store them in the vector.
    mp::for_index_seq<sizeof...( Ss )>(
        [&, this]<size_t Idx>(
            std::integral_constant<size_t, Idx> ic ) {
          forwarders.push_back( forwarder( ic, this ) );
        } );
  }

  interleave()                    = default;
  interleave( interleave const& ) = delete;
  interleave& operator=( interleave const& ) = delete;
  // Cannot be moved because this object contains a self refer-
  // ence, due to the above lambda capture of `this`.
  interleave( interleave&& ) = delete;
  interleave& operator=( interleave&& ) = delete;

private:
  // Input streamables.
  std::tuple<Ss*...> streamables;
  // This is a stream that supplies the output to the interleave.
  stream<value_type> output_stream;
  // Holds ownership of a list of waitables that each own a
  // coroutine whose job it is to monitor a given stream and for-
  // ward its events into `output_stream`.
  std::vector<waitable<>> forwarders;
};

/****************************************************************
** detect_suspend
*****************************************************************/
template<typename T>
struct ResultWithSuspend {
  T    result;
  bool suspended;
};

struct DetectSuspend {
  // Wrap a waitable in this in order to detect whether it sus-
  // pends in the process of computing its result.
  template<typename T>
  waitable<ResultWithSuspend<T>> operator()(
      waitable<T>&& w ) const {
    ResultWithSuspend<T> res;
    res.suspended = !w.ready();
    res.result    = co_await std::move( w );
    co_return res;
  }
};

inline constexpr DetectSuspend detect_suspend{};

} // namespace rn::co
