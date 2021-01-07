/****************************************************************
**no-serial.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-10.
*
* Description: Wrapper to avoid serializing/formatting a type.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "cc-specific.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"

namespace fb {
struct no_serial;
}

namespace rn {

// Wrap a type in this object if the object needs to be used in a
// context in which the compiler requires serialization and/or
// formatting specialization but where it is not necessary and/or
// desirable to provide them. There are two subcases:
//
//   1. The object will never be serialized in practice, in which
//      case the bFailOnSerialize should be true.
//   2. The object will be serialized in practice, but it is ok
//      to serialize/deserialize a default instance of the ob-
//      ject.
//
template<typename T, bool bFailOnSerialize = true>
struct no_serial {
  template<typename... Args>
  no_serial( Args&&... args )
    : o( std::forward<Args>( args )... ) {}

  // Implicit conversion.
  operator T const &() const& { return o; }
  operator T&() & { return o; }
  operator T&&() && { return std::move( o ); }

  T const* operator->() const& { return &o; }
  T*       operator->() & { return &o; }

  bool operator==(
      no_serial<T, bFailOnSerialize> const& rhs ) const {
    return o == rhs.o;
  }

  bool operator!=(
      no_serial<T, bFailOnSerialize> const& rhs ) const {
    return !( *this == rhs );
  }

  T o;
};

namespace serial {

template<typename Hint, typename T, bool bFailOnSerialize>
auto serialize( FBBuilder&,
                ::rn::no_serial<T, bFailOnSerialize> const&,
                serial::ADL ) {
  if constexpr( bFailOnSerialize ) {
    FATAL(
        "Should not serialize a `no_serial` object (contains "
        "type "
        "{}).",
        demangled_typename<T>() );
  }
  return ReturnValue{ FBOffset<fb::no_serial>{} };
}

template<typename SrcT, typename T, bool bFailOnSerialize>
valid_deserial_t deserialize(
    SrcT const*, ::rn::no_serial<T, bFailOnSerialize>*,
    serial::ADL ) {
  if constexpr( bFailOnSerialize ) {
    FATAL(
        "Should not deserialize a `no_serial` object (contains "
        "type {}).",
        demangled_typename<T>() );
  }
  return valid;
}

} // namespace serial

} // namespace rn

namespace fmt {

template<typename T, bool bFailOnSerialize>
struct formatter<::rn::no_serial<T, bFailOnSerialize>>
  : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::no_serial<T, bFailOnSerialize> const& o,
               FormatContext& ctx ) {
    if constexpr( rn::has_fmt<T> ) {
      return formatter_base::format( fmt::format( "{}", o.o ),
                                     ctx );
    } else {
      return formatter_base::format(
          fmt::format( "no_serial<{}>",
                       ::rn::demangled_typename<T>() ),
          ctx );
    }
  }
};

} // namespace fmt
