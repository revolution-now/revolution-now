/****************************************************************
**ansi.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-21.
*
* Description: ANSI terminal code utilities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <string_view>

namespace rn {

namespace ansi {

/// Formatting codes
inline constexpr std::string_view reset      = "\033[m";
inline constexpr std::string_view bold       = "\033[1m";
inline constexpr std::string_view dark       = "\033[2m";
inline constexpr std::string_view underline  = "\033[4m";
inline constexpr std::string_view blink      = "\033[5m";
inline constexpr std::string_view reverse    = "\033[7m";
inline constexpr std::string_view concealed  = "\033[8m";
inline constexpr std::string_view clear_line = "\033[K";

// Foreground colors
inline constexpr std::string_view black   = "\033[30m";
inline constexpr std::string_view red     = "\033[31m";
inline constexpr std::string_view green   = "\033[32m";
inline constexpr std::string_view yellow  = "\033[33m";
inline constexpr std::string_view blue    = "\033[34m";
inline constexpr std::string_view magenta = "\033[35m";
inline constexpr std::string_view cyan    = "\033[36m";
inline constexpr std::string_view white   = "\033[37m";

/// Background colors
inline constexpr std::string_view on_black   = "\033[40m";
inline constexpr std::string_view on_red     = "\033[41m";
inline constexpr std::string_view on_green   = "\033[42m";
inline constexpr std::string_view on_yellow  = "\033[43m";
inline constexpr std::string_view on_blue    = "\033[44m";
inline constexpr std::string_view on_magenta = "\033[45m";
inline constexpr std::string_view on_cyan    = "\033[46m";
inline constexpr std::string_view on_white   = "\033[47m";

} // namespace ansi

} // namespace rn
