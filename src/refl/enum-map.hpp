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

// refl
#include "cdr.hpp"
#include "ext.hpp"
#include "query-enum.hpp"

// Cdr
#include "cdr/converter.hpp"
#include "cdr/ext.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/cc-specific.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace refl {

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
struct enum_map : public std::unordered_map<Enum, ValT> {
  using base = std::unordered_map<Enum, ValT>;

  static constexpr int kSize = refl::enum_count<Enum>;

  base&       as_base() { return *this; }
  base const& as_base() const { return *this; }

 public:
  using typename base::key_type;
  using typename base::value_type;

  // All other constructors should ultimately call this one,
  // since this is the one that ensures that all keys have a
  // value.
  enum_map( base&& b ) : base( std::move( b ) ) {
    for( Enum e : refl::enum_values<Enum> )
      if( this->find( e ) == this->end() ) //
        this->emplace( e, ValT{} );
  }

  enum_map( base const& b ) : enum_map( base{ b } ) {}

  enum_map() : enum_map( base{} ) {}

  enum_map(
      std::initializer_list<std::pair<Enum const, ValT>> il )
    : enum_map( base( il.begin(), il.end() ) ) {}

  consteval size_t size() const { return kSize; }
  consteval int    ssize() const { return kSize; }

  bool operator==( enum_map const& ) const = default;

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

  // Make sure that the base class `contains` and `erase` methods
  // are not callable since calling them is not correct for this
  // class; enum_maps always contain every enum key, and so if
  // someone is trying to call them it is certainly a bug.
  void contains( Enum ) = delete;
  void erase( Enum )    = delete;

  friend cdr::value to_canonical( cdr::converter& conv,
                                  enum_map const& o,
                                  cdr::tag_t<enum_map> ) {
    cdr::table tbl;
    // Here we can use to_field to allow the converter to control
    // default field value behavior because, for this data struc-
    // ture, we know the complete set of possible keys and all of
    // their names, similar to a struct.
    for( Enum e : refl::enum_values<Enum> )
      conv.to_field(
          tbl, std::string( refl::enum_value_name( e ) ), o[e] );
    return tbl;
  }

  friend cdr::result<enum_map> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<enum_map> ) {
    UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
    std::unordered_set<std::string> used_keys;
    enum_map                        res;
    // Here we can use from_field to allow the converter to con-
    // trol default field value behavior because, for this data
    // structure, we know the complete set of possible keys and
    // all of their names, similar to a struct.
    for( Enum e : refl::enum_values<Enum> ) {
      UNWRAP_RETURN(
          val,
          conv.from_field<ValT>(
              tbl, std::string( refl::enum_value_name( e ) ),
              used_keys ) );
      res[e] = std::move( val );
    }
    HAS_VALUE_OR_RET(
        conv.end_field_tracking( tbl, used_keys ) );
    return res;
  }
};

} // namespace refl
