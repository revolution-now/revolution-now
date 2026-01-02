/****************************************************************
**entropy.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-01.
*
* Description: Representation for random entropy.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"
#include "base/to-str.hpp"

// cdr
#include "cdr/ext.hpp"

// C++ standard library
#include <cstdint>
#include <string_view>

namespace rng {

/****************************************************************
** entropy
*****************************************************************/
// This is a value that can be used to represent some true ran-
// domness (e.g. to seed a pseudo-random rng) or could just be
// used to generally hold a seed obtained deterministically.
struct entropy {
  // These these are interpreted together as a 128 number they
  // are taken as "least significant" first.
  uint32_t e1 = {};
  uint32_t e2 = {};
  uint32_t e3 = {};
  uint32_t e4 = {};

  auto operator<=>( entropy const& ) const = default;

  // ------------------------------------------------------------
  // Device/hardward source.
  // ------------------------------------------------------------
  static entropy from_random_device();

  // ------------------------------------------------------------
  // Mixing.
  // ------------------------------------------------------------
  void mix();

  entropy mixed() const;

  // ------------------------------------------------------------
  // Consuming.
  // ------------------------------------------------------------
  // Consume will extract enough bytes from the entropy in order
  // to populate the given type. But it will not destroy any en-
  // tropy in the members; it will just rotate the whole so that
  // subsequent calls will yield different results until such
  // type as all the bytes are consumed, then it will repeat.
  template<typename T>
  requires( std::is_integral_v<T> && std::is_unsigned_v<T> &&
            !std::is_same_v<T, bool> )
  [[nodiscard]] T consume() {
    T res = {};
    for( size_t i = 0; i < sizeof( T ); ++i ) {
      uint8_t const byte = e1 & 0xff;
      res += byte;
      res = std::rotr( res, 8 );
      rotate_right_n_bytes( 1 );
    }
    return res;
  }

  // ------------------------------------------------------------
  // Stringification.
  // ------------------------------------------------------------
  friend void to_str( entropy const& o, std::string& out,
                      base::tag<entropy> );

  static base::maybe<entropy> from_string( std::string_view sv );

  // ------------------------------------------------------------
  // CDR.
  // ------------------------------------------------------------
  friend cdr::value to_canonical( cdr::converter& conv,
                                  entropy const& o,
                                  cdr::tag_t<entropy> );

  friend cdr::result<entropy> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<entropy> );

  // ------------------------------------------------------------
  // Implementation.
  // ------------------------------------------------------------
  void rotate_right_n_bytes( uint8_t n_bytes );
};

/****************************************************************
** seed
*****************************************************************/
// For better expressing intended interpretation of an entropy
// value. The basic convention we use is that "entropy" is some-
// thing non-deterministic that comes from a random device and is
// used to seed a pseudo-random generator, while a "seed" could
// be generated deterministically by a pseudo-random generator
// and used to seed something higher level such as a map gener-
// ator where we might want to serialize and/or control or
// specify the seed for reproducibility.
using seed = entropy;

} // namespace rng
