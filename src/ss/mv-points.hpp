/****************************************************************
**mv-points.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-02.
*
* Description: A type for representing movement points that will
*              ensure correct handling of fractional points.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Cdr
#include "cdr/ext.hpp"

// luapp
#include "luapp/ext.hpp"

namespace rn {

// Cannot convert to int.
class ND MovementPoints {
 private:
  static constexpr int factor = 3;

  // atoms_ can be > 2
  constexpr MovementPoints( int integral, int atoms_arg )
    : atoms_(
          ( ( integral + ( atoms_arg / factor ) ) * factor ) +
          ( atoms_arg % factor ) ) {}

 public:
  constexpr MovementPoints() = default;
  explicit constexpr MovementPoints( int p )
    : atoms_( p * factor ) {}

  MovementPoints& operator=( int p ) {
    atoms_ = p * factor;
    return *this;
  }

  constexpr MovementPoints( MovementPoints const& other ) =
      default;
  constexpr MovementPoints( MovementPoints&& other ) = default;

  MovementPoints& operator=( MovementPoints const& other ) =
      default;
  MovementPoints& operator=( MovementPoints&& other ) = default;

  int atoms() const { return atoms_; }

  static constexpr MovementPoints _1_3() {
    return MovementPoints( 0, 1 );
  };

  static constexpr MovementPoints _2_3() {
    return MovementPoints( 0, 2 );
  };

  constexpr bool operator==( MovementPoints const& rhs ) const {
    return atoms_ == rhs.atoms_;
  }
  constexpr bool operator==( int rhs ) const {
    return atoms_ == rhs * factor;
  }

  constexpr bool operator!=( MovementPoints const& rhs ) const {
    return atoms_ != rhs.atoms_;
  }
  constexpr bool operator!=( int rhs ) const {
    return atoms_ != rhs * factor;
  }

  constexpr bool operator>( MovementPoints const& rhs ) const {
    return atoms_ > rhs.atoms_;
  }
  constexpr bool operator>( int rhs ) const {
    return atoms_ > rhs * factor;
  }

  constexpr bool operator<( MovementPoints const& rhs ) const {
    return atoms_ < rhs.atoms_;
  }
  constexpr bool operator<( int rhs ) const {
    return atoms_ < rhs * factor;
  }

  constexpr bool operator>=( MovementPoints const& rhs ) const {
    return atoms_ >= rhs.atoms_;
  }
  constexpr bool operator>=( int rhs ) const {
    return atoms_ >= rhs * factor;
  }

  constexpr bool operator<=( MovementPoints const& rhs ) const {
    return atoms_ <= rhs.atoms_;
  }
  constexpr bool operator<=( int rhs ) const {
    return atoms_ <= rhs * factor;
  }

  constexpr MovementPoints operator+(
      MovementPoints const& rhs ) const {
    return MovementPoints( 0, atoms_ + rhs.atoms_ );
  }
  constexpr MovementPoints operator+( int rhs ) const {
    return MovementPoints( 0, atoms_ + ( rhs * factor ) );
  }

  constexpr MovementPoints operator-(
      MovementPoints const& rhs ) const {
    return MovementPoints( 0, atoms_ - rhs.atoms_ );
  }
  constexpr MovementPoints operator-( int rhs ) const {
    return MovementPoints( 0, atoms_ - ( rhs * factor ) );
  }

  void operator+=( MovementPoints const& rhs ) {
    atoms_ += rhs.atoms_;
  }
  void operator+=( int rhs ) { atoms_ += rhs * factor; }

  void operator-=( MovementPoints const& rhs ) {
    atoms_ -= rhs.atoms_;
  }
  void operator-=( int rhs ) { atoms_ -= rhs * factor; }

  constexpr MovementPoints operator-() const {
    return MovementPoints( 0, -atoms_ );
  }

  friend void to_str( MovementPoints const& o, std::string& out,
                      base::ADL_t );

  base::valid_or<std::string> validate() const;

  friend maybe<MovementPoints> lua_get(
      lua::cthread L, int idx, lua::tag<MovementPoints> );
  friend void lua_push( lua::cthread L, MovementPoints mv_pts );

  friend cdr::value to_canonical( cdr::converter&       conv,
                                  MovementPoints const& o,
                                  cdr::tag_t<MovementPoints> );

  friend cdr::result<MovementPoints> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<MovementPoints> );

 private:
  // 2 points would be represented by 2*factor.
  int atoms_ = 0;
};
NOTHROW_MOVE( MovementPoints );

using MvPoints = MovementPoints;

} // namespace rn
