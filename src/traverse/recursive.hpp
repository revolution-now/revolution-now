/****************************************************************
**recursive.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-05-23.
*
* Description: Helpers for doing recursive traversal.
*
*****************************************************************/
#pragma once

// traverse
#include "ext.hpp"

// base
#include "base/scope-exit.hpp"

// C++ standard library
#include <string>
#include <string_view>
#include <vector>

namespace trv {

/****************************************************************
** RecursiveTraverser
*****************************************************************/
// Breadth-first recursive traverser. Note that this does not
// supply the key to the function for those that won't need it.
// If you do need the key (e.g. for logging or error reporting)
// then you should use the tracking variant below.
template<trv::Traversable T, typename Fn>
void traverse_recursive( T const& o, Fn&& fn ) {
  traverse( o, [&]( auto& e, auto const& /*key*/ ) {
    traverse_recursive( e, fn );
  } );
  fn( o );
}

/****************************************************************
** RecursiveTraverserWithTracking
*****************************************************************/
// Use this if you need to track the position in the traversal as
// a string for logging or error message purposes. If you don't
// need that then use the slimmer `traverse_recursive` function.
struct RecursiveTraverserWithTracking {
  template<typename T>
  requires Traversable<std::remove_reference_t<T>>
  void operator()( this auto&& self, T&& o ) {
    self( o, none );
  }

  template<typename T, typename K>
  requires Traversable<std::remove_reference_t<T>>
  void operator()( this auto&& self, T&& o, K const& k ) {
    self.push_key( self.stringify_key( k ) );
    SCOPE_EXIT { self.pop_key(); };
    trv::traverse( o, self );
    self.visit( o );
  }

 protected:
  // This is slow so it should only be called for error logging
  // or when verbose logging is enabled.
  std::string path() const;

 private:
  void push_key( std::string&& key );
  void pop_key();

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
  int len_ = 0;
};

} // namespace trv
