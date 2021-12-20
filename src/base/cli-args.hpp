/****************************************************************
**cli-args.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: Command-line argument parsing.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "expect.hpp"

// C++ standard library
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace base {

struct ProgramArguments {
  explicit operator bool() const {
    return !key_val_args.empty() || !flag_args.empty() ||
           !positional_args.empty();
  }

  std::unordered_map<std::string, std::string> key_val_args;
  std::unordered_set<std::string>              flag_args;
  std::vector<std::string>                     positional_args;

  bool operator==( ProgramArguments const& ) const = default;

  friend void to_str( ProgramArguments const& pa,
                      std::string&            out, base::ADL_t );
};

expect<ProgramArguments> parse_args(
    std::span<std::string const> args );

ProgramArguments parse_args_or_die_with_usage(
    std::span<std::string const> args );

} // namespace base
