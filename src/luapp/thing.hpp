/****************************************************************
**thing.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: C++ containers for Lua values/objects.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// luapp
#include "types.hpp"

// base
#include "base/fmt.hpp"
#include "base/variant.hpp"

// Lua
#include "lua.h"

// C++ standard library
#include <string>

namespace luapp {

struct lightuserdata : public base::safe::void_p {
  using Base = base::safe::void_p;

  using Base::Base;
  // using Base::operator==;
  // using Base::operator void*;
  using Base::get;
};

template<typename Derived>
struct reference {
  //
};

struct table : public reference<table> {
  //
};

struct lstring : public reference<lstring> {
  //
};

struct lfunction : public reference<lfunction> {
  //
};

struct userdata : public reference<userdata> {
  //
};

struct lthread : public reference<lthread> {
  //
};

// nil_t should be first so that it is selected as the default.
using thing_base = base::variant<nil_t,         //
                                 boolean,       //
                                 lightuserdata, //
                                 integer,       //
                                 floating,      //
                                 lstring,       //
                                 table,         //
                                 lfunction,     //
                                 userdata,      //
                                 lthread        //
                                 >;

struct thing : public thing_base {
  using Base = thing_base;

  // clang-format off
  template<typename T>
  requires( !std::is_same_v<T*, void*> )
  bool operator==( T* p ) const noexcept {
    return (*this) == static_cast<void*>(p);
  }
  // clang-format on

  template<typename T>
  bool operator==( T const& rhs ) const noexcept {
    return this->visit( [&]<typename U>( U&& elem ) {
      using elem_t  = decltype( std::forward<U>( elem ) );
      using decayed = std::remove_cvref_t<U>;
      if constexpr( std::is_same_v<T, decayed> ) {
        static_assert(
            std::equality_comparable_with<elem_t, T const&> );
        return ( std::forward<U>( elem ) == rhs );
      } else if constexpr( std::is_convertible_v<T const&,
                                                 elem_t> ) {
        return ( elem == static_cast<elem_t>( rhs ) );
      } else if constexpr( std::is_convertible_v<elem_t,
                                                 T const&> ) {
        return ( static_cast<T const&>( elem ) == rhs );
      } else {
        return false;
      }
    } );
  }

  // Follows Lua's rules, where every value is true except for
  // boolean:false and nil.
  operator bool() const noexcept;

  template<typename T>
  bool operator!=( T const& rhs ) const noexcept {
    return !( *this == rhs );
  }

  e_lua_type type() const noexcept;

  /**************************************************************
  ** Take everything from std::variant.
  ***************************************************************/
  using Base::as_std;
  using Base::Base;
  using Base::operator=;
  // using Base::emplace;
  // using Base::get;
  // using Base::get_if;
  // using Base::holds;
  // using Base::index;
  // using Base::swap;
  // using Base::to_enum;
  // using Base::valueless_by_exception;
};

void to_str( table const& o, std::string& out );
void to_str( lstring const& o, std::string& out );
void to_str( lfunction const& o, std::string& out );
void to_str( userdata const& o, std::string& out );
void to_str( lthread const& o, std::string& out );
void to_str( lightuserdata const& o, std::string& out );

} // namespace luapp

TOSTR_TO_FMT( luapp::table );
TOSTR_TO_FMT( luapp::lstring );
TOSTR_TO_FMT( luapp::lfunction );
TOSTR_TO_FMT( luapp::userdata );
TOSTR_TO_FMT( luapp::lthread );
TOSTR_TO_FMT( luapp::lightuserdata );

DEFINE_FORMAT( luapp::thing, "{}", o.as_std() );
