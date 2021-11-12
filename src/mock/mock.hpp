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

// base
#include "base/maybe.hpp"

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <queue>

namespace mock {

/****************************************************************
** Helper Types
*****************************************************************/
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

/****************************************************************
** Matchers
*****************************************************************/
struct Any {};

template<typename T>
struct Matcher {
  Matcher() = delete;
  Matcher( T val ) : o_( std::move( val ) ) {}
  Matcher( Any ) : o_{} {}

  base::maybe<T> o_;

  bool matches( T const& val ) const {
    if( !o_.has_value() ) return true;
    return val == *o_;
  }

  bool operator==( T const& val ) const {
    return matches( val );
  }
};

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
** MOCK_METHOD
*****************************************************************/
#define MOCK_ADD_MATCHER( type, var ) ::mock::Matcher<type>
#define MOCK_TO_FN_ARGS( type, var ) type var
#define MOCK_GET_ARG_VARS( type, var ) var

#define MOCK_METHOD( ret_type, fn_name, fn_args,      \
                     const_modifier )                 \
  EVAL( MOCK_METHOD_IMPL( ret_type, fn_name, fn_args, \
                          const_modifier ) )

#define MOCK_METHOD_IMPL( ret_type, fn_name, fn_args,           \
                          const_modifier )                      \
  using ret_type__##fn_name   = ret_type;                       \
  using args_tuple__##fn_name = std::tuple<PP_MAP_TUPLE_COMMAS( \
      PP_PAIR_TAKE_FIRST, PP_REMOVE_PARENS fn_args )>;          \
  using args_matchers_tuple__##fn_name =                        \
      std::tuple<PP_MAP_TUPLE_COMMAS(                           \
          MOCK_ADD_MATCHER, PP_REMOVE_PARENS fn_args )>;        \
                                                                \
  struct caller__##fn_name {                                    \
    base::maybe<RetHolder<ret_type__##fn_name>> ret_ = {};      \
    args_matchers_tuple__##fn_name              args_;          \
                                                                \
    caller__##fn_name( args_matchers_tuple__##fn_name&& args )  \
      : args_( std::move( args ) ) {}                           \
                                                                \
    ret_type__##fn_name operator()(                             \
        args_tuple__##fn_name const& args ) {                   \
      INFO(                                                     \
          fmt::format( "mock function call with "               \
                       "unexpected arguments: {}{}",            \
                       #fn_name, args ) );                      \
      REQUIRE( args == args_ );                                 \
      if constexpr( !std::is_same_v<ret_type__##fn_name,        \
                                    void> ) {                   \
        BASE_CHECK( ret_.has_value(),                           \
                    "return value not set for {}", #fn_name );  \
        return ret_->get();                                     \
      }                                                         \
    }                                                           \
                                                                \
    caller__##fn_name& returns(                                 \
        RetHolder<ret_type__##fn_name> val ) {                  \
      ret_ = std::move( val );                                  \
      return *this;                                             \
    }                                                           \
  };                                                            \
                                                                \
  mutable std::queue<caller__##fn_name> answers__##fn_name =    \
      {};                                                       \
  mutable base::maybe<caller__##fn_name>                        \
      answer_all__##fn_name = {};                               \
                                                                \
  exhaust_checker<std::queue<caller__##fn_name>>                \
      checker__##fn_name = &answers__##fn_name;                 \
                                                                \
  caller__##fn_name& add__##fn_name##__impl(                    \
      args_matchers_tuple__##fn_name args ) {                   \
    answers__##fn_name.push(                                    \
        caller__##fn_name( std::move( args ) ) );               \
    return answers__##fn_name.back();                           \
  }                                                             \
                                                                \
  caller__##fn_name& set__##fn_name##__impl(                    \
      args_matchers_tuple__##fn_name args ) {                   \
    answer_all__##fn_name =                                     \
        caller__##fn_name( std::move( args ) );                 \
    return *answer_all__##fn_name;                              \
  }                                                             \
                                                                \
  template<typename... Args>                                    \
  caller__##fn_name& add__##fn_name( Args&&... args ) {         \
    return add__##fn_name##__impl(                              \
        { std::forward<Args>( args )... } );                    \
  }                                                             \
                                                                \
  template<typename... Args>                                    \
  caller__##fn_name& set__##fn_name( Args&&... args ) {         \
    return set__##fn_name##__impl(                              \
        { std::forward<Args>( args )... } );                    \
  }                                                             \
                                                                \
  ret_type__##fn_name fn_name( PP_MAP_TUPLE_COMMAS(             \
      MOCK_TO_FN_ARGS, PP_REMOVE_PARENS fn_args ) )             \
      PP_REMOVE_PARENS const_modifier override {                \
    if( !answers__##fn_name.empty() ) {                         \
      auto f = std::move( answers__##fn_name.front() );         \
      answers__##fn_name.pop();                                 \
      return f( { PP_MAP_TUPLE_COMMAS(                          \
          MOCK_GET_ARG_VARS, PP_REMOVE_PARENS fn_args ) } );    \
    }                                                           \
    if( answer_all__##fn_name.has_value() )                     \
      return answer_all__##fn_name->operator()(                 \
          { PP_MAP_TUPLE_COMMAS(                                \
              MOCK_GET_ARG_VARS,                                \
              PP_REMOVE_PARENS fn_args ) } );                   \
    FATAL(                                                      \
        "unexpected mock function call: {}{}", #fn_name,        \
        args_tuple__##fn_name{ PP_MAP_TUPLE_COMMAS(             \
            MOCK_GET_ARG_VARS, PP_REMOVE_PARENS fn_args ) } );  \
  }

} // namespace mock
