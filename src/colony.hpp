/****************************************************************
**colony.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-15.
*
* Description: Data structure representing a Colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "adt.hpp"
#include "colony-mfg.hpp"
#include "commodity.hpp"
#include "enum.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "nation.hpp"

// Flatbuffers
#include "fb/colony_generated.h"

namespace rn {

adt_s_rn( ColonyJob,                 //
          ( land,                    //
            ( e_direction, d ) ),    //
          ( mfg,                     //
            ( e_mfg_job, mfg_job ) ) //
);

class Colony {
public:
  Colony()  = default; // for serialization framework.
  ~Colony() = default;
  Colony( Colony const& ) = delete;
  Colony( Colony&& )      = default;

  Colony& operator=( Colony const& ) = delete;
  Colony& operator=( Colony&& ) = default;

  /************************* Getters ***************************/

  ColonyId           id() const { return id_; }
  e_nation           nation() const { return nation_; }
  std::string const& name() const { return name_; }
  Coord              location() const { return location_; }
  int                sentiment() const { return sentiment_; }
  int prod_hammers() const { return prod_hammers_; }
  int prod_tools() const { return prod_tools_; }
  FlatSet<e_colony_building> const& buildings() const {
    return buildings_;
  }

  /************************ Modifiers **************************/
  // NOTE: these modifiers do not enforce invariants!
  void add_building( e_colony_building building );
  void add_unit( UnitId id, ColonyJob_t const& job );
  void remove_unit( UnitId id );

  /************************ Functions **************************/
  // NOTE: these modifiers do not enforce invariants!
  int  population() const;
  bool has_unit( UnitId id ) const;

  std::string to_string() const;

  // This class itself is not equipped to check all of its own
  // invariants, since many of them require other game state to
  // validate.
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

private:
  friend expect<ColonyId> cstate_create_colony(
      e_nation nation, Coord const& where,
      std::string_view name );

  using FlatMap_e_commodity_int = FlatMap<e_commodity, int>;
  using FlatMap_UnitId_ColonyJob_t =
      FlatMap<UnitId, ColonyJob_t>;
  using FlatSet_e_colony_building = FlatSet<e_colony_building>;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Colony,
  // Basic info.
  ( ColonyId,                   id_           ),
  ( e_nation,                   nation_       ),
  ( std::string,                name_         ),
  ( Coord,                      location_     ),

  // Commodities.
  ( FlatMap_e_commodity_int,    commodities_  ),

  // Serves to both record the units in this colony as well as
  // their occupations.
  ( FlatMap_UnitId_ColonyJob_t, units_        ),
  ( FlatSet_e_colony_building,  buildings_    ),

  // Production
  ( Opt<e_colony_building>,     production_   ),
  ( int,                        prod_hammers_ ),
  ( int,                        prod_tools_   ),

  // Liberty sentiment: [0,100].
  ( int,                        sentiment_    ));
  // clang-format on
};
NOTHROW_MOVE( Colony );

} // namespace rn

DEFINE_FORMAT( ::rn::Colony, "{}", o.to_string() );
