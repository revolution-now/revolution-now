/****************************************************************
**flat-deque.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-06.
*
* Description: Deque backed by flat hash map.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fb.hpp"
#include "fmt-helper.hpp"

// C++ standard library
#include <unordered_map>

namespace rn {

template<typename T>
class flat_deque {
public:
  flat_deque()
    : map_{},
      // front_{ std::numeric_limits<uint64_t>::max() / 2 },
      front_{ 10000 },
      back_{ front_ } {
    check_invariants();
  }

  flat_deque( flat_deque<T> const& ) = default;
  flat_deque( flat_deque<T>&& )      = default;

  flat_deque<T>& operator=( flat_deque<T>&& rhs ) noexcept {
    flat_deque<T> moved( std::move( rhs ) );
    moved.swap( *this );
    return *this;
  }

  void swap( flat_deque<T>& rhs ) noexcept {
    std::swap( map_, rhs.map_ );
    std::swap( front_, rhs.front_ );
    std::swap( back_, rhs.back_ );
  }

  friend void swap( flat_deque<T>& lhs,
                    flat_deque<T>& rhs ) noexcept {
    lhs.swap( rhs );
  }

  int  size() const { return back_ - front_; }
  bool empty() const { return size() == 0; }

  // !! Ref returned is not stable.
  maybe<T const&> front() const {
    if( size() > 0 ) {
      DCHECK( map_.contains( front_ ) );
      auto it = map_.find( front_ );
      DCHECK( it != map_.end() );
      return it->second;
    }
    return nothing;
  }

  // !! Ref returned is not stable.
  maybe<T&> front() {
    if( size() > 0 ) {
      DCHECK( map_.contains( front_ ) );
      check_invariants();
      return map_[front_];
    }
    check_invariants();
    return nothing;
  }

  // !! Ref returned is not stable.
  maybe<T const&> back() const {
    if( size() > 0 ) {
      DCHECK( map_.contains( back_ - 1 ) );
      auto it = map_.find( back_ - 1 );
      DCHECK( it != map_.end() );
      return it->second;
    }
    return nothing;
  }

  // !! Ref returned is not stable.
  maybe<T&> back() {
    if( size() > 0 ) {
      DCHECK( map_.contains( back_ - 1 ) );
      check_invariants();
      return map_[back_ - 1];
    }
    check_invariants();
    return nothing;
  }

  void push_back( T const& item ) {
    map_[back_++] = item;
    check_invariants();
  }

  template<typename... Args>
  void push_back_emplace( Args&&... args ) {
    map_.emplace( back_++, std::forward<Args>( args )... );
    check_invariants();
  }

  void push_front( T const& item ) {
    --front_;
    map_[front_] = item;
    check_invariants();
  }

  void pop_front() {
    DCHECK( map_.size() > 0 );
    if( front_ == back_ ) return;
    DCHECK( map_.contains( front_ ) );
    map_.erase( front_++ );
    check_invariants();
  }

  void pop_back() {
    DCHECK( map_.size() > 0 );
    if( front_ == back_ ) return;
    --back_;
    DCHECK( map_.contains( back_ ) );
    map_.erase( back_ );
    check_invariants();
  }

  bool operator==( flat_deque<T> const& rhs ) const {
    if( size() != rhs.size() ) return false;
    DCHECK( map_.size() == rhs.map_.size() );
    if( size() == 0 ) return true;
    int front_lhs = front_;
    int front_rhs = rhs.front_;
    for( int i = 0; i < int( size() ); ++i ) {
      auto lhs_it = map_.find( front_lhs++ );
      auto rhs_it = rhs.map_.find( front_rhs++ );
      DCHECK( lhs_it != map_.end() );
      DCHECK( rhs_it != rhs.map_.end() );
      if( lhs_it->second != rhs_it->second ) return false;
    }
    return true;
  }

  bool operator!=( flat_deque<T> const& rhs ) const {
    return !( ( *this ) == rhs );
  }

  std::string to_string(
      int max_elems = std::numeric_limits<int>::max() ) const {
    std::string res  = "[front:";
    int         back = front_ + std::min( max_elems, size() );
    for( int i = front_; i < back; ++i ) {
      auto it = map_.find( i );
      DCHECK( it != map_.end() );
      res += fmt::format( "{}", it->second );
      if( i != back - 1 ) res += ',';
    }
    if( max_elems < size() ) res += "...";
    res += ']';
    return res;
  }

  // {fmt} formatter.
  friend struct fmt::formatter<flat_deque<T>>;

private:
  void check_overflow() {
    if( back_ == std::numeric_limits<uint64_t>::max() - 1 ||
        front_ == 1 ) {
      flat_deque<T> new_dq;
      for( uint64_t i = front_; i < back_; ++i ) {
        new_dq.push_back_emplace( std::move( map_[i] ) );
      }
      *this = std::move( new_dq );
    }
  }

  void check_invariants() {
    check_overflow();
    CHECK( front_ <= back_ );
    CHECK( map_.size() == ( back_ - front_ ) );
  }

  std::unordered_map<uint64_t, T> map_;
  uint64_t                        front_;
  uint64_t                        back_;
  NOTHROW_MOVE( T );
};

// TODO: needs unit test.
template<typename T>
void deduplicate_deque( flat_deque<T>* q ) {
  flat_deque<T>         new_q;
  std::unordered_set<T> s;
  s.reserve( q->size() );
  while( q->size() > 0 ) {
    auto& item = *q->front();
    q->pop_front();
    if( s.contains( item ) ) continue;
    s.insert( item );
    new_q.push_back( item );
  }
  *q = std::move( new_q );
}

namespace serial {

template<typename Hint, typename T>
auto serialize( FBBuilder& builder, ::rn::flat_deque<T> const& m,
                serial::ADL ) {
  std::vector<T> data;
  data.reserve( size_t( m.size() ) );
  auto m_copy = m;
  while( m_copy.front() ) {
    data.emplace_back( *m_copy.front() );
    m_copy.pop_front();
  }
  return serialize<Hint>( builder, data, serial::ADL{} );
}

template<typename SrcT, typename T>
expect<> deserialize( SrcT const* src, ::rn::flat_deque<T>* m,
                      serial::ADL ) {
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which is
    // an empty queue.
    return xp_success_t{};
  }
  std::vector<T> data;
  XP_OR_RETURN_( deserialize( src, &data, serial::ADL{} ) );
  for( auto& e : data ) m->push_back_emplace( std::move( e ) );
  return xp_success_t{};
}

} // namespace serial

/****************************************************************
** Testing
*****************************************************************/
void test_flat_deque();

} // namespace rn

namespace fmt {
// {fmt} formatter.
template<typename T>
struct formatter<::rn::flat_deque<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::flat_deque<T> const& o,
               FormatContext&             ctx ) {
    std::string res = "[front:";
    for( auto i = o.front_; i < o.back_; ++i ) {
      auto it = o.map_.find( i );
      DCHECK( it != o.map_.end() );
      res += fmt::format( "{}", it->second );
      if( i != o.back_ - 1 ) res += ',';
    }
    res += ']';
    return formatter_base::format( res, ctx );
  }
};
} // namespace fmt
