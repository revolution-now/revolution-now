/****************************************************************
**coro-compat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-29.
*
* Description: Some fixes and compatibility stuff that will make
*              coroutines work with both clang and gcc and with
*              both libc++ and libstdc++, including the
*              clang+libstdc++ combo.
*
*****************************************************************/
#pragma once

// Need to include at least one cpp header to ensure that we get
// the _LIBCPP_VERSION macro.
#include <cctype>

/****************************************************************
** Fix CLI flag inconsistency.
*****************************************************************/
// libstdc++ throws an error if this is not defined, but it is
// only defined when -fcoroutines is specified, which clang does
// not support (it uses -fcoroutines-ts), which makes the
// clang+libstdc++ combo impossible with coroutines. This works
// around that.
#if !defined( __cpp_impl_coroutine )
#  define __cpp_impl_coroutine 1
#endif

/****************************************************************
** Fix header inconsistency.
*****************************************************************/
// FIXME: remove this when libc++ moves the coroutine library out
// of experimental.
#if defined( _LIBCPP_VERSION )
#  include <experimental/coroutine> // libc++
#else
#  include <coroutine> // libstdc++
#endif

/****************************************************************
** Fix namespace inconsistency.
*****************************************************************/
// FIXME: remove this when libc++ moves the coroutine library out
// of experimental.
#if defined( _LIBCPP_VERSION )
#  define CORO_NS std::experimental // libc++
#else
#  define CORO_NS std // libstdc++
#endif

namespace coro = ::CORO_NS;

/****************************************************************
** Fix coroutine_handle::from_address not being noexcept.
*****************************************************************/
namespace base {
template<typename T = void>
class coroutine_handle : public coro::coroutine_handle<T> {};
} // namespace base

/****************************************************************
** Fix that gcc's suspend_never has non-noexcept methods.
*****************************************************************/
namespace base {
// We need to include this because the one that gcc 10.2 comes
// shiped with does not have its methods declared with noexcept,
// which scares clang and prevents us from using coroutines with
// the clang+libstdc++ combo. FIXME: remove this when gcc 10.2
// fixes its version.
struct suspend_never {
  bool await_ready() noexcept { return true; }
  void await_suspend( coro::coroutine_handle<> ) noexcept {}
  void await_resume() noexcept {}
};
struct suspend_always {
  bool await_ready() noexcept { return false; }
  void await_suspend( coro::coroutine_handle<> ) noexcept {}
  void await_resume() noexcept {}
};
} // namespace base
