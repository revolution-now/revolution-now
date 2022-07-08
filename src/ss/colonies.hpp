/****************************************************************
**gs-colonies.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Colony-related save-game state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "ss/colonies.rds.hpp"

namespace rn {

struct ColoniesState {
  ColoniesState();
  bool operator==( ColoniesState const& ) const = default;

  // Implement refl::WrapsReflected.
  ColoniesState( wrapped::ColoniesState&& o );
  wrapped::ColoniesState const&     refl() const { return o_; }
  static constexpr std::string_view refl_ns   = "rn";
  static constexpr std::string_view refl_name = "ColoniesState";

  // API. Here we mainly just have the functions that are re-
  // quired to access the state in this class and maintain in-
  // variants. Any more complicated game logic that gets layered
  // on top of these should go elsewhere.

  ColonyId last_colony_id() const;

  std::unordered_map<ColonyId, Colony> const& all() const;

  std::vector<ColonyId> for_nation( e_nation nation ) const;

  Colony const& colony_for( ColonyId id ) const;
  Colony&       colony_for( ColonyId id );

  Coord coord_for( ColonyId id ) const;

  base::maybe<ColonyId> maybe_from_coord( Coord const& c ) const;
  ColonyId              from_coord( Coord const& c ) const;

  base::maybe<ColonyId> maybe_from_name(
      std::string_view name ) const;

  // The id of this colony must be zero (i.e., you can't select
  // the ID); a new ID will be generated for this unit and re-
  // turned.
  ColonyId add_colony( Colony&& colony );

  // DO NOT call this directly as it will not properly remove
  // units or check for errors. Should not be holding any refer-
  // ences to the colony after this.
  void destroy_colony( ColonyId id );

 private:
  [[nodiscard]] ColonyId next_colony_id();

  base::valid_or<std::string> validate() const;
  void                        validate_or_die() const;

  // ----- Serializable state.
  wrapped::ColoniesState o_;

  // ----- Non-serializable (transient) state.
  std::unordered_map<Coord, ColonyId>       colony_from_coord_;
  std::unordered_map<std::string, ColonyId> colony_from_name_;
};

} // namespace rn
