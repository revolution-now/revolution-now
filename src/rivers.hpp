/****************************************************************
**rivers.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-03-14.
*
* Description: Handles river generation.
*
*****************************************************************/
#pragma once

// rds
#include "rivers.rds.hpp"

// config
#include "config/range-helpers.rds.hpp"

// base
#include "base/function-ref.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct MapMatrix;
struct RiverParameters;

namespace config::map_gen {
struct RiverLandFormParameters;
struct RiverClimateParameters;
}

/****************************************************************
** RiverParameterInterpolator.
*****************************************************************/
struct RiverParameterInterpolator {
  RiverParameterInterpolator( double scale, int climate )
    : scale_( scale ), climate_( climate ) {}

  [[nodiscard]] double for_land_form(
      auto const p_field ) const {
    return land_form_interp_impl(
        [&]( auto const& val ) { return val.*p_field; } );
  }

  [[nodiscard]] double for_climate( auto const p_field ) const {
    return climate_interp_impl(
        [&]( auto const& val ) { return val.*p_field; } );
  }

  [[nodiscard]] config::Probability for_land_form_probability(
      auto const p_field ) const {
    return config::Probability{
      .probability =
          land_form_interp_impl( [&]( auto const& val ) {
            return ( val.*p_field ).probability;
          } ) };
  }

  [[nodiscard]] config::Probability for_climate_probability(
      auto const p_field ) const {
    return config::Probability{
      .probability =
          climate_interp_impl( [&]( auto const& val ) {
            return ( val.*p_field ).probability;
          } ) };
  }

 private:
  using LandFormGetter = base::function_ref<double(
      config::map_gen::RiverLandFormParameters const& )>;
  using ClimateGetter  = base::function_ref<double(
      config::map_gen::RiverClimateParameters const& )>;

  double land_form_interp_impl( LandFormGetter fn ) const;
  double climate_interp_impl( ClimateGetter fn ) const;

  double const scale_;
  int const climate_;
};

/****************************************************************
** Public API.
*****************************************************************/
void add_rivers( MapMatrix& m, IRand& rand,
                 RiverParameters const& params );

[[nodiscard]] int count_rivers( MapMatrix const& m );

} // namespace rn
