/****************************************************************
**seed.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-01.
*
* Description: Representation for random seeds.
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

struct seed {
  // e is for entropy.
  uint32_t e1 = {};
  uint32_t e2 = {};
  uint32_t e3 = {};
  uint32_t e4 = {};

  void mix();

  auto operator<=>( seed const& ) const = default;

  static base::maybe<seed> from_string( std::string_view sv );

  friend void to_str( seed const& o, std::string& out,
                      base::tag<seed> );

  friend cdr::value to_canonical( cdr::converter& conv,
                                  seed const& o,
                                  cdr::tag_t<seed> );

  friend cdr::result<seed> from_canonical( cdr::converter& conv,
                                           cdr::value const& v,
                                           cdr::tag_t<seed> );
};

} // namespace rng
