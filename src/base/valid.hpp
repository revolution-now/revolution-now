/****************************************************************
**valid.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-08.
*
* Description: Return type for functions that do validation.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "expect.hpp"

// {fmt}
#include "fmt/format.h"

namespace base {

// This type is used as the return type of functions that perform
// validation, i.e., if validation passes then nothing needs to
// be returned, but if it fails, then some error type needs to be
// returned.
struct valid_t {
  bool operator==( valid_t const& ) const = default;
};

/****************************************************************
** Forward Declaration.
*****************************************************************/
template<typename E>
class valid_or;

/****************************************************************
** Basic metaprogramming helpers.
*****************************************************************/
template<typename E>
constexpr bool is_valid_or_v = false;
template<typename E>
constexpr bool is_valid_or_v<valid_or<E>> = true;

/****************************************************************
** valid_or
*****************************************************************/
template<typename E>
class [[nodiscard]] valid_or : expect<valid_t, E> {
  using Base = expect<valid_t, E>;
  Base const&  as_expect() const& { return *this; }
  Base&        as_expect() & { return *this; }
  Base const&& as_expect() const&& { return *this; }
  Base&&       as_expect() && { return *this; }

public:
  using typename Base::error_type;

  // Take only the things from the base class that are relevant
  // given that we're not holding values, only errors.
  using Base::Base;
  using Base::operator bool;
  // using Base::operator=;
  using Base::error;

  bool valid() const { return !!( *this ); }

  friend constexpr bool operator==( valid_or<E> const& lhs,
                                    valid_t const& ) noexcept {
    return lhs.has_value();
  }

  friend constexpr bool operator==(
      valid_or<E> const& lhs,
      valid_or<E> const&
          rhs ) noexcept( noexcept( lhs.as_expect() ==
                                    rhs.as_expect() ) ) {
    return lhs.as_expect() == rhs.as_expect();
  }

  friend constexpr bool operator!=(
      valid_or<E> const& lhs,
      valid_or<E> const&
          rhs ) noexcept( noexcept( lhs.as_expect() !=
                                    rhs.as_expect() ) ) {
    return lhs.as_expect() != rhs.as_expect();
  }

  friend constexpr bool operator==(
      valid_or<E> const& lhs,
      E const& rhs ) noexcept( noexcept( lhs.as_expect() ==
                                         rhs ) ) {
    return lhs.as_expect() == rhs;
  }
};

/****************************************************************
** valid & invalid
*****************************************************************/
inline constexpr valid_t valid{};

// Constructs a valid_or in-place in an error state.
template<typename E>
inline constexpr valid_or<E> invalid( E&& e ) {
  return valid_or<E>( std::forward<E>( e ) );
}

/****************************************************************
** to_str
*****************************************************************/
template<typename E>
void to_str( valid_or<E> const& vo, std::string& out ) {
  if( vo.valid() )
    out += "valid";
  else {
    out += "invalid{";
    to_str( vo.error(), out );
    out += "}";
  }
}

inline void to_str( valid_t const&, std::string& out ) {
  out += "valid";
}

} // namespace base

/****************************************************************
** {fmt}
*****************************************************************/
// {fmt} formatter for formatting valid_or's whose contained
// types are formattable.
template<typename E>
struct fmt::formatter<base::valid_or<E>>
  : fmt::formatter<std::string> {
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( base::valid_or<E> const& e, FormatContext& ctx ) {
    if( e )
      return formatter_base::format( "valid", ctx );
    else
      return formatter_base::format(
          fmt::format( "invalid{{{}}}", e.error() ), ctx );
  }
};

// {fmt} formatter for formatting valid_t.
template<>
struct fmt::formatter<base::valid_t>
  : fmt::formatter<std::string> {
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( base::valid_t const&, FormatContext& ctx ) {
    static const std::string valid_str( "valid" );
    return formatter_base::format( valid_str, ctx );
  }
};
