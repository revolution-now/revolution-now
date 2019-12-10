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
#include "aliases.hpp"
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
// desirable to provide them (e.g., the type will never be seri-
// alized in practice).
template<typename T>
struct no_serial {
  template<typename... Args>
  no_serial( Args&&... args )
    : o( std::forward<Args>( args )... ) {}

  // Implicit conversion.
  operator T const&() const& { return o; }
  operator T&() & { return o; }
  operator T &&() && { return std::move( o ); }

  T const* operator->() const& { return &o; }
  T*       operator->() & { return &o; }

  bool operator==( no_serial<T> const& rhs ) const {
    return o == rhs.o;
  }

  bool operator!=( no_serial<T> const& rhs ) const {
    return !( *this == rhs );
  }

  T o;
};

namespace serial {

template<typename Hint, typename T>
auto serialize( FBBuilder&, ::rn::no_serial<T> const&,
                serial::ADL ) {
  FATAL(
      "Should not serialize a `no_serial` object (contains type "
      "{}).",
      demangled_typename<T>() );
  return ReturnValue{ FBOffset<fb::no_serial>{} };
}

template<typename SrcT, typename T>
expect<> deserialize( SrcT const*, ::rn::no_serial<T>*,
                      serial::ADL ) {
  FATAL(
      "Should not deserialize a `no_serial` object (contains "
      "type {}).",
      demangled_typename<T>() );
  return ::rn::xp_success_t{};
}

} // namespace serial

} // namespace rn

namespace fmt {

template<typename T>
struct formatter<::rn::no_serial<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::no_serial<T> const& o,
               FormatContext&            ctx ) {
    if constexpr( rn::has_fmt<T> ) {
      return formatter_base::format( fmt::format( "{}", o.o ),
                                     ctx );
    } else {
      return formatter_base::format(
          fmt::format( "no_serial<{}>",
                       demangled_typename<T>() ),
          ctx );
    }
  }
};

} // namespace fmt
