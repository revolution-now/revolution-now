/****************************************************************
* mv-points.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-02.
*
* Description: A type for representing movement points that will
*              ensure correct handling of atomsal points.
*
*****************************************************************/
#pragma once

namespace rn {

// Cannot convert to int.
class MovementPoints {

public:
  MovementPoints() : points_atoms( 0 ) {}
  MovementPoints( int p ) : points_atoms( p*factor ) {}

  static MovementPoints _1_3() { return MovementPoints( 0, 1 ); };

  static MovementPoints _2_3() { return MovementPoints( 0, 2 ); };

  bool operator==( MovementPoints const& rhs ) const {
    return points_atoms == rhs.points_atoms; }
  bool operator==( int rhs ) const {
    return points_atoms == rhs*factor; }

  bool operator!=( MovementPoints const& rhs ) const {
    return points_atoms != rhs.points_atoms; }
  bool operator!=( int rhs ) const {
    return points_atoms != rhs*factor; }

  bool operator>( MovementPoints const& rhs ) const {
    return points_atoms > rhs.points_atoms; }
  bool operator>( int rhs ) const {
    return points_atoms > rhs*factor; }

  bool operator<( MovementPoints const& rhs ) const {
    return points_atoms < rhs.points_atoms; }
  bool operator<( int rhs ) const {
    return points_atoms < rhs*factor; }

  bool operator>=( MovementPoints const& rhs ) const {
    return points_atoms >= rhs.points_atoms; }
  bool operator>=( int rhs ) const {
    return points_atoms >= rhs*factor; }

  bool operator<=( MovementPoints const& rhs ) const {
    return points_atoms <= rhs.points_atoms; }
  bool operator<=( int rhs ) const {
    return points_atoms <= rhs*factor; }

  MovementPoints operator+( MovementPoints const& rhs ) const {
    return MovementPoints( 0, points_atoms+rhs.points_atoms ); }
  MovementPoints operator+( int rhs ) const {
    return MovementPoints( 0, points_atoms+(rhs*factor) ); }

  MovementPoints operator-( MovementPoints const& rhs ) const {
    return MovementPoints( 0, points_atoms-rhs.points_atoms ); }
  MovementPoints operator-( int rhs ) const {
    return MovementPoints( 0, points_atoms-(rhs*factor) ); }

  void operator+=( MovementPoints const& rhs ) {
    points_atoms += rhs.points_atoms; }
  void operator+=( int rhs ) {
    points_atoms += rhs*factor; }

  void operator-=( MovementPoints const& rhs ) {
    points_atoms -= rhs.points_atoms; }
  void operator-=( int rhs ) {
    points_atoms -= rhs*factor; }

private:
  // atoms can be > 2
  MovementPoints( int integral, int atoms );
  static constexpr int factor = 3;
  int points_atoms; // 2 points would be represented by 2*factor.
};

} // namespace rn

