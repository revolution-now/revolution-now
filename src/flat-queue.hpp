/****************************************************************
**flat-queue.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-04.
*
* Description: Single Ended Queue in Contiguous Memory.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

namespace rn {

// If the front of the flat-queue gets this many elements beyond
// the beginning of the storage block then there will be a resize
// and/or reallocation to normalize the internal storage so that
// it doesn't grow indefinitely.
inline constexpr int k_flat_queue_reallocation_size_default =
    1000;

/****************************************************************
** Single-Ended Flat Queue
*****************************************************************/
// Queue with elements stored in contiguous memory, but lacking
// in pointer stability.
//
// Lacks pointer stability due to occasional reallocations. The
// "reallocation size" represents the position in the vector such
// that if the front element is located beyond it then the vector
// will be reallocated (normalized) to bring the head element
// to position zero. This prevents the vector from growing
// indefinitely.
template<typename T>
class flat_queue {
public:
  flat_queue( int reallocation_size =
                  k_flat_queue_reallocation_size_default )
    : queue_{},
      front_{0},
      reallocation_size_( reallocation_size ) {
    DCHECK( reallocation_size_ >= 10,
            "reallocation_size should be >= 10 to good "
            "performance (this is just a heuristic)." );
    check_invariants();
  }

  int  size() const { return int( queue_.size() ) - front_; }
  bool empty() const { return size() == 0; }

  // !! Ref returned is not stable.
  Opt<CRef<T>> front() const {
    Opt<CRef<T>> res;
    if( front_ != int( queue_.size() ) ) res = queue_[front_];
    return res;
  }

  // !! Ref returned is not stable.
  Opt<Ref<T>> front() {
    Opt<Ref<T>> res;
    if( front_ != int( queue_.size() ) ) res = queue_[front_];
    check_invariants();
    return res;
  }

  void push( T const& item ) {
    queue_.push_back( item );
    check_invariants();
  }

  template<typename... Args>
  void push_emplace( Args&&... args ) {
    queue_.emplace_back( std::forward<Args>( args )... );
    check_invariants();
  }

  void pop() {
    DCHECK( front_ != int( queue_.size() ) );
    if( front_ == int( queue_.size() ) ) return;
    queue_[front_++].~T();
    if( front_ == reallocation_size_ ) {
      queue_.erase( queue_.begin(),
                    queue_.begin() + reallocation_size_ );
      queue_.shrink_to_fit();
      front_ = 0;
    }
    check_invariants();
  }

private:
  void check_invariants() {
    CHECK( front_ >= 0 );
    CHECK( front_ <= int( queue_.size() ) );
    CHECK( front_ < reallocation_size_ );
  }

  Vec<T> queue_;
  int    front_; // no iterator; would be invalidated.
  int    reallocation_size_;
};

} // namespace rn
