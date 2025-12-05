/****************************************************************
**colony-mgr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Main interface for controlling Colonies.
*
*****************************************************************/
#pragma once

// Rds
#include "colony-mgr.rds.hpp"

// Revolution Now
#include "expect.hpp"
#include "wait.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/commodity.rds.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <string_view>

namespace rn {

struct ColoniesState;
struct Colony;
struct ColonyJob;
struct Player;
struct SS;
struct SSConst;
struct TerrainConnectivity;
struct TS;
struct Unit;

enum class e_outdoor_job;
enum class e_player;

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    SSConst const& ss, UnitId founder );

// This will change the player of the colony and all units that
// are workers in the colony as well as units that are in the
// same map square as the colony.
void change_colony_player( SS& ss, TS& ts, Colony& colony,
                           e_player new_player );

// Before calling this, it should already have been the case that
// `can_found_colony` was called to validate; so it should work,
// and thus if it doesn't, it will check-fail.
ColonyId found_colony( SS& ss, TS& ts, Player const& player,
                       UnitId founder, std::string_view name );

// This basically creates a default-constructed colony and gives
// it a player, name, and location, but nothing more. So it is
// not a valid colony yet. Normal game code shouldn't really call
// this, it is mostly exposed for testing.
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_player player, Coord where,
                              std::string_view name );

// Will strip the unit of any commodities (including inventory
// and modifiers) and deposit the commodities into the colony.
// This is used to transform the unit when founding a colony. In
// that situation, the unit needs to be stripped to its base type
// and all of the commodities that it has should be added into
// the colony's commodity store. All modifiers will be stripped
// from the unit as well.
void strip_unit_to_base_type( SS& ss, TS& ts, Unit& unit,
                              Colony& colony );

void change_unit_outdoor_job( Colony& colony, UnitId id,
                              e_outdoor_job new_job );

int colony_population( Colony const& colony );

// Total populate defined as number of citizens working in
// colonies. Does not include ground units.
int total_colonies_population( SSConst const& ss,
                               e_player player );

// All indoor+outdoor units. Does not include units at the colony
// gate. Will be sorted by ID.
std::vector<UnitId> colony_workers( Colony const& colony );

bool colony_has_unit( Colony const& colony, UnitId id );

// Destroy's a colony without any interactivity. Normal game code
// should use the interactive version below.
ColonyDestructionOutcome destroy_colony( SS& ss, TS& ts,
                                         Colony& colony );

// This will do the colony destruction depixelation animation and
// then will actually destroy the colony using the destroy_colony
// method documented above. An optional message will be shown to
// the user first, before any others.
wait<> run_animated_colony_destruction(
    SS& ss, TS& ts, Colony& colony, e_ship_damaged_reason reason,
    maybe<std::string> msg );

// Same as above but for when the animation is done separately.
wait<> run_colony_destruction( SS& ss, TS& ts, Colony& colony,
                               e_ship_damaged_reason reason,
                               maybe<std::string> msg );

// Given a colony, find the squares in its surroundings that are
// being worked by units in other colonies, either friendly or
// foreign. In the original game, these squares will be drawn
// with red boxes around them in the colony view to signal that
// no further colonists can work there.
refl::enum_map<e_direction, bool>
find_occupied_surrounding_colony_squares( SSConst const& ss,
                                          Colony const& colony );

// If the colony doesn't already have a stockade and the player
// has Sieur de La Salle and its population >= 3 then it will be
// given a stockade.
void give_stockade_if_needed( Player const& player,
                              Colony& colony );

// This is a low-level method that shouldn't be used directly;
// instead, one should use the unit-ownership module API.
void add_unit_to_colony_obj_low_level( SS& ss, TS& ts,
                                       Colony& colony,
                                       Unit& unit,
                                       ColonyJob const& job );

// This is a low-level method that shouldn't be used directly;
// instead, one should use the unit-ownership module API.
void remove_unit_from_colony_obj_low_level( SS& ss,
                                            Colony& colony,
                                            UnitId unit_id );

// These are colonies with sea lane access on either the left or
// right edge of the map.
std::vector<ColonyId> find_coastal_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player player );

// Finds the player's colonies that have the same connectivity
// index as the given tile.
std::vector<ColonyId> find_connected_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player player, gfx::point tile );

// Sorts the commodities by their total value (defined as pre-tax
// sale rate multiplied by quantity). The more valuable ones will
// be first. If two commodities have an equal value it will
// prefer those with larger indexes. Note that this vector of
// pair type is the one that we get from the unit cargo API.
void sort_slotted_commodities_by_value(
    SSConst const& ss, Player const& player,
    std::vector<std::pair<Commodity, int /*slot*/>>& comms );

// Same as above but on the commodities in the colony store.
[[nodiscard]] std::vector<Commodity> colony_commodities_by_value(
    SSConst const& ss, Player const& player,
    Colony const& colony );

// Same as above but can limit to a desired set.
[[nodiscard]] std::vector<Commodity>
colony_commodities_by_value_restricted(
    SSConst const& ss, Player const& player,
    Colony const& colony,
    std::vector<e_commodity> const& desired );

// This is called when the player presses 'l' (load) in the
// colony view. It will auto-select a commodity from the store
// and load it into the cargo of the given unit assuming it fits.
// Returns if something was loaded.
maybe<Commodity> colony_auto_load_commodity(
    SSConst const& ss, Player const& player, Unit& unit,
    Colony& colony );

// This is called when the player presses 'u' (unload) in the
// colony view. It will auto-select a commodity from the cargo
// and unload it into the store. Returns if something was un-
// loaded.
maybe<Commodity> colony_auto_unload_commodity(
    SSConst const& ss, Player const& player, Unit& unit,
    Colony& colony );

} // namespace rn
