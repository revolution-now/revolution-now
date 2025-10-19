/****************************************************************
**unit-id.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Strongly-typed IDs for units.
*
*****************************************************************/
#pragma once

// luapp
#include "luapp/ext.hpp"

// cdr
#include "cdr/ext.hpp"

// traverse
#include "traverse/ext.hpp"

// base
#include "base/to-str.hpp"

namespace rn {

// The types in this module are defined as empty enum classes
// with an underlying type of int. Even though the enums have no
// named values, it is guaranteed that they can assume any value
// that an int can assume (i.e. no undefined behavior).

// TODO: When C++23 is ready replace with std::to_underlying.
template<typename Enum>
inline constexpr auto to_underlying( Enum e ) noexcept {
  return static_cast<std::underlying_type_t<Enum>>( e );
}

/****************************************************************
** UnitId
*****************************************************************/
// European units.
enum class UnitId : int {
};

// to_str
void to_str( UnitId o, std::string& out, base::tag<UnitId> );

// cdr
cdr::value to_canonical( cdr::converter& conv, UnitId o,
                         cdr::tag_t<UnitId> );

cdr::result<UnitId> from_canonical( cdr::converter& conv,
                                    cdr::value const& v,
                                    cdr::tag_t<UnitId> );

// traverse: Handled by virtue of being a scalar.
static_assert( std::is_scalar_v<UnitId> );

// lua
void lua_push( lua::cthread L, UnitId o );

lua::lua_expect<UnitId> lua_get( lua::cthread L, int idx,
                                 lua::tag<UnitId> );

/****************************************************************
** NativeUnitId
*****************************************************************/
// Native units.
enum class NativeUnitId : int {
};

// to_str
void to_str( NativeUnitId o, std::string& out,
             base::tag<NativeUnitId> );

// cdr
cdr::value to_canonical( cdr::converter& conv, NativeUnitId o,
                         cdr::tag_t<NativeUnitId> );

cdr::result<NativeUnitId> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<NativeUnitId> );

// traverse: Handled by virtue of being a scalar.
static_assert( std::is_scalar_v<NativeUnitId> );

// lua
void lua_push( lua::cthread L, NativeUnitId o );

lua::lua_expect<NativeUnitId> lua_get( lua::cthread L, int idx,
                                       lua::tag<NativeUnitId> );

/****************************************************************
** GenericUnitId
*****************************************************************/
// Can represent either European units or native units. Define it
// as a struct so that we can add some implicit conversions from
// the more specific unit IDs, which are allowed in this case be-
// cause we are moving from specific to general (the other direc-
// tion is not allowed).
struct GenericUnitId {
  constexpr GenericUnitId() = default;

  // Must be explicit for safety.
  explicit constexpr GenericUnitId( int n ) noexcept : id( n ) {}

  // Implicit conversions.
  constexpr GenericUnitId( UnitId n ) noexcept
    : id( to_underlying( n ) ) {}
  constexpr GenericUnitId( NativeUnitId n ) noexcept
    : id( to_underlying( n ) ) {}

  auto operator<=>( GenericUnitId const& ) const = default;

  // to_str
  friend void to_str( GenericUnitId o, std::string& out,
                      base::tag<GenericUnitId> );

  // cdr
  friend cdr::value to_canonical( cdr::converter& conv,
                                  GenericUnitId o,
                                  cdr::tag_t<GenericUnitId> );

  friend cdr::result<GenericUnitId> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<GenericUnitId> );

  // traverse
  friend void traverse( GenericUnitId, auto&,
                        trv::tag_t<GenericUnitId> ) {}

  // lua
  friend void lua_push( lua::cthread L, GenericUnitId o );

  friend lua::lua_expect<GenericUnitId> lua_get(
      lua::cthread L, int idx, lua::tag<GenericUnitId> );

  int id = 0;
};

template<>
inline constexpr auto to_underlying<GenericUnitId>(
    GenericUnitId id ) noexcept {
  return id.id;
}

} // namespace rn

// std::hash
namespace std {
template<>
struct hash<::rn::GenericUnitId> {
  auto operator()( ::rn::GenericUnitId id ) const noexcept {
    return hash<int>{}( id.id );
  }
};

} // namespace std
