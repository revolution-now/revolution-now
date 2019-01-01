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

#include "base-util/string.hpp"

#include <iostream>
#include <optional>
#include <vector>

// This is a minimal wrapper around an T. It allows nothing ex-
// cept for (explicit) construction from T, copying/assignment,
// equality, and (explicit) conversion back to T.
//
// One use case for this is to wrap an T which will not ever be
// subject to any arithmetic operations, such as IDs.
template<typename T, typename Tag>
struct TypedNumMinimal {
  constexpr TypedNumMinimal() = default;
  ~TypedNumMinimal()          = default;
  constexpr explicit TypedNumMinimal( T n_ ) : _( n_ ) {}
  constexpr TypedNumMinimal(
      TypedNumMinimal<T, Tag> const& other )
    : _( other._ ) {}
  constexpr TypedNumMinimal(
      TypedNumMinimal<T, Tag>&& other ) noexcept
    : _( other._ ) {}
  TypedNumMinimal<T, Tag>& operator=(
      TypedNumMinimal<T, Tag> const& other ) {
    _ = other._;
    return *this;
  }
  TypedNumMinimal<T, Tag>& operator=(
      TypedNumMinimal<T, Tag>&& other ) noexcept {
    _ = other._;
    return *this;
  }
  TypedNumMinimal<T, Tag>& operator=( T n ) {
    _ = n;
    return *this;
  }
  bool operator==( TypedNumMinimal<T, Tag> other ) const {
    return _ == other._;
  }
  bool operator!=( TypedNumMinimal<T, Tag> other ) const {
    return _ != other._;
  }
  explicit operator T() const { return _; }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H                              h,
                          TypedNumMinimal<T, Tag> const& c ) {
    return H::combine( std::move( h ), c._ );
  }

  T _{0};
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
  bool operator==( TypedInt<Tag> other ) const {
    return P::_ == other._;
  }
  bool operator!=( TypedInt<Tag> other ) const {
    return P::_ != other._;
  }
  TypedInt<Tag> operator-() { return TypedInt<Tag>( -P::_ ); }
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
  bool operator==( TypedDouble<Tag> other ) const {
    return P::_ == other._;
  }
  bool operator!=( TypedDouble<Tag> other ) const {
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
inline TypedDouble<Tag> operator*( TypedDouble<Tag> left,
                                   TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left._ * right._ );
}

template<typename Tag>
inline TypedDouble<Tag> operator/( TypedDouble<Tag> left,
                                   double           right ) {
  return TypedDouble<Tag>( left._ / right );
}
/* dividing like types yields TypedDouble<Tag> dimensionless
 * ratio.
 */
template<typename Tag>
inline double operator/( TypedDouble<Tag> a1,
                         TypedDouble<Tag> a2 ) {
  return a1._ / a2._;
}

template<typename Tag>
inline TypedDouble<Tag> operator+( TypedDouble<Tag> left,
                                   double           right ) {
  return TypedDouble<Tag>( left._ + right );
}

template<typename Tag>
inline TypedDouble<Tag> operator+( double           left,
                                   TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( right._ + left );
}

template<typename Tag>
inline TypedDouble<Tag> operator+( TypedDouble<Tag> left,
                                   TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left._ + right._ );
}

template<typename Tag>
inline TypedDouble<Tag> operator-( TypedDouble<Tag> left,
                                   double           right ) {
  return TypedDouble<Tag>( left._ - right );
}

template<typename Tag>
inline TypedDouble<Tag> operator-( double           left,
                                   TypedDouble<Tag> right ) {
  return TypedDouble<Tag>( left - right._ );
}

template<typename Tag>
inline bool operator<( TypedDouble<Tag> left, double right ) {
  return left._ < right;
}

template<typename Tag>
inline bool operator<( double left, TypedDouble<Tag> right ) {
  return left < right._;
}

template<typename Tag>
inline bool operator<( TypedDouble<Tag> left,
                       TypedDouble<Tag> right ) {
  return left._ < right._;
}

template<typename Tag>
inline bool operator>( TypedDouble<Tag> left, double right ) {
  return left._ > right;
}

template<typename Tag>
inline bool operator>( double left, TypedDouble<Tag> right ) {
  return left > right._;
}

template<typename Tag>
inline bool operator>( TypedDouble<Tag> left,
                       TypedDouble<Tag> right ) {
  return left._ > right._;
}

template<typename Tag>
inline bool operator<=( TypedDouble<Tag> left, double right ) {
  return left._ <= right;
}

template<typename Tag>
inline bool operator<=( double left, TypedDouble<Tag> right ) {
  return left <= right._;
}

template<typename Tag>
inline bool operator<=( TypedDouble<Tag> left,
                        TypedDouble<Tag> right ) {
  return left._ <= right._;
}

template<typename Tag>
inline bool operator>=( TypedDouble<Tag> left, double right ) {
  return left._ >= right;
}

template<typename Tag>
inline bool operator>=( double left, TypedDouble<Tag> right ) {
  return left >= right._;
}

template<typename Tag>
inline bool operator>=( TypedDouble<Tag> left,
                        TypedDouble<Tag> right ) {
  return left._ >= right._;
}

template<typename Tag>
inline std::ostream& operator<<( std::ostream&    out,
                                 TypedDouble<Tag> what ) {
  return ( out << what._ );
}

template<typename Tag>
inline std::istream& operator>>( std::istream&     in,
                                 TypedDouble<Tag>& what ) {
  return ( in >> what._ );
}

/** TypedInt ***************************************************/
template<typename Tag>
inline TypedInt<Tag> operator*( TypedInt<Tag> left,
                                TypedInt<Tag> right ) {
  return TypedInt<Tag>( left._ * right._ );
}

template<typename Tag>
inline TypedInt<Tag> operator/( TypedInt<Tag> left, int right ) {
  return TypedInt<Tag>( left._ / right );
}
/* dividing like types yields TypedInt<Tag> dimensionless ratio.
 */
template<typename Tag>
inline int operator/( TypedInt<Tag> a1, TypedInt<Tag> a2 ) {
  return a1._ / a2._;
}

template<typename Tag>
inline TypedInt<Tag> operator+( TypedInt<Tag> left, int right ) {
  return TypedInt<Tag>( left._ + right );
}

template<typename Tag>
inline TypedInt<Tag> operator+( int left, TypedInt<Tag> right ) {
  return TypedInt<Tag>( right._ + left );
}

template<typename Tag>
inline TypedInt<Tag> operator+( TypedInt<Tag> left,
                                TypedInt<Tag> right ) {
  return TypedInt<Tag>( left._ + right._ );
}

template<typename Tag>
inline TypedInt<Tag> operator-( TypedInt<Tag> left, int right ) {
  return TypedInt<Tag>( left._ - right );
}

template<typename Tag>
inline TypedInt<Tag> operator-( int left, TypedInt<Tag> right ) {
  return TypedInt<Tag>( left - right._ );
}

template<typename Tag>
inline bool operator<( TypedInt<Tag> left, int right ) {
  return left._ < right;
}

template<typename Tag>
inline bool operator<( int left, TypedInt<Tag> right ) {
  return left < right._;
}

template<typename Tag>
inline bool operator<( TypedInt<Tag> left,
                       TypedInt<Tag> right ) {
  return left._ < right._;
}

template<typename Tag>
inline bool operator>( TypedInt<Tag> left, int right ) {
  return left._ > right;
}

template<typename Tag>
inline bool operator>( int left, TypedInt<Tag> right ) {
  return left > right._;
}

template<typename Tag>
inline bool operator>( TypedInt<Tag> left,
                       TypedInt<Tag> right ) {
  return left._ > right._;
}

template<typename Tag>
inline bool operator<=( TypedInt<Tag> left, int right ) {
  return left._ <= right;
}

template<typename Tag>
inline bool operator<=( int left, TypedInt<Tag> right ) {
  return left <= right._;
}

template<typename Tag>
inline bool operator<=( TypedInt<Tag> left,
                        TypedInt<Tag> right ) {
  return left._ <= right._;
}

template<typename Tag>
inline bool operator>=( TypedInt<Tag> left, int right ) {
  return left._ >= right;
}

template<typename Tag>
inline bool operator>=( int left, TypedInt<Tag> right ) {
  return left >= right._;
}

template<typename Tag>
inline bool operator>=( TypedInt<Tag> left,
                        TypedInt<Tag> right ) {
  return left._ >= right._;
}

template<typename Tag>
inline std::ostream& operator<<( std::ostream& out,
                                 TypedInt<Tag> what ) {
  return ( out << what._ );
}

template<typename Tag>
inline std::istream& operator>>( std::istream&  in,
                                 TypedInt<Tag>& what ) {
  return ( in >> what._ );
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
#define DERIVE_TYPED_NUM( t, a, b, suffix )               \
  namespace rn {                                          \
  struct a : public b<a> {                                \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */      \
    using P = b<a>; /* parent */                          \
    constexpr a() : P( 0 ) {}                             \
    constexpr a( a const& rhs ) = default;                \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */      \
    constexpr a( a&& rhs ) = default;                     \
    ~a()                   = default;                     \
    explicit constexpr a( t n_ ) : P( n_ ) {}             \
    constexpr a( P const& ti ) : P( ti ) {}               \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */      \
    a& operator=( a const& other ) {                      \
      _ = other._;                                        \
      return *this;                                       \
    }                                                     \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */      \
    a& operator=( a&& other ) noexcept {                  \
      _ = other._;                                        \
      return *this;                                       \
    }                                                     \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */      \
    a& operator=( t n ) {                                 \
      _ = n;                                              \
      return *this;                                       \
    }                                                     \
  };                                                      \
  using Opt##a = std::optional<a>;                        \
  using a##Vec = std::vector<a>;                          \
  }                                                       \
  namespace util {                                        \
  template<>                                              \
  inline std::string to_string<rn::a>( rn::a const& n ) { \
    return util::to_string( n._ ) + "_" #suffix;          \
  }                                                       \
  }                                                       \
  inline std::ostream& operator<<( std::ostream& out,     \
                                   rn::a const&  n ) {     \
    return ( out << n._ << "_" #suffix );                 \
  }

// Typed nums that are to represent coordinates should use this
// macro. It will ensure that they have types that allow the nec-
// essary operations and no more.
#define TYPED_COORD( t, a, s ) \
  DERIVE_TYPED_NUM( t, a, TypedInt, s )

// Typed nums that are to represent IDs should use this macro. It
// will create a type which is int-like except that one cannot
// perform any arithmetic operations on it, since those would not
// make sense for an ID.
#define TYPED_ID( a ) \
  DERIVE_TYPED_NUM( int, a, TypedIntMinimal, id )

// Scales are numbers that can only be multiplied by themselves
// or by the corresponding coordinate/length type.
#define TYPED_SCALE( t, a, s ) \
  DERIVE_TYPED_NUM( t, a, TypedIntMinimal, s )

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

// These express that when we add a delta to a coordinate that we
// get another coordinate. They are specific to the semantics of
// cartesian coordinates so we don't include them in the TYPE-
// D_INT macro.
inline X    operator+( X x, W w ) { return X( x._ + w._ ); }
inline Y    operator+( Y y, H h ) { return Y( y._ + h._ ); }
inline X    operator+( W w, X x ) { return X( x._ + w._ ); }
inline Y    operator+( H h, Y y ) { return Y( y._ + h._ ); }
inline X    operator-( X x, W w ) { return X( x._ - w._ ); }
inline Y    operator-( Y y, H h ) { return Y( y._ - h._ ); }
inline X    operator-( W w, X x ) { return X( x._ - w._ ); }
inline Y    operator-( H h, Y y ) { return Y( y._ - h._ ); }
inline void operator+=( X& x, W w ) { x._ += w._; }
inline void operator+=( Y& y, H h ) { y._ += h._; }
inline void operator-=( X& x, W w ) { x._ -= w._; }
inline void operator-=( Y& y, H h ) { y._ -= h._; }
// We can allow deltas to subtract from eachother in a mutating
// way. We don't just put these in the above generic classes
// because we don't want to allow e.g. x1 -= x2; this is because
// subtracting a coordinate from another one should only yield a
// width, and so that statement would not make sense.
inline void operator-=( W& w1, W w2 ) { w1._ -= w2._; }
inline void operator-=( H& h1, H h2 ) { h1._ -= h2._; }
// These express that when we subtract two coordinates or
// two deltas then we get a delta (not a coordinate).
inline W operator-( X x1, X x2 ) { return W( x1._ - x2._ ); }
inline H operator-( Y y1, Y y2 ) { return H( y1._ - y2._ ); }
inline W operator-( W x1, W x2 ) { return W( x1._ - x2._ ); }
inline H operator-( H y1, H y2 ) { return H( y1._ - y2._ ); }
// These express that an absolute position divided by a distance
// yields a dimensionless ratio.
inline int operator/( X x, W w ) { return x._ / w._; }
inline int operator/( Y y, H h ) { return y._ / h._; }
// These express that mod'ing an absolute position by a distance
// or mod'ing two distances yields another distance (i.e., not an
// absolute position).
inline W operator%( X x, W w ) { return W( x._ % w._ ); }
inline H operator%( Y y, H h ) { return H( y._ % h._ ); }
inline W operator%( W w1, W w2 ) { return W( w1._ % w2._ ); }
inline H operator%( H h1, H h2 ) { return H( h1._ % h2._ ); }
inline W operator%( X x, SX sx ) { return W( x._ % sx._ ); }
inline H operator%( Y y, SY sy ) { return H( y._ % sy._ ); }
inline W operator%( W w, SX sx ) { return W( w._ % sx._ ); }
inline H operator%( H h, SY sy ) { return H( h._ % sy._ ); }
// These express that one can only multiply or divide a typed int
// by the appropriate scaling type.
inline X    operator*( X x, SX sx ) { return X( x._ * sx._ ); }
inline Y    operator*( Y y, SY sy ) { return Y( y._ * sy._ ); }
inline X    operator/( X x, SX sx ) { return X( x._ / sx._ ); }
inline Y    operator/( Y y, SY sy ) { return Y( y._ / sy._ ); }
inline W    operator*( W w, SX sx ) { return W( w._ * sx._ ); }
inline H    operator*( H h, SY sy ) { return H( h._ * sy._ ); }
inline W    operator/( W w, SX sx ) { return W( w._ / sx._ ); }
inline H    operator/( H h, SY sy ) { return H( h._ / sy._ ); }
inline void operator*=( X& x, SX sx ) { x._ *= sx._; }
inline void operator*=( Y& y, SY sy ) { y._ *= sy._; }
inline void operator/=( X& x, SX sx ) { x._ /= sx._; }
inline void operator/=( Y& y, SY sy ) { y._ /= sy._; }
inline void operator*=( W& w, SX sx ) { w._ *= sx._; }
inline void operator*=( H& h, SY sy ) { h._ *= sy._; }
inline void operator/=( W& w, SX sx ) { w._ /= sx._; }
inline void operator/=( H& h, SY sy ) { h._ /= sy._; }

// These express that when we add a delta to a coordinate that we
// get another coordinate. They are specific to the semantics of
// cartesian coordinates so we don't include them in the TYDPE-
// D_INT macro.
inline XD   operator+( XD x, WD w ) { return XD( x._ + w._ ); }
inline YD   operator+( YD y, HD h ) { return YD( y._ + h._ ); }
inline XD   operator+( WD w, XD x ) { return XD( x._ + w._ ); }
inline YD   operator+( HD h, YD y ) { return YD( y._ + h._ ); }
inline XD   operator-( XD x, WD w ) { return XD( x._ - w._ ); }
inline YD   operator-( YD y, HD h ) { return YD( y._ - h._ ); }
inline XD   operator-( WD w, XD x ) { return XD( x._ - w._ ); }
inline YD   operator-( HD h, YD y ) { return YD( y._ - h._ ); }
inline void operator+=( XD& x, WD w ) { x._ += w._; }
inline void operator+=( YD& y, HD h ) { y._ += h._; }
inline void operator-=( XD& x, WD w ) { x._ -= w._; }
inline void operator-=( YD& y, HD h ) { y._ -= h._; }
// WDe can allow deltas to subtract from eachother in a mutating
// way. WDe don't just put these in the above generic classes
// because we don't want to allow e.g. x1 -= x2; this is because
// subtracting a coordinate from another one should only yield a
// width, and so that statement would not make sense.
inline void operator-=( WD& w1, WD w2 ) { w1._ -= w2._; }
inline void operator-=( HD& h1, HD h2 ) { h1._ -= h2._; }
// These express that when we subtract two coordinates or
// two deltas then we get a delta (not a coordinate).
inline WD operator-( XD x1, XD x2 ) { return WD( x1._ - x2._ ); }
inline HD operator-( YD y1, YD y2 ) { return HD( y1._ - y2._ ); }
inline WD operator-( WD x1, WD x2 ) { return WD( x1._ - x2._ ); }
inline HD operator-( HD y1, HD y2 ) { return HD( y1._ - y2._ ); }
// These express that an absolute position divided by a distance
// yields a dimensionless ratio.
inline int operator/( XD x, WD w ) { return x._ / w._; }
inline int operator/( YD y, HD h ) { return y._ / h._; }
// These express that mod'ing an absolute position by a distance
// or mod'ing two distances yields another distance (i.e., not an
// absolute position).
inline WD operator%( XD x, WD w ) { return WD( x._ % w._ ); }
inline HD operator%( YD y, HD h ) { return HD( y._ % h._ ); }
inline WD operator%( WD w1, WD w2 ) { return WD( w1._ % w2._ ); }
inline HD operator%( HD h1, HD h2 ) { return HD( h1._ % h2._ ); }
inline WD operator%( XD x, SXD sx ) { return WD( x._ % sx._ ); }
inline HD operator%( YD y, SYD sy ) { return HD( y._ % sy._ ); }
inline WD operator%( WD w, SXD sx ) { return WD( w._ % sx._ ); }
inline HD operator%( HD h, SYD sy ) { return HD( h._ % sy._ ); }
// These express that one can only multiply or divide a typed int
// by the appropriate scaling type.
inline XD operator*( XD x, SXD sx ) { return XD( x._ * sx._ ); }
inline YD operator*( YD y, SYD sy ) { return YD( y._ * sy._ ); }
inline XD operator/( XD x, SXD sx ) { return XD( x._ / sx._ ); }
inline YD operator/( YD y, SYD sy ) { return YD( y._ / sy._ ); }
inline WD operator*( WD w, SXD sx ) { return WD( w._ * sx._ ); }
inline HD operator*( HD h, SYD sy ) { return HD( h._ * sy._ ); }
inline WD operator/( WD w, SXD sx ) { return WD( w._ / sx._ ); }
inline HD operator/( HD h, SYD sy ) { return HD( h._ / sy._ ); }
inline void operator*=( XD& x, SXD sx ) { x._ *= sx._; }
inline void operator*=( YD& y, SYD sy ) { y._ *= sy._; }
inline void operator/=( XD& x, SXD sx ) { x._ /= sx._; }
inline void operator/=( YD& y, SYD sy ) { y._ /= sy._; }
inline void operator*=( WD& w, SXD sx ) { w._ *= sx._; }
inline void operator*=( HD& h, SYD sy ) { h._ *= sy._; }
inline void operator/=( WD& w, SXD sx ) { w._ /= sx._; }
inline void operator/=( HD& h, SYD sy ) { h._ /= sy._; }
} // namespace rn
