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

/****************************************************************
** Single-Ended Flat Queue
*****************************************************************/
// Queue with elements stored in contiguous memory, but lacking
// in pointer stability.
//
// Lacks pointer stability due to occasional reallocations. The
// "reallocation length" represents the position in the vector
// such that if the front element is located beyond it then the
// vector will be reallocated (normalized) to bring the head ele-
// ment to position zero. This prevents the vector from growing
// indefinitely. The min_size is the minimum size that will be
// occupied by the vector at any given time.
template<typename T>
class flat_queue {
public:
  flat_queue( int min_size = 10, int reallocation_size = 1000 )
    : queue_{},
      begin_{0},
      end_{0},
      min_size_( min_size ),
      reallocation_size_( reallocation_size ) {
    queue_.resize( min_size );
    check_invariants();
  }

  // !! Ref returned is not stable.
  Opt<CRef<T>> front() const {
    Opt<CRef<T>> res;
    if( begin_ != end_ ) res = queue_[begin_];
    return res;
  }

  // These are mainly useful for testing.
  int  size() const { return end_ - begin_; }
  int  capacity() const { return queue_.capacity(); }
  bool empty() const { return ( end_ - begin_ == 0 ); }

  void push( T const& item ) {
    if( end_ == queue_.size() )
      queue_.push_back( item );
    else
      queue_[end_] = item;
    ++end_;
    check_invariants();
  }

  Opt<T> pop() {
    Opt<T> res;
    if( begin_ != end_ ) {
      res = std::move( queue_[begin_++] );
      if( begin_ == end_ ) {
        begin_ = end_ = 0;
        if( int( queue_.size() ) > min_size_ ) {
          queue_ = Vec<T>{};
          queue_.resize( min_size_ );
        }
        check_invariants();
      }
      DCHECK( begin_ <= reallocation_size_ );
      if( begin_ == reallocation_size_ ) {
        queue_.erase( queue_.begin(),
                      queue_.begin() + reallocation_size_ );
        end_ -= begin_;
        begin_ = 0;
        check_invariants();
      }
    }
    check_invariants();
    return res;
  }

private:
  void check_invariants() {
    DCHECK( min_size_ > 0 );
    DCHECK( reallocation_size_ >= 10,
            "reallocation_size should be >= 10 to good "
            "performance." );
    DCHECK( min_size_ < reallocation_size_ );
    DCHECK( begin_ >= 0 );
    DCHECK( end_ >= 0 );
    DCHECK( begin_ <= end_ );
    DCHECK( end_ <= int( queue_.size() ) );
    DCHECK( int( queue_.size() ) >= min_size_ );
    DCHECK( begin_ < reallocation_size_ );
  }

  Vec<T>    queue_;
  int       begin_;
  int       end_;
  int const min_size_;
  int const reallocation_size_;
};

} // namespace rn
