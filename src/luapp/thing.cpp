/****************************************************************
**thing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: C++ containers for Lua values/objects.
*
*****************************************************************/
#include "thing.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"

using namespace std;

namespace luapp {

/****************************************************************
** reference
*****************************************************************/
reference::reference( lua_State* st, int ref )
  : L( st ), ref_( ref ) {}

reference::~reference() noexcept { release(); }

void reference::release() noexcept {
  if( ref_ != LUA_NOREF ) {
    luaL_unref( L, LUA_REGISTRYINDEX, ref_ );
    ref_ = LUA_NOREF;
  }
}

reference::reference( reference&& rhs ) noexcept
  : L( rhs.L ), ref_( std::exchange( rhs.ref_, LUA_NOREF ) ) {}

reference& reference::operator=( reference&& rhs ) noexcept {
  if( this == &rhs ) return *this;
  if( ref_ != LUA_NOREF ) {
    // We have a reference already.
    if( L == rhs.L ) {
      // If we referring to the same Lua state then we should
      // never be holding the same reference as the rhs (unless
      // it's LUA_NOREF which we've already checked).
      CHECK( ref_ != rhs.ref_ );
    }
    release();
  }
  L    = rhs.L;
  ref_ = rhs.ref_;
  rhs.release();
  return *this;
}

reference::operator bool() const noexcept {
  return ref_ != LUA_NOREF;
}

int reference::noref() noexcept { return c_api::noref(); }

void reference::push() const noexcept {
  c_api C( L, /*own=*/false );
  C.registry_get( ref_ );
}

/****************************************************************
** thing
*****************************************************************/
e_lua_type thing::type() const noexcept {
  switch( index() ) {
    case 0: return e_lua_type::nil;
    case 1: return e_lua_type::boolean;
    case 2: return e_lua_type::lightuserdata;
    case 3: return e_lua_type::number;
    case 4: return e_lua_type::number;
    case 5: return e_lua_type::string;
    case 6: return e_lua_type::table;
    case 7: return e_lua_type::function;
    case 8: return e_lua_type::userdata;
    case 9: return e_lua_type::thread;
  }
  SHOULD_NOT_BE_HERE;
}

// Follows Lua's rules, where every value is true except for
// boolean:false and nil.
thing::operator bool() const noexcept {
  switch( index() ) {
    case 0:
      // nil
      return false;
    case 1:
      // bool
      return this->get<boolean>().get();
    default: //
      return true;
  }
}


bool thing::operator==( thing const& rhs ) const noexcept {
  return std::visit(
      []( auto const& l, auto const& r ) {
        using left_t  = std::remove_cvref_t<decltype( l )>;
        using right_t = std::remove_cvref_t<decltype( r )>;
        if constexpr( std::is_same_v<left_t, right_t> )
          return ( l == r );
        else if constexpr( std::is_convertible_v<
                               left_t const&, right_t const&> )
          return ( static_cast<right_t const&>( l ) == r );
        else if constexpr( std::is_convertible_v<right_t const&,
                                                 left_t const&> )
          return ( l == static_cast<left_t const&>( r ) );
        else
          return false;
      },
      this->as_std(), rhs.as_std() );
}

/****************************************************************
** to_str
*****************************************************************/
void to_str( table const& o, std::string& out ) {
  (void)o;
  out += "<table>";
}

void to_str( lstring const& o, std::string& out ) {
  (void)o;
  out += "<string>";
}

void to_str( lfunction const& o, std::string& out ) {
  (void)o;
  out += "<function>";
}

void to_str( userdata const& o, std::string& out ) {
  (void)o;
  out += "<userdata>";
}

void to_str( lthread const& o, std::string& out ) {
  (void)o;
  out += "<thread>";
}

void to_str( lightuserdata const& o, std::string& out ) {
  out += fmt::format( "<lightuserdata:{}>", o.get() );
}

} // namespace luapp
