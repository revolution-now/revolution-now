/****************************************************************
**heap-value.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Represents an object on the heap with value
*              semantics.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "adl-tag.hpp"
#include "attributes.hpp"
#include "zero.hpp"

namespace base {

/****************************************************************
** heap_value
*****************************************************************/
// Holds and maintains sole ownership over an object of type T on
// the heap, but exposes value-like semantics. In particular, it
// always has a value (even upon default construction) and is
// copyable, in which case it makes a new heap allocation and
// copies the object. Note that it also does an allocation on
// move construction (see below).
//
// This is useful in cases where std::unique_ptr might work but
// where you want value-like semantics and copyability.
//
// A moved-from heap_value still has a value (as always), but
// that value is moved-from.
//
// We don't constrain the functions with requires clauses or put
// any type traits in the signatures of the methods because that
// seems to prevent us from using this type with a forward de-
// clared T, which we want to be able to do. Though there are a
// few exceptions that seem to be fine for our use cases.
//
// NOTE: Compare this with the `indirect` type from here:
//
//   https://github.com/jbcoe/value_types
//
// which basically does the same thing. However, note that there
// is an important difference in how move works. heap_value moves
// the underlying value, thus when a heap_value is move con-
// structed it must do an allocation to create the new value to
// move into (and hence it is not no-throw move constructible,
// which is not good). The `indirect` type, however, will just do
// a shallow move similar to unique_ptr; this has a major conse-
// quence that `indirect` has a value-less state, and so it does
// not provide the same guarantees that heap_value does. Specifi-
// cally, it is UB to access `indirect` after it has been moved
// from, and there apparently is no way to check that it is in
// the valueless state. This may not be an issue; in that case,
// probably better to use `indirect` because it will be more ef-
// ficient when moving and has no-throw move semantics, which are
// desirable. Plus, it looks like it's going to be standardized.
//
template<typename T>
requires( !std::is_reference_v<T> )
struct heap_value : zero<heap_value<T>, T*> {
  using base_t = base::zero<heap_value, T*>;

  // Since this object has value semantics, it must be initial-
  // ized with a valid object.
  heap_value() : base_t( new T{} ) {}

  template<typename... Args>
  heap_value( Args&&... args )
    : base_t( new T( std::forward<Args>( args )... ) ) {}

  // A moved-from heap_value still has a value (as always), but
  // that value is moved-from.
  //
  // NOTE: heap_value is NOT no-throw move constructible because
  // it must always do an allocation on any construction, in-
  // cluding move construction. On the bright side, it is
  // no-throw move assignable.
  heap_value( heap_value&& rhs ) noexcept( false )
    : heap_value( std::move( *rhs ) ) {}

  // A copied heap_value will do a deep copy, performaing a heap
  // allocation for the new value.
  heap_value( heap_value const& rhs ) : heap_value( *rhs ) {}

  heap_value& operator=( heap_value const& rhs ) {
    **this = *rhs;
    return *this;
  }

  heap_value& operator=( heap_value&& rhs ) noexcept {
    **this = std::move( *rhs );
    return *this;
  }

  // Since this object has value semantics, assignment will as-
  // sign through to the underlying object.
  template<typename U>
  heap_value& operator=( U&& rhs ) noexcept(
      std::is_nothrow_assignable_v<T, U> ) {
    **this = std::forward<U>( rhs );
    return *this;
  }

  template<typename U>
  heap_value& operator=( heap_value<U> const& rhs ) noexcept(
      std::is_nothrow_assignable_v<T, U> ) {
    **this = *rhs;
    return *this;
  }

  template<typename U>
  heap_value& operator=( heap_value<U>&& rhs ) noexcept(
      std::is_nothrow_assignable_v<T, U> ) {
    **this = std::move( *rhs );
    return *this;
  }

  operator T&() noexcept ATTR_LIFETIMEBOUND {
    // Resource is always non-null.
    return *this->resource();
  }

  operator T const&() const noexcept ATTR_LIFETIMEBOUND {
    // Resource is always non-null.
    return *this->resource();
  }

  T& operator*() noexcept ATTR_LIFETIMEBOUND {
    // Resource is always non-null.
    return *this->resource();
  }

  T const& operator*() const noexcept ATTR_LIFETIMEBOUND {
    // Resource is always non-null.
    return *this->resource();
  }

  T* operator->() noexcept ATTR_LIFETIMEBOUND {
    return this->resource();
  }

  T const* operator->() const noexcept ATTR_LIFETIMEBOUND {
    return this->resource();
  }

  T& get() noexcept ATTR_LIFETIMEBOUND {
    return *this->resource();
  }

  T const& get() const noexcept ATTR_LIFETIMEBOUND {
    return *this->resource();
  }

  friend void to_str( heap_value const& o, std::string& out,
                      tag<heap_value> ) {
    to_str( *o, out, tag<T>{} );
  }

 private:
  friend base_t;

  // Implement base::zero.
  void free_resource() { delete this->resource(); }

  // Implement base::zero.
  T* copy_resource() const { return new T( *this->resource() ); }
};

template<typename T, typename U>
bool operator==( heap_value<T> const& lhs,
                 heap_value<U> const& rhs ) {
  return *lhs == *rhs;
}

template<typename T, typename U>
bool operator==( heap_value<T> const& lhs, U const& rhs ) {
  return *lhs == rhs;
}

} // namespace base
