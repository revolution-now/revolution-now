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
#include "error.hpp"
#include "maybe.hpp"

// C++ standard library
#include <cassert>

namespace base {

// Any type that needs to manage resources should do so by inher-
// iting from this class.  Derived class needs to implement at
// least the following;
//
//  using resource_type = ...;
//  void free_resource();
//
// and optionally the following if copyability is to be sup-
// ported;
//
//  resource_type copy_resource();
//
// The copy_resource method MUST ALWAYS return something that is
// owned, on which free_resource can be called. copy_resource
// will not be called when there is either no value to be copied
// or if the resource is not owned.
//
// From either of the above two functions, call resource() to get
// access to the resource.
//
template<typename Derived, typename Resource> // CRTP
struct zero {
  zero() = default;
  // Must be a live resource on which we can call free.
  zero( Resource r ) noexcept
    : r_( std::move( r ) ), own_( true ) {}

  zero( Resource r, bool own ) noexcept
    : r_( std::move( r ) ), own_( own ) {}

  ~zero() noexcept { free(); }

  zero( zero const& rhs ) : r_{}, own_{ false } { *this = rhs; }

  // There is some non-trivial logic in this copy operation. If
  // `rhs` does not own its resource, then it is OK for us to
  // just do a dumb copy. Otherwise, if it does own it, then we
  // need to ask it to make a proper copy (where a "copy" here is
  // defined as something that we will own and on which we can
  // later call free_resource).
  zero& operator=( zero const& rhs ) {
    if( this == &rhs ) return *this;
    free();
    if( rhs.own_ ) {
      r_   = rhs.copy();
      own_ = true;
    } else {
      r_   = rhs.r_;
      own_ = rhs.own_; // false
    }
    return *this;
  }

  zero( zero&& rhs ) noexcept
    : r_( std::move( rhs.r_ ) ), own_( rhs.own_ ) {
    rhs.reset();
  }

  zero& operator=( zero&& rhs ) noexcept {
    if( this == &rhs ) return *this;
    free();
    r_   = std::move( rhs.r_ );
    own_ = rhs.own_;
    rhs.reset();
    return *this;
  }

  bool has_value() const noexcept { return r_.has_value(); }
  bool own() const noexcept { return own_; }

  // These relinquishes ownership over the resource, but does not
  // free it.  I.e., it can cause leaks.
  void relinquish() noexcept { own_ = false; }

  // Free the resource.
  void free() {
    if( !has_value() || !own() ) return;
    derived().free_resource();
    reset();
  }

  // These can go BOOM! Must check alive() first.
  Resource& resource() {
    DCHECK( r_.has_value() );
    return *r_;
  }

  Resource const& resource() const {
    DCHECK( r_.has_value() );
    return *r_;
  }

 private:
  Derived& derived() noexcept {
    return *static_cast<Derived*>( this );
  }
  Derived const& derived() const noexcept {
    return *static_cast<Derived const*>( this );
  }

  void reset() {
    r_.reset();
    own_ = false;
  }

  // This should return a resource that can be freed indepen-
  // dently of this one, i.e., one that we can take ownership
  // over. Depending on how the resource works, it doesn't have
  // to be a physically different entity, it just has to be some-
  // thing on which .free() can be called independently without
  // triggering a double-free problem, e.g., it could increase a
  // reference count on something. Or, of course, it could do a
  // deep clone of the resource as well.
  maybe<Resource> copy() const {
    if( !has_value() ) return nothing;
    return derived().copy_resource();
  }

 protected:
  maybe<Resource> r_;
  bool            own_ = false;
};

} // namespace base
