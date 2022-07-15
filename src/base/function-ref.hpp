/****************************************************************
**function-ref.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-14.
*
* Description: function_ref implementation.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "attributes.hpp"
#include "error.hpp"

// C++ standard library
#include <utility>

namespace base {

// NOTE: there are multiple similar copies (specializations) of
// function_ref below, if you change one of them don't forget to
// change the others.

/****************************************************************
** function_ref
*****************************************************************/
// NOTE: It would seem that function_ref is on track for stan-
// dardization, and so when that happens this should be replaced
// by that. At the time of writing, the current version of the
// proposal is p0792r10. The below implementation seeks to be as
// compatible with that as possible.
template<typename... T>
struct function_ref;

// The p0792r10 proposal proposes to make one specialization for
// each combination of possible (const, noexcept) qualifications.
// For now we will just make the ones that we need.

// const, non-noexcept.
template<typename R, typename... Args>
struct function_ref<R( Args... ) const /*noexcept*/> {
 private:
  constexpr static bool kIsNoexcept = false;

  template<typename T>
  static constexpr bool is_callable_in_context =
      kIsNoexcept ? std::is_nothrow_invocable_r_v<R, T, Args...>
                  : std::is_invocable_r_v<R, T, Args...>;

  using Signature = R( Args... );
  using Thunk     = R( void const*, Args... );

  Thunk*      p_thunk_      = nullptr;
  void const* bound_entity_ = nullptr;

 public:
  function_ref() = delete;

  template<typename F>
  requires( std::is_function_v<F> && is_callable_in_context<F> )
  function_ref( F* f ATTR_LIFETIMEBOUND ) noexcept {
    p_thunk_ = +[]( void const* f, Args... args ) -> R {
      return reinterpret_cast<F*>( f )(
          std::forward<Args>( args )... );
    };
    // Note that we're not taking the address of f, we're just
    // converting it directly to a pointer. This prevents holding
    // a dangling point to a temporary pointer, which is easy to
    // cause at the call site.
    bound_entity_ = reinterpret_cast<void const*>( f );
  }

  template<typename F>
  requires(
      !std::is_same_v<std::remove_cvref_t<F>, function_ref> &&
      !std::is_member_pointer_v<std::remove_reference_t<F>> &&
      is_callable_in_context<std::remove_reference_t<F> const&> )
  constexpr function_ref( F&& f ATTR_LIFETIMEBOUND ) noexcept {
    p_thunk_ = +[]( void const* f, Args... args ) -> R {
      // The proposal mentions that it must be LValue-Callable...
      // I think that enables us to do the following, i.e., just
      // converting f to an R lvalue and calling it.
      return ( *static_cast<std::remove_cvref_t<F> const*>(
          f ) )( std::forward<Args>( args )... );
    };
    bound_entity_ = std::addressof( f );
  }

  constexpr function_ref( function_ref const& ) noexcept =
      default;
  constexpr function_ref& operator=(
      function_ref const& ) noexcept = default;

  template<typename T>
  requires( !std::is_same_v<T, function_ref> &&
            !std::is_pointer_v<T> )
  function_ref& operator=( T ) = delete;

  bool operator==( std::nullptr_t ) const = delete;

  R operator()( Args... args ) const noexcept( kIsNoexcept ) {
    DCHECK( p_thunk_ != nullptr );
    DCHECK( bound_entity_ != nullptr );
    return p_thunk_( bound_entity_,
                     std::forward<Args>( args )... );
  }
};

// non-const, non-noexcept.
template<typename R, typename... Args>
struct function_ref<R( Args... ) /*const*/ /*noexcept*/> {
 private:
  constexpr static bool kIsNoexcept = false;

  template<typename T>
  static constexpr bool is_callable_in_context =
      kIsNoexcept ? std::is_nothrow_invocable_r_v<R, T, Args...>
                  : std::is_invocable_r_v<R, T, Args...>;

  using Signature = R( Args... );
  using Thunk     = R( void*, Args... );

  Thunk* p_thunk_      = nullptr;
  void*  bound_entity_ = nullptr;

 public:
  function_ref() = delete;

  template<typename F>
  requires( std::is_function_v<F> && is_callable_in_context<F> )
  function_ref( F* f ATTR_LIFETIMEBOUND ) noexcept {
    p_thunk_ = +[]( void* f, Args... args ) -> R {
      return reinterpret_cast<F*>( f )(
          std::forward<Args>( args )... );
    };
    // Note that we're not taking the address of f, we're just
    // converting it directly to a pointer. This prevents holding
    // a dangling point to a temporary pointer, which is easy to
    // cause at the call site.
    bound_entity_ = reinterpret_cast<void*>( f );
  }

  template<typename F>
  requires(
      !std::is_same_v<std::remove_cvref_t<F>, function_ref> &&
      !std::is_member_pointer_v<std::remove_reference_t<F>> &&
      is_callable_in_context<std::remove_reference_t<F>&> )
  constexpr function_ref( F&& f ATTR_LIFETIMEBOUND ) noexcept {
    p_thunk_ = +[]( void* f, Args... args ) -> R {
      // The proposal mentions that it must be LValue-Callable...
      // I think that enables us to do the following, i.e., just
      // converting f to an R lvalue and calling it.
      return ( *static_cast<std::remove_cvref_t<F>*>( f ) )(
          std::forward<Args>( args )... );
    };
    bound_entity_ = std::addressof( f );
  }

  constexpr function_ref( function_ref const& ) noexcept =
      default;
  constexpr function_ref& operator=(
      function_ref const& ) noexcept = default;

  template<typename T>
  requires( !std::is_same_v<T, function_ref> &&
            !std::is_pointer_v<T> )
  function_ref& operator=( T ) = delete;

  bool operator==( std::nullptr_t ) const = delete;

  R operator()( Args... args ) noexcept( kIsNoexcept ) {
    DCHECK( p_thunk_ != nullptr );
    DCHECK( bound_entity_ != nullptr );
    return p_thunk_( bound_entity_,
                     std::forward<Args>( args )... );
  }
};

/****************************************************************
** Deduction guides.
*****************************************************************/
template<typename F>
function_ref( F* ) -> function_ref<F>;

} // namespace base