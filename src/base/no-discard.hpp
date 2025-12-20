/****************************************************************
**no-discard.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Wrapper type with the [[nodiscard]] attribute.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <utility>

namespace base {

// This is useful for wrapping the type inside of a wait to
// signal that it shouldn't be discarded. If we were to just put
// the [[nodiscard]] on the return type of the function returning
// the wait then it would only apply to the wait, not the result
// inside of it, so that co_await'ing on that wait would yield a
// result that could be discarded. Hence, we use this:
//
//   wait<base::NoDiscard<bool>> some_func( ... ) {
//     ...
//   }
//
// though NOTE that if you're going to use that specific one then
// use wait_bool.
//
template<typename T>
struct [[nodiscard]] NoDiscard {
  template<typename U>
  NoDiscard( U&& val_ ) : val( std::forward<U>( val_ ) ){};
  operator T&() { return val; }
  operator T const&() const { return val; }
  T const& get() const { return val; }
  T val;
};

} // namespace base
