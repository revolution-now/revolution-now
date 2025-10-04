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
#include "traverse/ext.hpp"

// base
#include "base/logger.hpp"
#include "base/scope-exit.hpp"

namespace refl {

namespace detail {

/****************************************************************
** FailedValidationException
*****************************************************************/
struct FailedValidationException {
  std::string err;
};

/****************************************************************
** RecursiveTraversingValidator
*****************************************************************/
struct RecursiveTraversingValidator {
  bool const ENABLE_NOISY_LOGGING = false;

  template<trv::Traversable T, typename K>
  void operator()( T const& o, K const& k ) {
    push_key( stringify_key( k ) );
    SCOPE_EXIT { pop_key(); };
    if( ENABLE_NOISY_LOGGING )
      base::lg.debug( "traversing: {}", path() );
    trv::traverse( o, *this );
    if constexpr( ValidatableStruct<T> ) {
      if( ENABLE_NOISY_LOGGING )
        base::lg.debug( "validating: {}", path() );
      if( auto const res = o.validate(); !res.valid() )
        throw FailedValidationException{
          .err = std::format( "{}: error: {}", path(),
                              res.error() ) };
    }
  }

 private:
  void push_key( std::string key ) {
    keys_.push_back( std::move( key ) );
    if( keys_[0].starts_with( "." ) )
      keys_[0] = keys_[0].substr( 1 );
  }

  void pop_key() {
    CHECK( !keys_.empty() );
    keys_.pop_back();
  }

  std::string path() const {
    std::string p;
    for( auto const& e : keys_ ) p += e;
    return p;
  }

  template<typename K>
  static std::string field( K const& k ) {
    return std::format( ".{}", base::to_str( k ) );
  }

  template<typename K>
  static std::string index( K const& k ) {
    return std::format( "[{}]", base::to_str( k ) );
  }

  inline static std::string const kUnknown = ".?";

  template<typename K>
  std::string stringify_key( K const& k ) const {
    if constexpr( std::is_same_v<K, trv::none_t> ) return "";

    if constexpr( base::Show<K> ) {
      if constexpr( std::is_same_v<K, std::string> ||
                    std::is_same_v<K, std::string_view> )
        return field( k );
      else if constexpr( std::is_array_v<K> ) {
        if constexpr( std::is_same_v<
                          std::remove_cvref_t<decltype( k[0] )>,
                          char> )
          return field( k );
      }

      return index( k );
    }

    return kUnknown;
  }

 private:
  std::vector<std::string> keys_;
};

} // namespace detail

/****************************************************************
** Recursive Validator
*****************************************************************/
// The type T actually does not need to be validatable, since in
// general not all children will be (i.e., not every type will
// have a .validate() method). However, it must be traversable.
template<trv::Traversable T>
base::valid_or<std::string> validate_recursive(
    T const& o, std::string_view const top = "" ) {
  detail::RecursiveTraversingValidator rv;
  try {
    rv( o, top );
  } catch( detail::FailedValidationException const& e ) {
    return e.err;
  }
  return base::valid;
}

} // namespace refl
