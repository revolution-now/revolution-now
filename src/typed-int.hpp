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

#include <iostream>

#define TYPED_INT( a )                                                  \
  struct a {                                                            \
    constexpr a() : _( 0 ) {}                                           \
    constexpr explicit a( int n_ ) : _( n_ ) {}                         \
    constexpr a( a const& other ) : _( other._ ) {}                     \
    a const& operator=( a const& other ) {                              \
      _ = other._;                                                      \
      return *this;                                                     \
    }                                                                   \
    a const& operator=( int n ) {                                       \
      _ = n;                                                            \
      return *this;                                                     \
    }                                                                   \
    bool operator==( a other ) const { return _ == other._; }           \
    void operator++( int ) { ++_; }                                     \
    a operator++() { return a( _++ ); }                                 \
    void operator--( int ) { --_; }                                     \
    a operator--() { return a( _-- ); }                                 \
    void operator*=( int n ) { _ *= n; }                                \
    void operator%=( int n ) { _ %= n; }                                \
    void operator/=( int n ) { _ /= n; }                                \
    void operator+=( int n ) { _ += n; }                                \
    void operator-=( int n ) { _ -= n; }                                \
    void operator*=( a other ) { _ *= other._; }                        \
    void operator%=( a other ) { _ %= other._; }                        \
    void operator/=( a other ) { _ /= other._; }                        \
    void operator+=( a other) { _ += other._; }                         \
    /* operator- left out, should yield delta */                        \
    explicit operator int() const { return _; }                         \
    explicit operator double() const { return double( _ ); }            \
    int _;                                                              \
  };                                                                    \
                                                                        \
  inline a operator*( a left, int right ) { return a( left._*right ); } \
  inline a operator*( int left, a right ) { return a( right._*left ); } \
  /* don't define operator*( a left, a right ) */                       \
                                                                        \
  inline a operator/( a left, int right ) { return a( left._/right ); } \
  /* dividing like types yields a dimensionless ratio. */               \
  inline int operator/( a a1, a a2 ) { return a1._/a2._; }              \
  /* don't define operator/( int left, a right ) */                     \
                                                                        \
  inline a operator%( a left, int right ) { return a( left._%right ); } \
  /* mod'ing like types not allowed in general. */                      \
                                                                        \
  inline a operator+( a left, int right ) { return a( left._+right ); } \
  inline a operator+( int left, a right ) { return a( right._+left ); } \
  inline a operator+( a left, a right ) { return a( left._+right._ ); } \
                                                                        \
  inline a operator-( a left, int right ) { return a( left._-right ); } \
  inline a operator-( int left, a right ) { return a( left-right._ ); } \
                                                                        \
  inline bool operator<( a left, int right ) { return left._<right; }   \
  inline bool operator<( int left, a right ) { return left<right._; }   \
  inline bool operator<( a left, a right ) { return left._<right._; }   \
                                                                        \
  inline bool operator>( a left, int right ) { return left._>right; }   \
  inline bool operator>( int left, a right ) { return left>right._; }   \
  inline bool operator>( a left, a right ) { return left._>right._; }   \
                                                                        \
  inline std::ostream& operator<<( std::ostream& out, a what ) {        \
    return (out << what._);                                             \
  }                                                                     \
  inline std::istream& operator>>( std::istream& in, a& what ) {        \
    return (in >> what._);                                              \
  }

namespace rn {

TYPED_INT( X ) // x coordinate
TYPED_INT( Y ) // y coordinate
TYPED_INT( W ) // width
TYPED_INT( H ) // height

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
