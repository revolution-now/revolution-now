/****************************************************************
* typed-int.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: A type safe integer for representing cartesian
*              coordinates and distances.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include <iostream>
#include <optional>
#include <vector>

// This is a minimal wrapper around an int. It allows nothing ex-
// cept for (explicit) construction from int, copying/assignment,
// equality, and (explicit) conversion back to int.
//
// One use case for this is to wrap an int which will not ever be
// subject to any arithmetic operations, such as IDs.
template<typename Tag>
struct TypedIntMinimal {
  constexpr TypedIntMinimal() : _( 0 ) {}
  constexpr explicit TypedIntMinimal( int n_ ) : _( n_ ) {}
  constexpr TypedIntMinimal( TypedIntMinimal<Tag> const& other )
    : _( other._ ) {}
  TypedIntMinimal<Tag> const& operator=( TypedIntMinimal<Tag> const& other )
    { _ = other._; return *this; }
  TypedIntMinimal<Tag> const& operator=( int n )
    { _ = n; return *this; }
  bool operator==( TypedIntMinimal<Tag> other ) const
    { return _ == other._; }
  bool operator!=( TypedIntMinimal<Tag> other ) const
    { return _ != other._; }
  explicit operator int() const { return _; }

  int _;
};

// This one is as above but adds arithmetic operations.
template<typename Tag>
struct TypedInt : public TypedIntMinimal<Tag> {
  using P = TypedIntMinimal<Tag>; // parent
  constexpr TypedInt() : P( 0 ) {}
  constexpr explicit TypedInt( int n_ ) : P( n_ ) {}
  constexpr TypedInt( TypedInt<Tag> const& other )
    : P( other._ ) {}
  TypedInt<Tag> const& operator=( TypedInt<Tag> const& other ) {
    P::_ = other._;
    return *this;
  }
  TypedInt<Tag> const& operator=( int n ) {
    P::_ = n;
    return *this;
  }
  bool operator==( TypedInt<Tag> other ) const
    { return P::_ == other._; }
  bool operator!=( TypedInt<Tag> other ) const
    { return P::_ != other._; }
  void operator++( int ) { ++P::_; }
  TypedInt<Tag> operator++() { return TypedInt<Tag>( P::_++ ); }
  void operator--( int ) { --P::_; }
  TypedInt<Tag> operator--() { return TypedInt<Tag>( P::_-- ); }
  void operator*=( int n ) { P::_ *= n; }
  void operator%=( int n ) { P::_ %= n; }
  void operator/=( int n ) { P::_ /= n; }
  void operator+=( int n ) { P::_ += n; }
  void operator-=( int n ) { P::_ -= n; }
  void operator*=( TypedInt<Tag> other ) { P::_ *= other._; }
  void operator%=( TypedInt<Tag> other ) { P::_ %= other._; }
  void operator/=( TypedInt<Tag> other ) { P::_ /= other._; }
  void operator+=( TypedInt<Tag> other) { P::_ += other._; }
  /* operator- left out, should yield delta */
  explicit operator int() const { return P::_; }
  explicit operator double() const { return double( P::_ ); }
};

template<typename Tag>
inline TypedInt<Tag> operator*( TypedInt<Tag> left, int right )
  { return TypedInt<Tag>( left._*right ); }
template<typename Tag>
inline TypedInt<Tag> operator*( int left, TypedInt<Tag> right )
  { return TypedInt<Tag>( right._*left ); }
/* no operator*( TypedInt<Tag> left, TypedInt<Tag> right ) */

template<typename Tag>
inline TypedInt<Tag> operator/( TypedInt<Tag> left, int right )
  { return TypedInt<Tag>( left._/right ); }
/* dividing like types yields TypedInt<Tag> dimensionless ratio. */
template<typename Tag>
inline int operator/( TypedInt<Tag> a1, TypedInt<Tag> a2 )
  { return a1._/a2._; }
/* don't define operator/( int left, TypedInt<Tag> right ) */

template<typename Tag>
inline TypedInt<Tag> operator%( TypedInt<Tag> left, int right )
  { return TypedInt<Tag>( left._%right ); }
/* mod'ing like types not allowed in general. */

template<typename Tag>
inline TypedInt<Tag> operator+( TypedInt<Tag> left, int right )
  { return TypedInt<Tag>( left._+right ); }
template<typename Tag>
inline TypedInt<Tag> operator+( int left, TypedInt<Tag> right )
  { return TypedInt<Tag>( right._+left ); }
template<typename Tag>
inline TypedInt<Tag> operator+( TypedInt<Tag> left, TypedInt<Tag> right )
  { return TypedInt<Tag>( left._+right._ ); }

template<typename Tag>
inline TypedInt<Tag> operator-( TypedInt<Tag> left, int right )
  { return TypedInt<Tag>( left._-right ); }
template<typename Tag>
inline TypedInt<Tag> operator-( int left, TypedInt<Tag> right )
  { return TypedInt<Tag>( left-right._ ); }

template<typename Tag>
inline bool operator<( TypedInt<Tag> left, int right )
  { return left._<right; }
template<typename Tag>
inline bool operator<( int left, TypedInt<Tag> right )
  { return left<right._; }
template<typename Tag>
inline bool operator<( TypedInt<Tag> left, TypedInt<Tag> right )
  { return left._<right._; }

template<typename Tag>
inline bool operator>( TypedInt<Tag> left, int right )
  { return left._>right; }
template<typename Tag>
inline bool operator>( int left, TypedInt<Tag> right )
  { return left>right._; }
template<typename Tag>
inline bool operator>( TypedInt<Tag> left, TypedInt<Tag> right )
  { return left._>right._; }

template<typename Tag>
inline bool operator<=( TypedInt<Tag> left, int right )
  { return left._<=right; }
template<typename Tag>
inline bool operator<=( int left, TypedInt<Tag> right )
  { return left<=right._; }
template<typename Tag>
inline bool operator<=( TypedInt<Tag> left, TypedInt<Tag> right )
  { return left._<=right._; }

template<typename Tag>
inline bool operator>=( TypedInt<Tag> left, int right )
  { return left._>=right; }
template<typename Tag>
inline bool operator>=( int left, TypedInt<Tag> right )
  { return left>=right._; }
template<typename Tag>
inline bool operator>=( TypedInt<Tag> left, TypedInt<Tag> right )
  { return left._>=right._; }

template<typename Tag>
inline std::ostream& operator<<( std::ostream& out, TypedInt<Tag> what ) {
  return (out << what._);
}
template<typename Tag>
inline std::istream& operator>>( std::istream& in, TypedInt<Tag>& what ) {
  return (in >> what._);
}

// This is intended to be used inside the std namespace, but
// should be issued outside of the ::rn namespace.
#define DEFINE_HASH_FOR_TYPED_INT( a )             \
  template<> struct hash<a> {                      \
    auto operator()( a const& c ) const noexcept { \
      /* Just delegate to the int hash. */         \
      return ::std::hash<int>{}( c._ );            \
    }                                              \
  };                                               \

// Here we use the Curiously Recurring Template patter so that we
// can a) inherit from a base class and inherit all of its func-
// tions, but at the same time b) maintain that each distinct
// typed in type will be unrelated type-wise to any other. If the
// base classes were not templated then different typed in types
// would have a common base class which would cause trouble for
// the inherited member functions that refer to that base class;
// e.g., it would allow two distinct typed int's to be added to-
// gether.
#define DERIVE_TYPED_INT( a, b )                \
  struct a : public b<a> {                      \
    using P = b<a>; /* parent */                \
    constexpr a() : P( 0 ) {}                   \
    constexpr explicit a( int n_ ) : P( n_ ) {} \
    constexpr a( P ti ) : P( ti ) {}            \
    a const& operator=( a const& other ) {      \
      _ = other._;                              \
      return *this;                             \
    }                                           \
    a const& operator=( int n ) {               \
      _ = n;                                    \
      return *this;                             \
    }                                           \
  };                                            \
  using Opt ## a = std::optional<a>;            \
  using a ## Vec = std::vector<a>;

// Typed ints that are to represent coordinates should use this
// macro. It will ensure that they have types that allow the nec-
// essary operations and no more.
#define TYPED_COORD( a ) \
  DERIVE_TYPED_INT( a, TypedInt )

// Typed ints that are to represent IDs should use this macro. It
// will create a type which is int-like except that one cannot
// perform any arithmetic operations on it, since those would not
// make sense for an ID.
#define TYPED_ID( a ) \
  DERIVE_TYPED_INT( a, TypedIntMinimal )

namespace rn {

TYPED_COORD( X ) // x coordinate
TYPED_COORD( Y ) // y coordinate
TYPED_COORD( W ) // width
TYPED_COORD( H ) // height

// These templates allow us to map a dimension
template<typename Coordinate>
struct LengthTypeFor;

template<> struct LengthTypeFor<X> { using length_t = W; };
template<> struct LengthTypeFor<Y> { using length_t = H; };

template<typename Coordinate>
using LengthType = typename LengthTypeFor<Coordinate>::length_t;

// These express that when we add a delta to a coordinate that we
// get another coordinate. They are specific to the semantics of
// cartesian coordinates so we don't include them in the TYPE-
// D_INT macro.
inline X operator+( X x, W w ) { return X( x._ + w._ ); }
inline Y operator+( Y y, H h ) { return Y( y._ + h._ ); }
inline X operator+( W w, X x ) { return X( x._ + w._ ); }
inline Y operator+( H h, Y y ) { return Y( y._ + h._ ); }
inline X operator-( X x, W w ) { return X( x._ - w._ ); }
inline Y operator-( Y y, H h ) { return Y( y._ - h._ ); }
inline X operator-( W w, X x ) { return X( x._ - w._ ); }
inline Y operator-( H h, Y y ) { return Y( y._ - h._ ); }
// These express that when we subtract two coordinates or
// two deltas then we get a delta (not a coordinate).
inline W operator-( X x1, X x2 ) { return W( x1._ - x2._ ); }
inline H operator-( Y y1, Y y2 ) { return H( y1._ - y2._ ); }
inline W operator-( W x1, W x2 ) { return W( x1._ - x2._ ); }
inline H operator-( H y1, H y2 ) { return H( y1._ - y2._ ); }
// These express that an absolute position divided by a distance
// yields a dimensionless ratio.
inline int operator/( X x, W w ) { return x._/w._; }
inline int operator/( Y y, H h ) { return y._/h._; }
// These express that mod'ing an absolute position by a distance
// or mod'ing two distances yields another distance (i.e., not an
// absolute position).
inline W operator%( X x, W w ) { return W(x._%w._); }
inline H operator%( Y y, H h ) { return H(y._%h._); }
inline W operator%( W w1, W w2 ) { return W(w1._%w2._); }
inline H operator%( H h1, H h2 ) { return H(h1._%h2._); }

} // namespace rn
