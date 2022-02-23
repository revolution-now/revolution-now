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
// copies the object.
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
// clang-format off
template<typename T>
requires( !std::is_reference_v<T> )
struct heap_value : zero<heap_value<T>, T*> {
  // clang-format on
  using base_t = base::zero<heap_value, T*>;

  // Since this object has value semantics, it must be initial-
  // ized with a valid object.
  heap_value() : base_t( new T{} ) {}

  template<typename... Args>
  heap_value( Args&&... args )
    : base_t( new T( std::forward<Args>( args )... ) ) {}

  // A moved-from heap_value still has a value (as always), but
  // that value is moved-from.
  heap_value( heap_value&& rhs ) noexcept
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
  // clang-format off
  template<typename U>
  heap_value& operator=( U&& rhs )
    noexcept( std::is_nothrow_assignable_v<T, U> ) {
    // clang-format on
    **this = std::forward<U>( rhs );
    return *this;
  }

  // clang-format off
  template<typename U>
  heap_value& operator=( heap_value<U> const& rhs )
    noexcept( std::is_nothrow_assignable_v<T, U> ) {
    // clang-format on
    **this = *rhs;
    return *this;
  }

  // clang-format off
  template<typename U>
  heap_value& operator=( heap_value<U>&& rhs )
    noexcept( std::is_nothrow_assignable_v<T, U> ) {
    // clang-format on
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
