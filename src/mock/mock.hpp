/****************************************************************
**mock.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-11.
*
* Description: Mocking Framework.
*
*****************************************************************/
#pragma once

// mock
#include "matcher.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/maybe.hpp"

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <queue>

/****************************************************************
** EXPECT* Macros
*****************************************************************/
// This one sets a method that will (and must) be called exactly
// once, at which point it will be removed.
#define EXPECT_CALL( obj, method_with_args ) \
  obj.add__##method_with_args

// This one sets a (single) method that will be repeatedly called
// any number of times when there are no more EXPECT_CALLs.
#define EXPECT_MULTIPLE_CALLS( obj, method_with_args ) \
  obj.set__##method_with_args

/****************************************************************
** MOCK_METHOD Macros
*****************************************************************/
#define MOCK_TO_FN_ARGS( type, var ) type var
#define MOCK_GET_ARG_VARS( type, var ) var

#define MOCK_METHOD( ret_type, fn_name, fn_args,      \
                     const_modifier )                 \
  EVAL( MOCK_METHOD_IMPL( ret_type, fn_name, fn_args, \
                          const_modifier ) )

#define MOCK_METHOD_IMPL( ret_type, fn_name, fn_args,           \
                          const_modifier )                      \
  using responder__##fn_name = ::mock::detail::Responder<       \
      ret_type,                                                 \
      std::tuple<PP_MAP_TUPLE_COMMAS(                           \
          PP_PAIR_TAKE_FIRST, PP_REMOVE_PARENS fn_args )>,      \
      decltype( std::index_sequence_for<PP_MAP_TUPLE_COMMAS(    \
                    PP_PAIR_TAKE_FIRST,                         \
                    PP_REMOVE_PARENS fn_args )>() )>;           \
                                                                \
  mutable ::mock::detail::ResponderQueues<responder__##fn_name> \
      queues__##fn_name = { #fn_name };                         \
                                                                \
  template<typename... Args>                                    \
  requires std::is_constructible_v<                             \
      typename responder__##fn_name::matchers_t, Args...>       \
      responder__##fn_name& add__##fn_name( Args&&... args ) {  \
    auto matchers = responder__##fn_name::matchers_t{           \
        std::forward<Args>( args )... };                        \
    return queues__##fn_name.add( std::move( matchers ) );      \
  }                                                             \
                                                                \
  template<typename... Args>                                    \
  requires std::is_constructible_v<                             \
      typename responder__##fn_name::matchers_t, Args...>       \
      responder__##fn_name& set__##fn_name( Args&&... args ) {  \
    auto matchers = responder__##fn_name::matchers_t{           \
        std::forward<Args>( args )... };                        \
    return queues__##fn_name.set( std::move( matchers ) );      \
  }                                                             \
                                                                \
  ret_type fn_name( PP_MAP_TUPLE_COMMAS(                        \
      MOCK_TO_FN_ARGS, PP_REMOVE_PARENS fn_args ) )             \
      PP_REMOVE_PARENS const_modifier override {                \
    return queues__##fn_name( PP_MAP_TUPLE_COMMAS(              \
        MOCK_GET_ARG_VARS, PP_REMOVE_PARENS fn_args ) );        \
  }

namespace mock {

/****************************************************************
** Mocking
*****************************************************************/
namespace detail {

template<typename T>
struct RetHolder {
  RetHolder( T val ) : o( std::move( val ) ) {}
  T&& get() { return std::move( o ); }
  T   o;
};

template<>
struct RetHolder<void> {
  void get() {}
};

template<typename T>
struct exhaust_checker {
  T* p_;
  exhaust_checker( T* p ) : p_( p ) {}
  ~exhaust_checker() {
    BASE_CHECK( p_->empty(),
                "not all expected calls have been called." );
  }
};

template<typename T>
concept SettablePointer = std::is_pointer_v<T> &&
    !std::is_const_v<std::remove_pointer_t<T>>;

template<typename T>
concept SettableReference = std::is_reference_v<T> &&
    !std::is_const_v<std::remove_reference_t<T>>;

template<typename T>
concept Settable = SettablePointer<T> || SettableReference<T>;

struct None {};

template<typename...>
struct Responder;

// This is the object that holds the info needed to respond to a
// mock call in the various possible ways that that can be done.
template<typename RetT, typename... Args, size_t... Idx>
struct Responder<RetT, std::tuple<Args...>,
                 std::index_sequence<Idx...>> {
public:
  using args_t      = std::tuple<Args...>;
  using args_refs_t = std::tuple<Args const&...>;
  using matchers_t  = std::tuple<MatcherWrapper<Args>...>;
  using setters_t   = std::tuple<
      std::conditional_t<Settable<Args>,
                         base::maybe<std::remove_reference_t<
                             std::remove_pointer_t<Args>>>,
                         None>...>;

  Responder( std::string fn_name, matchers_t&& args )
    : fn_name_( std::move( fn_name ) ),
      matchers_( std::move( args ) ) {}

  RetT operator()( args_refs_t const& args ) {
    auto format_if_possible
        [[maybe_unused]] = []<typename T>( T&& o ) {
          std::string res = "<non-formattable>";
          if constexpr( base::has_fmt<std::remove_cvref_t<T>> )
            res = fmt::to_string( o );
          return res;
        };

    std::string formatted_args;
    ( ( formatted_args +=
        format_if_possible( std::get<Idx>( args ) ) + ", " ),
      ... );
    if( !formatted_args.empty() )
      formatted_args.resize( formatted_args.size() - 2 );

    // 1. Check if the arguments match.
    BASE_CHECK(
        args == matchers_,
        "mock function call with unexpected arguments: {}( {} )",
        fn_name_, formatted_args );

    // 2. Set any output parameters that need to be set.
    if( setters_.has_value() ) {
      auto setter [[maybe_unused]] = []<typename T, typename U>(
                                         T&& src, U& dst ) {
        if constexpr( !std::is_same_v<std::remove_reference_t<T>,
                                      None> ) {
          if constexpr( std::is_pointer_v<U> )
            *dst = *src;
          else // reference
            dst = *src;
        }
      };
      ( setter( std::get<Idx>( *setters_ ),
                std::get<Idx>( args ) ),
        ... );
    }

    // 3. Return what was requested to be returned.
    if constexpr( !std::is_same_v<RetT, void> ) {
      BASE_CHECK( ret_.has_value(),
                  "return value not set for {}.", fn_name_ );
      return ret_->get();
    }
  }

  Responder& returns( RetHolder<RetT> val ) {
    ret_ = std::move( val );
    return *this;
  }

  template<size_t Elem>
  /* clang-format off */
  requires Settable<std::tuple_element_t<Elem, args_t>>
  Responder& sets_arg(
      std::remove_reference_t<std::remove_pointer_t<
          std::tuple_element_t<Elem, args_t>>> const& val ) {
    /* clang-format on */
    if( !setters_.has_value() ) setters_.emplace();
    std::get<Elem>( *setters_ ) = val;
    return *this;
  }

private:
  base::maybe<RetHolder<RetT>> ret_ = {};
  // setters_t is wrapped in a maybe for efficiency purposes; in
  // most cases there will be no parameter setting, and so then
  // setters_ will remain `nothing` and when the mock is called,
  // we will not have to iterate through the tuple members to
  // check if there are any that need to be set.
  base::maybe<setters_t> setters_ = {};
  std::string            fn_name_;
  matchers_t             matchers_;
};

template<typename R>
struct ResponderQueues {
  std::string    fn_name_    = {};
  std::queue<R>  answers_    = {};
  base::maybe<R> answer_all_ = {};

  exhaust_checker<std::queue<R>> checker_ = &answers_;

  ResponderQueues( std::string fn_name )
    : fn_name_( std::move( fn_name ) ) {}

  R& add( typename R::matchers_t args ) {
    answers_.push( R( fn_name_, std::move( args ) ) );
    return answers_.back();
  }

  R& set( typename R::matchers_t args ) {
    answer_all_ = R( fn_name_, std::move( args ) );
    return *answer_all_;
  }

  template<typename... T>
  auto operator()( T&&... args ) {
    if( !answers_.empty() ) {
      R f = std::move( answers_.front() );
      answers_.pop();
      return f( { std::forward<T>( args )... } );
    }
    if( answer_all_.has_value() )
      return ( *answer_all_ )( { std::forward<T>( args )... } );
    FATAL( "unexpected mock function call: {}", fn_name_ );
  }
};

} // namespace detail

} // namespace mock
