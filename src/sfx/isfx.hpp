/****************************************************************
**isfx.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Interface for sound effects.
*
*****************************************************************/
#pragma once

// rds
#include "isfx.rds.hpp"

// config
#include "config/sfx-enum.rds.hpp"

// base
#include "base/expect.hpp"

// C++ standard library
#include <source_location>

namespace sfx {

struct error {
  std::source_location loc = std::source_location::current();
  std::string msg;

  friend void to_str( error const& o, std::string& out,
                      base::tag<error> );
};

template<typename T>
using or_err = base::expect<T, error>;

/****************************************************************
** ISfx
*****************************************************************/
struct ISfx {
  virtual ~ISfx() = default;

  virtual void play_sound_effect( rn::e_sfx sound ) const = 0;
};

} // namespace sfx
