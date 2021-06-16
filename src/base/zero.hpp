/****************************************************************
**zero.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Base class to enable The Rule of Zero.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

namespace base {

// Any type that needs to manage resources should do so by inher-
// iting from this class.  Derived class needs to implement at
// least the following:
//
//  using resource_type = ...;
//  static void free_resource( resource_type const& r );
//
// and optionally the following if copyability is to be sup-
// ported:
//
//  static resource_type copy_resource( resource_type const& r );
//
template<typename Derived, typename Resource> // CRTP
struct RuleOfZero {
  RuleOfZero() = default;
  // Must be a live resource on which we can call free.
  RuleOfZero( Resource r ) noexcept : r_( std::move( r ) ) {}

  ~RuleOfZero() noexcept { free(); }

  RuleOfZero( RuleOfZero const& rhs ) : r_( rhs.copy() ) {}

  RuleOfZero& operator=( RuleOfZero const& rhs ) {
    if( this == &rhs ) return *this;
    free();
    r_ = rhs.copy();
    return *this;
  }

  RuleOfZero( RuleOfZero&& rhs ) noexcept
    : r_( std::move( rhs.r_ ) ) {
    // A moved-from maybe type is not equal to nothing.
    rhs.relinquish();
  }

  RuleOfZero& operator=( RuleOfZero&& rhs ) noexcept {
    if( this == &rhs ) return *this;
    free();
    r_ = std::move( rhs.r_ );
    rhs.relinquish();
    return *this;
  }

  bool alive() const noexcept { return r_.has_value(); }

  // These relinquishes ownership over the resource, but does not
  // free it. Here we are assuming that the resource in question
  // is not freed by merely destorying the Resource object, as it
  // would be if Resource were a pointer to allocated memory.
  void relinquish() noexcept { r_.reset(); }

  // Free the resource.
  void free() {
    if( !alive() ) return;
    // We are assuming here that the act of freeing the resource
    // does not require modifying the Resource object; i.e., the
    // Resource object is just usually a handle.
    Derived::free_resource( *std::as_const( r_ ) );
    relinquish();
  }

  // These can go BOOM! Must check alive() first.
  Resource& resource() {
    DCHECK( r_.has_value(), "is this a moved-from object?" );
    return *r_;
  }

  Resource const& resource() const {
    DCHECK( r_.has_value(), "is this a moved-from object?" );
    return *r_;
  }

private:
  // This should return a resource that can be freed indepen-
  // dently of this one. Depending on how the resource works, it
  // doesn't have to be a physically different entity, it just
  // has to be something on which .free() can be called indepen-
  // dently without triggering a double-free problem, e.g., it
  // could increase a reference count on something. Or, of
  // course, it could do a deep clone of the resource as well.
  maybe<Resource> copy() const {
    if( !alive() ) return nothing;
    return Derived::copy_resource( *r_ );
  }

protected:
  maybe<Resource> r_;
};

} // namespace base
