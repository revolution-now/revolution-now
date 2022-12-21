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
enum class UnitId : int {};

// to_str
void to_str( UnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, UnitId o,
                         cdr::tag_t<UnitId> );

cdr::result<UnitId> from_canonical( cdr::converter&   conv,
                                    cdr::value const& v,
                                    cdr::tag_t<UnitId> );

// lua
void lua_push( lua::cthread L, UnitId o );

base::maybe<UnitId> lua_get( lua::cthread L, int idx,
                             lua::tag<UnitId> );

/****************************************************************
** NativeUnitId
*****************************************************************/
// Native units.
enum class NativeUnitId : int {};

// to_str
void to_str( NativeUnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, NativeUnitId o,
                         cdr::tag_t<NativeUnitId> );

cdr::result<NativeUnitId> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<NativeUnitId> );

// lua
void lua_push( lua::cthread L, NativeUnitId o );

base::maybe<NativeUnitId> lua_get( lua::cthread L, int idx,
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

  // FIXME: these should be ideally replaced with a defaulted
  // spaceship operator, but that emits a warning due to a bug in
  // clang, see here:
  //
  //   https://github.com/llvm/llvm-project/issues/55919
  //
  // when that gets fixed, replace the below with:
  //
  //   auto operator<=>( GenericUnitId const& ) const = default;
  //
  bool operator==( GenericUnitId const& ) const = default;
  bool operator<( GenericUnitId const& rhs ) const noexcept {
    return id < rhs.id;
  }
  bool operator>( GenericUnitId const& rhs ) const noexcept {
    return id > rhs.id;
  }
  bool operator<=( GenericUnitId const& rhs ) const noexcept {
    return id <= rhs.id;
  }
  bool operator>=( GenericUnitId const& rhs ) const noexcept {
    return id >= rhs.id;
  }

  int id = 0;
};

// to_str
void to_str( GenericUnitId o, std::string& out, base::ADL_t );

// cdr
cdr::value to_canonical( cdr::converter& conv, GenericUnitId o,
                         cdr::tag_t<GenericUnitId> );

cdr::result<GenericUnitId> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<GenericUnitId> );

// lua
void lua_push( lua::cthread L, GenericUnitId o );

base::maybe<GenericUnitId> lua_get( lua::cthread L, int idx,
                                    lua::tag<GenericUnitId> );

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
