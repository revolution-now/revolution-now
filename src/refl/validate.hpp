/****************************************************************
**validate.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-03.
*
* Description: Runs recursive validation on traversable types.
*
*****************************************************************/
#pragma once

// refl
#include "ext.hpp"

// traverse
#include "traverse/recursive.hpp"

namespace refl {

namespace detail {

struct ValidationFailed : std::exception {
  ValidationFailed( std::string&& msg )
    : err( std::move( msg ) ) {}
  std::string err;
};

struct Validator : public trv::RecursiveTraverserWithTracking {
  template<typename T>
  void visit( T const& o ) const {
    if constexpr( ValidatableStruct<T> ) {
      if( auto const res = o.validate(); !res.valid() )
        throw ValidationFailed( std::format(
            "{}: error: {}", path(), res.error() ) );
    }
  }
};

} // namespace detail

/****************************************************************
** Recursive Validator
*****************************************************************/
// The value passed in here actually does not need to be validat-
// able, since in general not all children will be (i.e., not
// every type will have a .validate() method). However, it must
// be traversable.
base::valid_or<std::string> validate_recursive(
    trv::Traversable auto const& o,
    std::string_view const top ) {
  using namespace detail;
  Validator v;
  try {
    v( o, top );
  } catch( ValidationFailed const& e ) { return e.err; }
  return base::valid;
}

base::valid_or<std::string> validate_recursive(
    trv::Traversable auto const& o ) {
  using namespace detail;
  Validator v;
  try {
    v( o, trv::none );
  } catch( ValidationFailed const& e ) { return e.err; }
  return base::valid;
}

} // namespace refl
