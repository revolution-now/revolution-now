/****************************************************************
**typed-int.hpp
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

// Rcl
#include "rcl/ext.hpp"

// Cdr
#include "cdr/ext.hpp"

// luapp
#include "luapp/ext.hpp"

// This is a minimal wrapper around an T. It allows nothing ex-
// cept for (explicit) construction from T, copying/assignment,
// equality, ordering, and (explicit) conversion back to T.
//
// One use case for this is to wrap an T which will not ever be
// subject to any arithmetic operations, such as IDs.
template<typename T, typename Tag>
struct TypedNumMinimal {
  constexpr TypedNumMinimal() = default;
  ~TypedNumMinimal()          = default;
  constexpr explicit TypedNumMinimal( T n_ ) : _( n_ ) {}
  constexpr bool operator==(
      TypedNumMinimal<T, Tag> const rhs ) const {
    return _ == rhs._;
  }
  constexpr bool operator!=(
      TypedNumMinimal<T, Tag> const rhs ) const {
    return _ != rhs._;
  }
  constexpr explicit operator T() const { return _; }

  T _{ 0 };
};

template<typename Tag>
using TypedIntMinimal = TypedNumMinimal<int, Tag>;
template<typename Tag>
using TypedDoubleMinimal = TypedNumMinimal<double, Tag>;

// This one is as above but adds arithmetic operations.
template<typename Tag>
struct TypedInt : public TypedIntMinimal<Tag> {
  using P = TypedIntMinimal<Tag>; // parent
  constexpr TypedInt() : P( 0 ) {}
  ~TypedInt() = default;
  constexpr explicit TypedInt( int n_ ) : P( n_ ) {}
  constexpr TypedInt( TypedInt<Tag> const& other )
    : P( other._ ) {}
  constexpr TypedInt( TypedInt<Tag>&& other ) noexcept
    : P( other._ ) {}
  constexpr bool operator==( TypedInt<Tag> const& rhs ) const {
    return P::_ == rhs._;
  }
  constexpr bool operator!=( TypedInt<Tag> const& rhs ) const {
    return P::_ != rhs._;
  }
  constexpr bool operator<( TypedInt<Tag> const& rhs ) const {
    return P::_ < rhs._;
  }
  constexpr bool operator>( TypedInt<Tag> const& rhs ) const {
    return P::_ > rhs._;
  }
  constexpr bool operator<=( TypedInt<Tag> const& rhs ) const {
    return P::_ <= rhs._;
  }
  constexpr bool operator>=( TypedInt<Tag> const& rhs ) const {
    return P::_ >= rhs._;
  }
  TypedInt<Tag>& operator=( TypedInt<Tag> const& other ) {
    P::_ = other._;
    return *this;
  }
  TypedInt<Tag>& operator=( TypedInt<Tag>&& other ) noexcept {
    P::_ = other._;
    return *this;
  }
  TypedInt<Tag>& operator=( int n ) {
    P::_ = n;
    return *this;
  }
  TypedInt<Tag> operator-() const {
    return TypedInt<Tag>( -P::_ );
  }
  void          operator++( int ) { ++P::_; }
  TypedInt<Tag> operator++() { return TypedInt<Tag>( P::_++ ); }
  void          operator--( int ) { --P::_; }
  TypedInt<Tag> operator--() { return TypedInt<Tag>( P::_-- ); }
  /*operator*=( int n ) left out */
  void operator%=( int n ) { P::_ %= n; }
  /*operator/=( int n ) left out */
  void operator+=( int n ) { P::_ += n; }
  void operator-=( int n ) { P::_ -= n; }
  void operator*=( TypedInt<Tag> other ) { P::_ *= other._; }
  void operator%=( TypedInt<Tag> other ) { P::_ %= other._; }
  /* operator/= left out, should yield dimensionless ratio */
  void operator+=( TypedInt<Tag> other ) { P::_ += other._; }
  /* operator- left out, should yield delta */
  explicit operator int() const { return P::_; }
  explicit operator double() const { return double( P::_ ); }
};

// This one is as above but adds arithmetic operations.
template<typename Tag>
struct TypedDouble : public TypedDoubleMinimal<Tag> {
  using P = TypedDoubleMinimal<Tag>; // parent
  constexpr TypedDouble() : P( 0 ) {}
  ~TypedDouble() = default;
  constexpr explicit TypedDouble( double n_ ) : P( n_ ) {}
  constexpr TypedDouble( TypedDouble<Tag> const& other )
    : P( other._ ) {}
  constexpr TypedDouble( TypedDouble<Tag>&& other ) noexcept
    : P( other._ ) {}
  TypedDouble<Tag>& operator=( TypedDouble<Tag> const& other ) {
    P::_ = other._;
    return *this;
  }
  TypedDouble<Tag>& operator=(
      TypedDouble<Tag>&& other ) noexcept {
    P::_ = other._;
    return *this;
  }
  TypedDouble<Tag>& operator=( double n ) {
    P::_ = n;
    return *this;
  }
  constexpr bool operator==( TypedDouble<Tag> other ) const {
    return P::_ == other._;
  }
  constexpr bool operator!=( TypedDouble<Tag> other ) const {
    return P::_ != other._;
  }
  TypedDouble<Tag> operator-() {
    return TypedDouble<Tag>( -P::_ );
  }
  /*operator*=( double n ) left out */
  void operator%=( double n ) { P::_ %= n; }
  /*operator/=( double n ) left out */
  void operator+=( double n ) { P::_ += n; }
  void operator-=( double n ) { P::_ -= n; }
  void operator*=( TypedDouble<Tag> other ) { P::_ *= other._; }
  void operator%=( TypedDouble<Tag> other ) { P::_ %= other._; }
  /* operator/= left out, should yield dimensionless ratio */
  void operator+=( TypedDouble<Tag> other ) { P::_ += other._; }
  /* operator- left out, should yield delta */
  explicit operator double() const { return P::_; }
};

/** TypedDouble
 * ***************************************************/
template<typename Tag>
inline constexpr TypedDouble<Tag> operator*(
    TypedDouble<Tag> left, TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left._ * right._ );
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator/(
    TypedDouble<Tag> left, double right ) {
  return TypedDouble<Tag>( left._ / right );
}
/* dividing like types yields TypedDouble<Tag> dimensionless
 * ratio.
 */
template<typename Tag>
inline constexpr double operator/( TypedDouble<Tag> a1,
                                   TypedDouble<Tag> a2 ) {
  return a1._ / a2._;
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator+(
    TypedDouble<Tag> left, double right ) {
  return TypedDouble<Tag>( left._ + right );
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator+(
    double left, TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( right._ + left );
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator+(
    TypedDouble<Tag> left, TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left._ + right._ );
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator-(
    TypedDouble<Tag> left, double right ) {
  return TypedDouble<Tag>( left._ - right );
}

template<typename Tag>
inline constexpr TypedDouble<Tag> operator-(
    double left, TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left - right._ );
}

template<typename Tag>
inline constexpr bool operator<( TypedDouble<Tag> left,
                                 double           right ) {
  return left._ < right;
}

template<typename Tag>
inline constexpr bool operator<( double           left,
                                 TypedDouble<Tag> right ) {
  return left < right._;
}

template<typename Tag>
inline constexpr bool operator<( TypedDouble<Tag> left,
                                 TypedDouble<Tag> right ) {
  return left._ < right._;
}

template<typename Tag>
inline constexpr bool operator>( TypedDouble<Tag> left,
                                 double           right ) {
  return left._ > right;
}

template<typename Tag>
inline constexpr bool operator>( double           left,
                                 TypedDouble<Tag> right ) {
  return left > right._;
}

template<typename Tag>
inline constexpr bool operator>( TypedDouble<Tag> left,
                                 TypedDouble<Tag> right ) {
  return left._ > right._;
}

template<typename Tag>
inline constexpr bool operator<=( TypedDouble<Tag> left,
                                  double           right ) {
  return left._ <= right;
}

template<typename Tag>
inline constexpr bool operator<=( double           left,
                                  TypedDouble<Tag> right ) {
  return left <= right._;
}

template<typename Tag>
inline constexpr bool operator<=( TypedDouble<Tag> left,
                                  TypedDouble<Tag> right ) {
  return left._ <= right._;
}

template<typename Tag>
inline constexpr bool operator>=( TypedDouble<Tag> left,
                                  double           right ) {
  return left._ >= right;
}

template<typename Tag>
inline constexpr bool operator>=( double           left,
                                  TypedDouble<Tag> right ) {
  return left >= right._;
}

template<typename Tag>
inline constexpr bool operator>=( TypedDouble<Tag> left,
                                  TypedDouble<Tag> right ) {
  return left._ >= right._;
}

/** TypedInt ***************************************************/
template<typename Tag>
inline constexpr TypedInt<Tag> operator*( TypedInt<Tag> left,
                                          TypedInt<Tag> right ) {
  return TypedInt<Tag>( left._ * right._ );
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator/( TypedInt<Tag> left,
                                          int           right ) {
  return TypedInt<Tag>( left._ / right );
}
/* dividing like types yields TypedInt<Tag> dimensionless ratio.
 */
template<typename Tag>
inline constexpr int operator/( TypedInt<Tag> a1,
                                TypedInt<Tag> a2 ) {
  return a1._ / a2._;
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator+( TypedInt<Tag> left,
                                          int           right ) {
  return TypedInt<Tag>( left._ + right );
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator+( int           left,
                                          TypedInt<Tag> right ) {
  return TypedInt<Tag>( right._ + left );
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator+( TypedInt<Tag> left,
                                          TypedInt<Tag> right ) {
  return TypedInt<Tag>( left._ + right._ );
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator-( TypedInt<Tag> left,
                                          int           right ) {
  return TypedInt<Tag>( left._ - right );
}

template<typename Tag>
inline constexpr TypedInt<Tag> operator-( int           left,
                                          TypedInt<Tag> right ) {
  return TypedInt<Tag>( left - right._ );
}

template<typename Tag>
inline constexpr bool operator<( TypedInt<Tag> left,
                                 int           right ) {
  return left._ < right;
}

template<typename Tag>
inline constexpr bool operator<( int           left,
                                 TypedInt<Tag> right ) {
  return left < right._;
}

template<typename Tag>
inline constexpr bool operator>( TypedInt<Tag> left,
                                 int           right ) {
  return left._ > right;
}

template<typename Tag>
inline constexpr bool operator>( int           left,
                                 TypedInt<Tag> right ) {
  return left > right._;
}

template<typename Tag>
inline constexpr bool operator<=( TypedInt<Tag> left,
                                  int           right ) {
  return left._ <= right;
}

template<typename Tag>
inline constexpr bool operator<=( int           left,
                                  TypedInt<Tag> right ) {
  return left <= right._;
}

template<typename Tag>
inline constexpr bool operator>=( TypedInt<Tag> left,
                                  int           right ) {
  return left._ >= right;
}

template<typename Tag>
inline constexpr bool operator>=( int           left,
                                  TypedInt<Tag> right ) {
  return left >= right._;
}

// This is intended to be used inside the std namespace, but
// should be issued outside of the ::rn namespace.
#define DEFINE_HASH_FOR_TYPED_NUM( t, a )          \
  template<>                                       \
  struct hash<a> {                                 \
    auto operator()( a const& c ) const noexcept { \
      /* Just delegate to the hash. */             \
      return ::std::hash<t>{}( c._ );              \
    }                                              \
  };

#define DEFINE_HASH_FOR_TYPED_INT( a ) \
  DEFINE_HASH_FOR_TYPED_NUM( int, a )
#define DEFINE_HASH_FOR_TYPED_DOUBLE( a ) \
  DEFINE_HASH_FOR_TYPED_NUM( double, a )

// Here we use the Curiously Recurring Template patter so that we
// can a) inherit from a base class and inherit all of its func-
// tions, but at the same time b) maintain that each distinct
// typed in type will be unrelated type-wise to any other. If the
// base classes were not templated then different typed in types
// would have a common base class which would cause trouble for
// the inherited member functions that refer to that base class;
// e.g., it would allow two distinct typed num's to be added to-
// gether.
#define DERIVE_TYPED_NUM_NS( ns, t, a, b, suffix )    \
  namespace ns {                                      \
  struct a : public b<a> {                            \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */  \
    using P = b<a>; /* parent */                      \
    constexpr a() : P( 0 ) {}                         \
    constexpr a( a const& rhs ) = default;            \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */  \
    constexpr a( a&& rhs ) = default;                 \
    ~a()                   = default;                 \
    explicit constexpr a( t n_ ) : P( n_ ) {}         \
    constexpr a( P const& ti ) : P( ti ) {}           \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */  \
    a& operator=( a const& other ) {                  \
      _ = other._;                                    \
      return *this;                                   \
    }                                                 \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */  \
    a& operator=( a&& other ) noexcept {              \
      _ = other._;                                    \
      return *this;                                   \
    }                                                 \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */  \
    a& operator=( t n ) {                             \
      _ = n;                                          \
      return *this;                                   \
    }                                                 \
    constexpr bool operator==( a const& rhs ) const { \
      return P::_ == rhs._;                           \
    }                                                 \
    constexpr bool operator!=( a const& rhs ) const { \
      return P::_ != rhs._;                           \
    }                                                 \
    constexpr bool operator<( a const& rhs ) const {  \
      return P::_ < rhs._;                            \
    }                                                 \
    constexpr bool operator>( a const& rhs ) const {  \
      return P::_ > rhs._;                            \
    }                                                 \
    constexpr bool operator<=( a const& rhs ) const { \
      return P::_ <= rhs._;                           \
    }                                                 \
    constexpr bool operator>=( a const& rhs ) const { \
      return P::_ >= rhs._;                           \
    }                                                 \
  };                                                  \
  inline void to_str( a const& o, std::string& out,   \
                      ::base::ADL_t ) {               \
    to_str( o._, out, ::base::ADL );                  \
    out += "_";                                       \
    out += #suffix;                                   \
  }                                                   \
  }                                                   \
  NOTHROW_MOVE( ::ns::a );

#define DERIVE_TYPED_NUM( ... ) \
  DERIVE_TYPED_NUM_NS( rn, __VA_ARGS__ )

// Typed nums that are to represent coordinates should use this
// macro. It will ensure that they have types that allow the nec-
// essary operations and no more.
#define TYPED_COORD( t, a, s ) \
  DERIVE_TYPED_NUM( t, a, TypedInt, s )

// Typed nums that are to represent IDs should use this macro. It
// will create a type which is int-like except that one cannot
// perform any arithmetic operations on it, since those would not
// make sense for an ID.
#define TYPED_ID_NS( ns, a ) \
  DERIVE_TYPED_NUM_NS( ns, int, a, TypedIntMinimal, id )

#define TYPED_ID( a ) TYPED_ID_NS( rn, a )

#define TYPED_INDEX( a ) \
  DERIVE_TYPED_NUM( int, a, TypedInt, idx )

#define TYPED_INT( a, suffix )                                 \
  DERIVE_TYPED_NUM( int, a, TypedInt, suffix )                 \
  inline ::rn::a operator""_##suffix( unsigned long long n ) { \
    return ::rn::a{ static_cast<int>( n ) };                   \
  }

// Scales are numbers that can only be multiplied by themselves
// or by the corresponding coordinate/length type.
#define TYPED_SCALE( t, a, s ) \
  DERIVE_TYPED_NUM( t, a, TypedIntMinimal, s )

// FIXME: move X,Y,W,H out of this header.
TYPED_COORD( int, X, x ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( int, Y, y ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( int, W, w ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( int, H, h ) // NOLINT(hicpp-explicit-conversions)

TYPED_COORD( double, XD,
             xd ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( double, YD,
             yd ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( double, WD,
             wd ) // NOLINT(hicpp-explicit-conversions)
TYPED_COORD( double, HD,
             hd ) // NOLINT(hicpp-explicit-conversions)

// These are "scales"; they are numbers that can be used to scale
// X/Y/W/H.
// FIXME: get rid of these.
TYPED_SCALE( int, SX, sx ) // NOLINT(hicpp-explicit-conversions)
TYPED_SCALE( int, SY, sy ) // NOLINT(hicpp-explicit-conversions)
TYPED_SCALE( double, SXD,
             sxd ) // NOLINT(hicpp-explicit-conversions)
TYPED_SCALE( double, SYD,
             syd ) // NOLINT(hicpp-explicit-conversions)

// User-defined literals.  These allow us to do e.g.:
//   auto x = 55_x; // `x` is of type X
#define UD_LITERAL( a, b )                                  \
  namespace rn {                                            \
  constexpr a operator"" _##b( unsigned long long int n ) { \
    return a( static_cast<decltype( a::_ )>( n ) );         \
  }                                                         \
  }
#define UD_LITERAL_DOUBLE( a, b )                   \
  namespace rn {                                    \
  constexpr a operator"" _##b( long double n ) {    \
    return a( static_cast<decltype( a::_ )>( n ) ); \
  }                                                 \
  }

// FIXME: move X,Y,W,H out of this header.
UD_LITERAL( X, x )
UD_LITERAL( Y, y )
UD_LITERAL( W, w )
UD_LITERAL( H, h )
UD_LITERAL( SX, sx )
UD_LITERAL( SY, sy )

UD_LITERAL_DOUBLE( XD, xd )
UD_LITERAL_DOUBLE( YD, yd )
UD_LITERAL_DOUBLE( WD, wd )
UD_LITERAL_DOUBLE( HD, hd )
UD_LITERAL_DOUBLE( SXD, sxd )
UD_LITERAL_DOUBLE( SYD, syd )

namespace rn {

// These templates allow us to map a dimension
template<typename Coordinate>
struct LengthTypeFor;
template<typename Coordinate>
struct ScaleTypeFor;

template<>
struct LengthTypeFor<X> {
  using length_t = W;
};
template<>
struct LengthTypeFor<Y> {
  using length_t = H;
};

template<>
struct ScaleTypeFor<X> {
  using scale_t = SX;
};
template<>
struct ScaleTypeFor<Y> {
  using scale_t = SY;
};

template<>
struct LengthTypeFor<XD> {
  using length_t = WD;
};
template<>
struct LengthTypeFor<YD> {
  using length_t = HD;
};

template<typename Coordinate>
using LengthType = typename LengthTypeFor<Coordinate>::length_t;

// FIXME: move X,Y,W,H out of this header.
// These express that when we add a delta to a coordinate that we
// get another coordinate. They are specific to the semantics of
// cartesian coordinates so we don't include them in the TYPE-
// D_INT macro.
// clang-format off
inline constexpr X    operator+( X x, W w ) { return X( x._ + w._ ); }
inline constexpr Y    operator+( Y y, H h ) { return Y( y._ + h._ ); }
inline constexpr X    operator+( W w, X x ) { return X( x._ + w._ ); }
inline constexpr Y    operator+( H h, Y y ) { return Y( y._ + h._ ); }
inline constexpr X    operator-( X x, W w ) { return X( x._ - w._ ); }
inline constexpr Y    operator-( Y y, H h ) { return Y( y._ - h._ ); }
inline constexpr X    operator-( W w, X x ) { return X( x._ - w._ ); }
inline constexpr Y    operator-( H h, Y y ) { return Y( y._ - h._ ); }
inline constexpr void operator+=( X& x, W w ) { x._ += w._; }
inline constexpr void operator+=( Y& y, H h ) { y._ += h._; }
inline constexpr void operator-=( X& x, W w ) { x._ -= w._; }
inline constexpr void operator-=( Y& y, H h ) { y._ -= h._; }
// We can allow deltas to subtract from eachother in a mutating
// way. We don't just put these in the above generic classes
// because we don't want to allow e.g. x1 -= x2; this is because
// subtracting a coordinate from another one should only yield a
// width, and so that statement would not make sense.
inline constexpr void operator-=( W& w1, W w2 ) { w1._ -= w2._; }
inline constexpr void operator-=( H& h1, H h2 ) { h1._ -= h2._; }
// These express that when we subtract two coordinates or
// two deltas then we get a delta (not a coordinate).
inline constexpr W operator-( X x1, X x2 ) { return W( x1._ - x2._ ); }
inline constexpr H operator-( Y y1, Y y2 ) { return H( y1._ - y2._ ); }
inline constexpr W operator-( W x1, W x2 ) { return W( x1._ - x2._ ); }
inline constexpr H operator-( H y1, H y2 ) { return H( y1._ - y2._ ); }
// These express that an absolute position divided by a distance
// yields a dimensionless ratio.
inline constexpr int operator/( X x, W w ) { return x._ / w._; }
inline constexpr int operator/( Y y, H h ) { return y._ / h._; }
// These express that mod'ing an absolute position by a distance
// or mod'ing two distances yields another distance (i.e., not an
// absolute position).
inline constexpr W operator%( X x, W w ) { return W( x._ % w._ ); }
inline constexpr H operator%( Y y, H h ) { return H( y._ % h._ ); }
inline constexpr W operator%( W w1, W w2 ) { return W( w1._ % w2._ ); }
inline constexpr H operator%( H h1, H h2 ) { return H( h1._ % h2._ ); }
inline constexpr W operator%( X x, SX sx ) { return W( x._ % sx._ ); }
inline constexpr H operator%( Y y, SY sy ) { return H( y._ % sy._ ); }
inline constexpr W operator%( W w, SX sx ) { return W( w._ % sx._ ); }
inline constexpr H operator%( H h, SY sy ) { return H( h._ % sy._ ); }
// These express that one can only multiply or divide a typed int
// by the appropriate scaling type.
inline constexpr X    operator*( X x, SX sx ) { return X( x._ * sx._ ); }
inline constexpr Y    operator*( Y y, SY sy ) { return Y( y._ * sy._ ); }
inline constexpr SX   operator*( SX s1, SX s2 ) { return SX( s1._ * s2._ ); }
inline constexpr SY   operator*( SY s1, SY s2 ) { return SY( s1._ * s2._ ); }
inline constexpr X    operator/( X x, SX sx ) { return X( x._ / sx._ ); }
inline constexpr Y    operator/( Y y, SY sy ) { return Y( y._ / sy._ ); }
inline constexpr SX   operator/( SX s1, SX s2 ) { return SX( s1._ / s2._ ); }
inline constexpr SY   operator/( SY s1, SY s2 ) { return SY( s1._ / s2._ ); }
inline constexpr W    operator*( W w, SX sx ) { return W( w._ * sx._ ); }
inline constexpr H    operator*( H h, SY sy ) { return H( h._ * sy._ ); }
inline constexpr W    operator/( W w, SX sx ) { return W( w._ / sx._ ); }
inline constexpr H    operator/( H h, SY sy ) { return H( h._ / sy._ ); }
inline constexpr void operator*=( X& x, SX sx ) { x._ *= sx._; }
inline constexpr void operator*=( Y& y, SY sy ) { y._ *= sy._; }
inline constexpr void operator/=( X& x, SX sx ) { x._ /= sx._; }
inline constexpr void operator/=( Y& y, SY sy ) { y._ /= sy._; }
inline constexpr void operator*=( W& w, SX sx ) { w._ *= sx._; }
inline constexpr void operator*=( H& h, SY sy ) { h._ *= sy._; }
inline constexpr void operator/=( W& w, SX sx ) { w._ /= sx._; }
inline constexpr void operator/=( H& h, SY sy ) { h._ /= sy._; }

// These express that when we add a delta to a coordinate that we
// get another coordinate. They are specific to the semantics of
// cartesian coordinates so we don't include them in the TYDPE-
// D_INT macro.
inline constexpr XD   operator+( XD x, WD w ) { return XD( x._ + w._ ); }
inline constexpr YD   operator+( YD y, HD h ) { return YD( y._ + h._ ); }
inline constexpr XD   operator+( WD w, XD x ) { return XD( x._ + w._ ); }
inline constexpr YD   operator+( HD h, YD y ) { return YD( y._ + h._ ); }
inline constexpr XD   operator-( XD x, WD w ) { return XD( x._ - w._ ); }
inline constexpr YD   operator-( YD y, HD h ) { return YD( y._ - h._ ); }
inline constexpr XD   operator-( WD w, XD x ) { return XD( x._ - w._ ); }
inline constexpr YD   operator-( HD h, YD y ) { return YD( y._ - h._ ); }
inline constexpr void operator+=( XD& x, WD w ) { x._ += w._; }
inline constexpr void operator+=( YD& y, HD h ) { y._ += h._; }
inline constexpr void operator-=( XD& x, WD w ) { x._ -= w._; }
inline constexpr void operator-=( YD& y, HD h ) { y._ -= h._; }
// WDe can allow deltas to subtract from eachother in a mutating
// way. WDe don't just put these in the above generic classes
// because we don't want to allow e.g. x1 -= x2; this is because
// subtracting a coordinate from another one should only yield a
// width, and so that statement would not make sense.
inline constexpr void operator-=( WD& w1, WD w2 ) { w1._ -= w2._; }
inline constexpr void operator-=( HD& h1, HD h2 ) { h1._ -= h2._; }
// These express that when we subtract two coordinates or
// two deltas then we get a delta (not a coordinate).
inline constexpr WD operator-( XD x1, XD x2 ) { return WD( x1._ - x2._ ); }
inline constexpr HD operator-( YD y1, YD y2 ) { return HD( y1._ - y2._ ); }
inline constexpr WD operator-( WD x1, WD x2 ) { return WD( x1._ - x2._ ); }
inline constexpr HD operator-( HD y1, HD y2 ) { return HD( y1._ - y2._ ); }
// These express that an absolute position divided by a distance
// yields a dimensionless ratio.
inline constexpr int operator/( XD x, WD w ) { return x._ / w._; }
inline constexpr int operator/( YD y, HD h ) { return y._ / h._; }
// These express that mod'ing an absolute position by a distance
// or mod'ing two distances yields another distance (i.e., not an
// absolute position).
inline constexpr WD operator%( XD x, WD w ) { return WD( x._ % w._ ); }
inline constexpr HD operator%( YD y, HD h ) { return HD( y._ % h._ ); }
inline constexpr WD operator%( WD w1, WD w2 ) { return WD( w1._ % w2._ ); }
inline constexpr HD operator%( HD h1, HD h2 ) { return HD( h1._ % h2._ ); }
inline constexpr WD operator%( XD x, SXD sx ) { return WD( x._ % sx._ ); }
inline constexpr HD operator%( YD y, SYD sy ) { return HD( y._ % sy._ ); }
inline constexpr WD operator%( WD w, SXD sx ) { return WD( w._ % sx._ ); }
inline constexpr HD operator%( HD h, SYD sy ) { return HD( h._ % sy._ ); }
// These express that one can only multiply or divide a typed int
// by the appropriate scaling type.
inline constexpr XD operator*( XD x, SXD sx ) { return XD( x._ * sx._ ); }
inline constexpr YD operator*( YD y, SYD sy ) { return YD( y._ * sy._ ); }
inline constexpr SXD operator*( SXD s1, SXD s2 ) { return SXD( s1._ * s2._ ); }
inline constexpr SYD operator*( SYD s1, SYD s2 ) { return SYD( s1._ * s2._ ); }
inline constexpr XD operator/( XD x, SXD sx ) { return XD( x._ / sx._ ); }
inline constexpr YD operator/( YD y, SYD sy ) { return YD( y._ / sy._ ); }
inline constexpr WD operator*( WD w, SXD sx ) { return WD( w._ * sx._ ); }
inline constexpr HD operator*( HD h, SYD sy ) { return HD( h._ * sy._ ); }
inline constexpr WD operator/( WD w, SXD sx ) { return WD( w._ / sx._ ); }
inline constexpr HD operator/( HD h, SYD sy ) { return HD( h._ / sy._ ); }
inline constexpr SXD operator/( SXD s1, SXD s2 ) { return SXD( s1._ / s2._ ); }
inline constexpr SYD operator/( SYD s1, SYD s2 ) { return SYD( s1._ / s2._ ); }
inline constexpr void operator*=( XD& x, SXD sx ) { x._ *= sx._; }
inline constexpr void operator*=( YD& y, SYD sy ) { y._ *= sy._; }
inline constexpr void operator/=( XD& x, SXD sx ) { x._ /= sx._; }
inline constexpr void operator/=( YD& y, SYD sy ) { y._ /= sy._; }
inline constexpr void operator*=( WD& w, SXD sx ) { w._ *= sx._; }
inline constexpr void operator*=( HD& h, SYD sy ) { h._ *= sy._; }
inline constexpr void operator/=( WD& w, SXD sx ) { w._ /= sx._; }
inline constexpr void operator/=( HD& h, SYD sy ) { h._ /= sy._; }
// clang-format on
} // namespace rn

namespace std {

// FIXME: move X,Y,W,H out of this header.
DEFINE_HASH_FOR_TYPED_INT( ::rn::X );
DEFINE_HASH_FOR_TYPED_INT( ::rn::Y );
DEFINE_HASH_FOR_TYPED_INT( ::rn::W );
DEFINE_HASH_FOR_TYPED_INT( ::rn::H );

} // namespace std

#define LUA_TYPED_INT_DECL( name )                    \
  base::maybe<name> lua_get( lua::cthread L, int idx, \
                             lua::tag<name> );        \
                                                      \
  void lua_push( lua::cthread L, name const& o );

#define LUA_TYPED_INT_IMPL( name )                    \
  base::maybe<name> lua_get( lua::cthread L, int idx, \
                             lua::tag<name> ) {       \
    base::maybe<int> n = lua::get<int>( L, idx );     \
    if( !n ) return base::nothing;                    \
    return name{ *n };                                \
  }                                                   \
                                                      \
  void lua_push( lua::cthread L, name const& o ) {    \
    lua::push( L, o._ );                              \
  }

#define RCL_TYPED_INT_DECL( name )                        \
  rcl::convert_err<name> convert_to( rcl::value const& v, \
                                     rcl::tag<name> );

#define RCL_TYPED_INT_IMPL( name )                        \
  rcl::convert_err<name> convert_to( rcl::value const& v, \
                                     rcl::tag<name> ) {   \
    base::maybe<int const&> i = v.get_if<int>();          \
    if( !i )                                              \
      return rcl::error( fmt::format(                     \
          "cannot produce a {} from type {}.", #name,     \
          rcl::name_of( rcl::type_of( v ) ) ) );          \
    return name{ *i };                                    \
  }

#define CDR_TYPED_INT_DECL( name )                          \
  cdr::value to_canonical( cdr::converter&, name o,         \
                           cdr::tag_t<name> );              \
                                                            \
  cdr::result<name> from_canonical( cdr::converter&   conv, \
                                    cdr::value const& v,    \
                                    cdr::tag_t<name> );

#define CDR_TYPED_INT_IMPL( name )                              \
  cdr::value to_canonical( cdr::converter&, name o,             \
                           cdr::tag_t<name> ) {                 \
    return cdr::value{ static_cast<cdr::integer_type>( o._ ) }; \
  }                                                             \
                                                                \
  cdr::result<name> from_canonical( cdr::converter&   conv,     \
                                    cdr::value const& v,        \
                                    cdr::tag_t<name> ) {        \
    if( !v.holds<cdr::integer_type>() )                         \
      return conv.err(                                          \
          "failed to convert value of type {} to int.",         \
          cdr::type_name( v ) );                                \
    /* WARNING: this may lose precision */                      \
    return name{                                                \
        static_cast<int>( v.get<cdr::integer_type>() ) };       \
  }

namespace rn {

// FIXME: move X,Y,W,H out of this header.
LUA_TYPED_INT_DECL( ::rn::X );
LUA_TYPED_INT_DECL( ::rn::Y );
LUA_TYPED_INT_DECL( ::rn::W );
LUA_TYPED_INT_DECL( ::rn::H );

RCL_TYPED_INT_DECL( ::rn::X );
RCL_TYPED_INT_DECL( ::rn::Y );
RCL_TYPED_INT_DECL( ::rn::W );
RCL_TYPED_INT_DECL( ::rn::H );

CDR_TYPED_INT_DECL( ::rn::X );
CDR_TYPED_INT_DECL( ::rn::Y );
CDR_TYPED_INT_DECL( ::rn::W );
CDR_TYPED_INT_DECL( ::rn::H );

} // namespace rn
