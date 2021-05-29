/****************************************************************
**fmt-to-str.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-01.
*
* Description: Utility for calling {fmt} without including its
*              headers.  This is done by pre-transforming args to
*              strings using the to_str framework.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <span>
#include <string>
#include <string_view>

namespace base {

namespace detail {

inline constexpr size_t kMaxFormatArgs = 10;

constexpr size_t estimate_str_res_size( std::string_view fmt_str,
                                        size_t arg_count ) {
  return fmt_str.size() + 5 * arg_count;
}

template<typename T>
void format_arg( size_t& idx,
                 int ( &arr )[detail::kMaxFormatArgs],
                 std::string& args_flat, T const& arg ) {
  arr[idx++] = args_flat.size();
  to_str( arg, args_flat );
}

} // namespace detail

std::string format_impl( std::string_view fmt_str,
                         std::span<int>   indexes,
                         std::string_view args_flat );

template<typename... Args>
std::string format( std::string_view fmt_str, Args&&... args ) {
  std::string args_flat;
  args_flat.reserve( detail::estimate_str_res_size(
      fmt_str, sizeof...( Args ) ) );
  static_assert( sizeof...( Args ) <= detail::kMaxFormatArgs,
                 "You have exceeded the maximum allowed number "
                 "of arguments to format." );
  int    sv_args[detail::kMaxFormatArgs];
  size_t i = 0;
  ( detail::format_arg( i, sv_args, args_flat, args ), ... );
  return format_impl( fmt_str, std::span<int>( &sv_args[0], i ),
                      args_flat );
}

} // namespace base
