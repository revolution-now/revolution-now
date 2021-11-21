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

// Rcl
#include "rcl/ext.hpp"

// Rds
#include "rds/helper/enum.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/fmt.hpp"

// C++ standard library
#include <array>
#include <memory>

namespace rn {

template<ReflectedEnum Enum, typename ValT>
struct ExhaustiveEnumMap {
  static constexpr int kSize = enum_traits<Enum>::count;

 private:
  using ArrayT = std::array<std::pair<Enum, ValT>, kSize>;

 public:
  ExhaustiveEnumMap() : data_( std::make_unique<ArrayT>() ) {
    for( int i = 0; i < kSize; ++i )
      ( *data_ )[i] = { from_idx( i ), {} };
  }

  ExhaustiveEnumMap(
      std::initializer_list<std::pair<Enum, ValT>> il )
    : ExhaustiveEnumMap() {
    for( auto& [e, v] : il ) at( e ) = std::move( v );
  }

  // This map always has a fixed size, so you should not need to
  // query its size. If you need to know its size, use kSize.
  constexpr int size() const = delete;

  bool operator==( ExhaustiveEnumMap const& rhs ) const {
    return *data_ == *rhs.data_;
  }

  ExhaustiveEnumMap& operator=( ExhaustiveEnumMap const& rhs ) {
    *data_ = *rhs.data_;
    return *this;
  }

  ExhaustiveEnumMap( ExhaustiveEnumMap&& ) = default;
  ExhaustiveEnumMap& operator=( ExhaustiveEnumMap&& ) = default;

  ValT const& operator[]( Enum i ) const { return at( i ); }

  ValT& operator[]( Enum i ) { return at( i ); }

  ValT const& at( Enum i ) const {
    return ( *data_ )[to_idx( i )].second;
  }

  ValT& at( Enum i ) { return ( *data_ )[to_idx( i )].second; }

  auto find( Enum what ) const {
    int idx = to_idx( what );
    return begin() + idx;
  }

  // This is for deserializing from Rcl config files.
  friend rcl::convert_err<ExhaustiveEnumMap> convert_to(
      rcl::value const& v, rcl::tag<ExhaustiveEnumMap> ) {
    static std::string kTypeName =
        fmt::format( "{}<{}, {}>", "ExhaustiveEnumMap",
                     base::demangled_typename<Enum>(),
                     base::demangled_typename<ValT>() );
    constexpr int kNumFieldsExpected = kSize;
    base::maybe<std::unique_ptr<rcl::table> const&> mtbl =
        v.get_if<std::unique_ptr<rcl::table>>();
    if( !mtbl )
      return rcl::error( fmt::format(
          "cannot produce a {} from type {}.", kTypeName,
          rcl::name_of( rcl::type_of( v ) ) ) );
    DCHECK( *mtbl != nullptr );
    rcl::table const& tbl = **mtbl;
    if( tbl.size() != kNumFieldsExpected )
      return rcl::error( fmt::format(
          "table must have precisely {} field(s) for "
          "conversion to {}.",
          kNumFieldsExpected, kTypeName ) );
    ExhaustiveEnumMap res;
    for( auto& [key, val] : tbl ) {
      UNWRAP_RETURN( key_conv, rcl::convert_to<Enum>( key ) );
      UNWRAP_RETURN( val_conv, rcl::convert_to<ValT>( val ) );
      // We don't need to check for duplicates here because that
      // will already have been done by the rcl parsing.
      res[key_conv] = std::move( val_conv );
    }
    return res;
  }

  auto begin() { return data_->begin(); }
  auto begin() const { return data_->begin(); }
  auto end() { return data_->end(); }
  auto end() const { return data_->end(); }

  std::string to_str() const {
    std::string res = "[";
    for( auto const& [k, v] : *this )
      res += fmt::format( "{}={},", k, v );
    if( res.size() > 1 )
      res.resize( res.size() - 1 ); // remove trailing comma.
    res += ']';
    return res;
  }

 private:
  static int to_idx( Enum i ) {
    int idx = static_cast<int>( i );
    DCHECK( idx < kSize );
    DCHECK( idx >= 0 );
    return idx;
  }

  static Enum from_idx( int i ) {
    UNWRAP_CHECK( e, enum_traits<Enum>::from_integral( i ) );
    return e;
  }

  std::unique_ptr<ArrayT> data_;
};

} // namespace rn

EVAL( DEFINE_FORMAT_T( ( K, V ), (::rn::ExhaustiveEnumMap<K, V>),
                       "{}", o.to_str() ) );
