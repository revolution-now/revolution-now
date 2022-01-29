/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-23.
*
* Description: Concepts relating to convertibility to/from the
*              CDR (Canonical Data Representation).
*
*****************************************************************/
#pragma once

// CDR
#include "repr.hpp"

// base
#include "base/expect.hpp"
#include "base/fmt.hpp"

// C++ standard library
#include <concepts>

namespace cdr {

/****************************************************************
** ADL / Conversion helper tag
*****************************************************************/
// The fact that it is templated helps to prevent unwanted over-
// load ambiguities.
template<typename T>
struct tag_t {};

template<typename T>
inline constexpr tag_t<T> tag{};

/****************************************************************
** C++ ==> Canonical
*****************************************************************/
// For types that support converting themselves to the canonical
// data representation. If they support it, it is expected that
// such a conversion will always succeed.
template<typename T>
concept ToCanonical = requires( T const& o ) {
  // We need the `tag` here to get ADL to search the `cdr` name-
  // space; for most user-defined types this will not be neces-
  // sary since their to_canonical functions will be in their own
  // namespace (which will be searched), but it will be necessary
  // for builtin types or std types where we don't want to open
  // the std namespace.
  // clang-format off
  { to_canonical( o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<value>;
  // clang-format on
};

// This one should be used to convert a value (i.e., don't call
// the one with the tag explicitly).
template<ToCanonical T>
value to_canonical( T const& o ) {
  // The function called below should be found via ADL.
  return to_canonical( o, tag<std::remove_const_t<T>> );
}

/****************************************************************
** Error reporting
*****************************************************************/
// This is similar to UNWRAP_RETURN except that, upon failure, it
// will add in the current frame for better error tracking. There
// needs to be a string in the local scope called kFrameName.
#define CDR_UNWRAP_RETURN( var, ... )                      \
  auto&& STRING_JOIN( __x, __LINE__ ) = __VA_ARGS__;       \
  if( !STRING_JOIN( __x, __LINE__ ).has_value() ) {        \
    auto err =                                             \
        std::move( STRING_JOIN( __x, __LINE__ ) ).error(); \
    err.frames.push_back( ::cdr::error::Frame{             \
        .conversion_target_type_name = kFrameName,         \
        .location = base::SourceLoc::current() } );        \
    return std::move( err );                               \
  }                                                        \
  auto&& var = *STRING_JOIN( __x, __LINE__ );

struct error {
  struct builder;

  error( error const& ) = default;
  error( error&& )      = default;

  error& operator=( error const& ) = default;
  error& operator=( error&& ) = default;

  // This doesn't compare frames.
  bool operator==( error const& rhs ) const {
    return what == rhs.what;
  }

  friend void to_str( error const& o, std::string& out ) {
    out += o.what;
  }

  struct Frame {
    std::string     conversion_target_type_name;
    base::SourceLoc location;
  };

  std::string        what;
  std::vector<Frame> frames;

 private:
  template<typename... Args>
  explicit error( std::string frame_name, base::SourceLoc loc,
                  std::string_view fmt_str, Args&&... args )
    : what( fmt::format( fmt::runtime( fmt_str ),
                         std::forward<Args>( args )... ) ),
      frames{ Frame{
          .conversion_target_type_name = std::move( frame_name ),
          .location                    = loc } } {}
};

struct error::builder {
  builder( std::string     name,
           base::SourceLoc loc = base::SourceLoc::current() )
    : name_( std::move( name ) ), loc_( loc ) {}
  std::string     name_;
  base::SourceLoc loc_;

  template<typename... Args>
  error operator()( std::string_view fmt_str,
                    Args&&... args ) const&& {
    return error( std::move( name_ ), loc_, fmt_str,
                  std::forward<Args>( args )... );
  }
};

template<typename T>
using result = base::expect<T, error>;

/****************************************************************
** Canonical ==> C++
*****************************************************************/
// For types that support converting themselves from the canon-
// ical data representation. Unlike `to_canonical`, this one may
// fail.
template<typename T>
concept FromCanonical = requires( value const& o ) {
  // Note that this from_canonical function is expected to per-
  // form any validation that is needed after the conversion.
  // clang-format off
  { from_canonical( o, tag<std::remove_const_t<T>> ) }
      -> std::same_as<result<std::remove_const_t<T>>>;
  // clang-format on
};

// This one should be used to convert a value (i.e., don't call
// the one with the tag explicitly).
template<FromCanonical T>
result<std::remove_const_t<T>> from_canonical( value const& v ) {
  // The function called below should be found via ADL.
  return from_canonical( v, tag<std::remove_const_t<T>> );
}

/****************************************************************
** C++ <==> Canonical
*****************************************************************/
template<typename T>
concept Canonical = ToCanonical<T> && FromCanonical<T>;

} // namespace cdr
