/****************************************************************
**deferred.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-14.
*
* Description: Simple wrapper imbuing default constructibility.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

namespace rn {

// Simple wrapper around types which are not default con-
// structible and which will permit deferring the construction of
// the wrapped object, e.g., if its constructor parameters are
// not known until a later time. This is similar to an optional
// but does not allow returning to an "empty" state after con-
// structing the wrapped object. std::unique_ptr would also be an
// alternative, but that requires heap allocation, which `de-
// ferred` will avoid.
template<typename T>
class deferred {
public:
  static_assert( !std::is_default_constructible_v<T>,
                 "The `deferred` wrapper is only to be used for "
                 "non-default-constructible types" );

  deferred() {}

  template<typename... Args>
  void emplace( Args&&... args ) {
    val_.emplace( std::forward<Args>( args )... );
  }

  // If you can construct `deferred` with a T then you don't need
  // `deferred`.
  deferred( T const& ) = delete;
  deferred( T&& )      = delete;

  deferred( deferred const& ) = default;
  deferred( deferred&& )      = default;

  T& operator=( T const& rhs ) { val_ = rhs; }
  T& operator=( T&& rhs ) { val_ = std::move( rhs ); }

  bool constructed() const { return val_.has_value(); }

  T& get() {
    DCHECK( val_.has_value() );
    return *val_;
  }
  T const& get() const {
    DCHECK( val_.has_value() );
    return *val_;
  }

  T& operator->() {
    DCHECK( val_.has_value() );
    return *val_;
  }
  T const& operator->() const {
    DCHECK( val_.has_value() );
    return *val_;
  }

private:
  maybe<T> val_;
};

} // namespace rn
