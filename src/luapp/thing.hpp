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
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** value types
*****************************************************************/
bool operator==( nil_t const& l, boolean const& r );
bool operator==( nil_t const& l, lightuserdata const& r );
bool operator==( nil_t const& l, integer const& r );
bool operator==( nil_t const& l, floating const& r );
bool operator==( boolean const& l, lightuserdata const& r );
bool operator==( boolean const& l, integer const& r );
bool operator==( boolean const& l, floating const& r );
bool operator==( lightuserdata const& l, integer const& r );
bool operator==( lightuserdata const& l, floating const& r );
bool operator==( integer const& l, floating const& r );

/****************************************************************
** reference
*****************************************************************/
struct reference {
  reference() = delete;
  ~reference() noexcept;

  void release() noexcept;

  reference( reference const& ) noexcept;
  reference& operator=( reference const& ) noexcept;

  // Pushes nil if there is no reference. We take a Lua state
  // here instead of using the one we have in order to force all
  // callers to specify one, that way there are no bugs when
  // working with multiple Lua threads.
  e_lua_type push( lua_State* L ) const noexcept;

  static int noref() noexcept;

  lua_State* lua_state() const noexcept;

protected:
  reference( lua_State* st, int ref, e_lua_type type ) noexcept;

  lua_State* L; // not owned.

private:
  int ref_;
};

bool operator==( reference const& lhs, reference const& rhs );

bool operator==( reference const& r, nil_t );
bool operator==( reference const& r, boolean const& b );
bool operator==( reference const& r, lightuserdata const& lud );
bool operator==( reference const& r, integer const& i );
bool operator==( reference const& r, floating const& f );

void push( lua_State* L, reference const& r );

/****************************************************************
** table
*****************************************************************/
struct table : public reference {
  using Base = reference;

  table( lua_State* st, int ref ) noexcept;
};

/****************************************************************
** lstring
*****************************************************************/
struct lstring : public reference {
  using Base = reference;

  lstring( lua_State* st, int ref ) noexcept;

  std::string as_cpp() const;

  bool operator==( char const* s ) const;
  bool operator==( std::string_view s ) const;
  bool operator==( std::string const& s ) const;
};

/****************************************************************
** lfunction
*****************************************************************/
struct lfunction : public reference {
  using Base = reference;

  lfunction( lua_State* st, int ref ) noexcept;
};

/****************************************************************
** userdata
*****************************************************************/
struct userdata : public reference {
  using Base = reference;

  userdata( lua_State* st, int ref ) noexcept;
};

/****************************************************************
** lthread
*****************************************************************/
struct lthread : public reference {
  using Base = reference;

  lthread( lua_State* st, int ref ) noexcept;
};

/****************************************************************
** thing
*****************************************************************/
// nil_t should be first so that it is selected as the default.
using thing_base =
    base::variant<nil_t, boolean, lightuserdata, integer,
                  floating, lstring, table, lfunction, userdata,
                  lthread>;

struct thing : public thing_base {
  using Base = thing_base;

  using Base::Base;
  using Base::operator=;

  bool operator==( thing const& rhs ) const noexcept;

  bool operator==( std::string_view rhs ) const noexcept;

  template<typename T>
  // clang-format off
  requires( !std::is_same_v<thing, T> &&
             std::is_constructible_v<thing, T const&> )
  bool operator==( T const& rhs ) const noexcept {
    // clang-format on
    return ( *this ) == thing( rhs );
  }

  std::string tostring() const noexcept;

  // Follows Lua's rules, where every value is true except for
  // false and nil.
  operator bool() const noexcept;

  e_lua_type type() const noexcept;

  void         push( lua_State* L ) const noexcept;
  static thing pop( lua_State* L ) noexcept;
};

void push( lua_State* L, thing const& th );

/****************************************************************
** to_str
*****************************************************************/
void to_str( reference const& r, std::string& out );
void to_str( thing const& th, std::string& out );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::table );
TOSTR_TO_FMT( lua::lstring );
TOSTR_TO_FMT( lua::lfunction );
TOSTR_TO_FMT( lua::userdata );
TOSTR_TO_FMT( lua::lthread );

TOSTR_TO_FMT( lua::thing );
