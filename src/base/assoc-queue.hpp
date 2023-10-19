/****************************************************************
**assoc-queue.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-10-18.
*
* Description: A queue that supports O(1) push, pop, and removal
*              from the middle by value, but which does not
*              support duplicate elements.
*
*****************************************************************/
#pragma once

// base
#include "error.hpp"

// C++ standard library
#include <list>
#include <unordered_map>

namespace base {

/****************************************************************
** AssociativeQueue
*****************************************************************/
// A queue that supports O(1) push, pop, and removal from the
// middle by value, but which does not support duplicate ele-
// ments.
//
// NOTE: does not support duplicate elements.
template<typename T>
struct AssociativeQueue {
  int size() const { return elems_.size(); }

  bool empty() const { return elems_.empty(); }

  template<typename U>
  void push( U&& elem ) {
    CHECK( !iters_.contains( elem ) );
    // Get this first before we move out of elem.
    auto& ref = iters_[elem];
    ref = elems_.insert( elems_.end(), std::forward<U>( elem ) );
  }

  void pop() {
    CHECK( !empty() );
    iters_.erase( elems_.front() );
    elems_.pop_front();
  }

  T const& front() const {
    CHECK( !empty() );
    return *elems_.begin();
  }

  // NOTE: this returns a const as well, since we can't allow
  // modifying elements otherwise it would invalidate the map
  // keys. We could delete this function, be we allow calling if
  // for convenience otherwise whenever we call it on a non-const
  // object we will have to as_const is first.
  T const& front() {
    CHECK( !empty() );
    return *elems_.begin();
  }

  void erase( T const& elem ) {
    auto it = iters_.find( elem );
    if( it == iters_.end() ) return;
    elems_.erase( it->second );
    iters_.erase( it );
  }

 private:
  std::list<T>                                           elems_;
  std::unordered_map<T, typename std::list<T>::iterator> iters_;
};

} // namespace base
