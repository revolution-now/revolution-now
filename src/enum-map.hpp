/****************************************************************
**enum-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-10.
*
* Description: Map data structure for exhaustive enum keys.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Cdr
#include "cdr/converter.hpp"
#include "cdr/ext-std.hpp"
#include "cdr/ext.hpp"

// Rcl
#include "rcl/ext.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/ext.hpp"
#include "refl/query-enum.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/cc-specific.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>

namespace rn {

// This is a map whose keys are always reflected enums and which
// is always guaranteed to have a value for every possible value
// of the enum, though some of those values might just be default
// constructed if they are not provided. That way, the map always
// has the same size and you can use operator[] on a const map.
//
// Note: type trait specializations of this (e.g. to_str) should
// just defer to to the base class implementation, which will
// usually always be provided because it is a standard container.
template<refl::ReflectedEnum Enum, typename ValT>
struct ExhaustiveEnumMap
  : public std::unordered_map<Enum, ValT> {
  using base = std::unordered_map<Enum, ValT>;

  static constexpr int kSize = refl::enum_count<Enum>;

  base&       as_base() { return *this; }
  base const& as_base() const { return *this; }

 public:
  // All other constructors should ultimately call this one,
  // since this is the one that ensures that all keys have a
  // value.
  ExhaustiveEnumMap( base&& b ) : base( std::move( b ) ) {
    for( Enum e : refl::enum_values<Enum> )
      if( this->find( e ) == this->end() ) //
        this->emplace( e, ValT{} );
  }

  ExhaustiveEnumMap( base const& b )
    : ExhaustiveEnumMap( base{ b } ) {}

  ExhaustiveEnumMap() : ExhaustiveEnumMap( base{} ) {}

  ExhaustiveEnumMap(
      std::initializer_list<std::pair<Enum const, ValT>> il )
    : ExhaustiveEnumMap( base( il.begin(), il.end() ) ) {}

  consteval size_t size() const { return kSize; }
  consteval int    ssize() const { return kSize; }

  bool operator==( ExhaustiveEnumMap const& ) const = default;

  ValT const& operator[]( Enum i ) const { return at( i ); }

  ValT& operator[]( Enum i ) { return at( i ); }

  ValT const& at( Enum i ) const {
    DCHECK( this->find( i ) != this->end() );
    return this->find( i )->second;
  }

  ValT& at( Enum i ) {
    DCHECK( this->find( i ) != this->end() );
    return this->find( i )->second;
  }

  friend cdr::value to_canonical(
      cdr::converter& conv, ExhaustiveEnumMap const& o,
      cdr::tag_t<ExhaustiveEnumMap> ) {
    return conv.to( o.as_base() );
  }

  friend cdr::result<ExhaustiveEnumMap> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<ExhaustiveEnumMap> ) {
    UNWRAP_RETURN( base_res,
                   conv.from<ExhaustiveEnumMap::base>( v ) );
    return ExhaustiveEnumMap( std::move( base_res ) );
  }

  // This is for deserializing from Rcl config files.
  friend rcl::convert_err<ExhaustiveEnumMap> convert_to(
      rcl::value const& v, rcl::tag<ExhaustiveEnumMap> ) {
    // TODO(migration): remove
    return rcl::via_cdr<ExhaustiveEnumMap>( v );
  }
};

} // namespace rn
