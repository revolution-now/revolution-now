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

// Rcl
#include "rcl/ext.hpp"

// Cdr
#include "cdr/ext.hpp"

// luapp
#include "luapp/ext.hpp"

namespace rn {

// Cannot convert to int.
class ND MovementPoints {
 public:
  MovementPoints() = default;
  explicit MovementPoints( int p )
    : points_atoms( p * factor ) {}

  MovementPoints& operator=( int p ) {
    points_atoms = p * factor;
    return *this;
  }

  MovementPoints( MovementPoints const& other ) = default;
  MovementPoints( MovementPoints&& other )      = default;

  MovementPoints& operator=( MovementPoints const& other ) =
      default;
  MovementPoints& operator=( MovementPoints&& other ) = default;

  static MovementPoints _1_3() {
    return MovementPoints( 0, 1 );
  };

  static MovementPoints _2_3() {
    return MovementPoints( 0, 2 );
  };

  bool operator==( MovementPoints const& rhs ) const {
    return points_atoms == rhs.points_atoms;
  }
  bool operator==( int rhs ) const {
    return points_atoms == rhs * factor;
  }

  bool operator!=( MovementPoints const& rhs ) const {
    return points_atoms != rhs.points_atoms;
  }
  bool operator!=( int rhs ) const {
    return points_atoms != rhs * factor;
  }

  bool operator>( MovementPoints const& rhs ) const {
    return points_atoms > rhs.points_atoms;
  }
  bool operator>( int rhs ) const {
    return points_atoms > rhs * factor;
  }

  bool operator<( MovementPoints const& rhs ) const {
    return points_atoms < rhs.points_atoms;
  }
  bool operator<( int rhs ) const {
    return points_atoms < rhs * factor;
  }

  bool operator>=( MovementPoints const& rhs ) const {
    return points_atoms >= rhs.points_atoms;
  }
  bool operator>=( int rhs ) const {
    return points_atoms >= rhs * factor;
  }

  bool operator<=( MovementPoints const& rhs ) const {
    return points_atoms <= rhs.points_atoms;
  }
  bool operator<=( int rhs ) const {
    return points_atoms <= rhs * factor;
  }

  MovementPoints operator+( MovementPoints const& rhs ) const {
    return MovementPoints( 0, points_atoms + rhs.points_atoms );
  }
  MovementPoints operator+( int rhs ) const {
    return MovementPoints( 0, points_atoms + ( rhs * factor ) );
  }

  MovementPoints operator-( MovementPoints const& rhs ) const {
    return MovementPoints( 0, points_atoms - rhs.points_atoms );
  }
  MovementPoints operator-( int rhs ) const {
    return MovementPoints( 0, points_atoms - ( rhs * factor ) );
  }

  void operator+=( MovementPoints const& rhs ) {
    points_atoms += rhs.points_atoms;
  }
  void operator+=( int rhs ) { points_atoms += rhs * factor; }

  void operator-=( MovementPoints const& rhs ) {
    points_atoms -= rhs.points_atoms;
  }
  void operator-=( int rhs ) { points_atoms -= rhs * factor; }

  friend void to_str( MovementPoints const& o, std::string& out,
                      base::ADL_t );

  base::valid_or<std::string> validate() const;

  friend maybe<MovementPoints> lua_get(
      lua::cthread L, int idx, lua::tag<MovementPoints> );
  friend void lua_push( lua::cthread L, MovementPoints mv_pts );

  // This is for deserializing from Rcl config files.
  friend rcl::convert_err<MovementPoints> convert_to(
      rcl::value const& v, rcl::tag<MovementPoints> );

  friend cdr::value to_canonical( cdr::converter&       conv,
                                  MovementPoints const& o,
                                  cdr::tag_t<MovementPoints> );

  friend cdr::result<MovementPoints> from_canonical(
      cdr::converter& conv, cdr::value const& v,
      cdr::tag_t<MovementPoints> );

 private:
  // atoms can be > 2
  MovementPoints( int integral, int atoms );
  static constexpr int factor = 3;

  // 2 points would be represented by 2*factor.
  int points_atoms;
};
NOTHROW_MOVE( MovementPoints );

using MvPoints = MovementPoints;

} // namespace rn
