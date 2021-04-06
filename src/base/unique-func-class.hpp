/****************************************************************
**unique-func-class.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-22.
*
* Description: A type-erased function object for move-only types.
*
*****************************************************************/
/* no #pragma once; must be included twice per TU. */

// C++ standard library
#include <memory>
#include <type_traits>

namespace base {

// This is so that this file can be compiled standalone so that
// syntax checkers will work in the editor.
#ifndef UNIQUE_FUNC_CONST
template<typename T>
class unique_func;
#  define UNIQUE_FUNC_CONST
#endif

// FIXME:
//   * Add small-buffer optimization.
//   * Doesn't work with callables that take rvalue refs.

template<typename R, typename... Args>
class unique_func<R( Args... ) UNIQUE_FUNC_CONST> {
  struct func_base {
    virtual ~func_base()                              = default;
    virtual R operator()( Args... ) UNIQUE_FUNC_CONST = 0;
  };
  std::unique_ptr<func_base> func_;

public:
  // Can never be empty.
  unique_func() = delete;

  template<typename Func>
  unique_func( Func&& func ) {
    static_assert( std::is_rvalue_reference_v<
                   decltype( std::forward<Func>( func ) )> );
    struct child : func_base {
      Func func;
      child( Func&& f ) noexcept : func( std::move( f ) ) {}
      R operator()( Args... args ) UNIQUE_FUNC_CONST
          noexcept( noexcept( func( args... ) ) ) override {
        return func( args... );
      }
    };
    func_.reset( new child( std::move( func ) ) );
  }

  unique_func( unique_func const& ) = delete;
  unique_func& operator=( unique_func const& ) = delete;

  unique_func( unique_func&& ) = default;
  unique_func& operator=( unique_func&& ) = default;

  template<typename... RealArgs>
  R operator()( RealArgs&&... args ) UNIQUE_FUNC_CONST
      noexcept( noexcept( func_->operator()(
          std::forward<RealArgs>( args )... ) ) ) {
    return func_->operator()(
        std::forward<RealArgs>( args )... );
  }

  func_base*       get() noexcept { return func_.get(); }
  func_base const* get() const noexcept { return func_.get(); }
};

} // namespace base