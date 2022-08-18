/****************************************************************
**orders-move.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders wherein a unit is asked to move
*              onto an adjacent square.
*
*****************************************************************/
#include "orders-move.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "conductor.hpp"
#include "fight.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "mv-calc.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "ustate.hpp"

// config
#include "config/nation.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/to-str-ext-std.hpp"
#include "base/vocab.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
**Unit Movement Behaviors / Capabilities
*****************************************************************/
#define TEMPLATE_BEHAVIOR                                      \
  template<e_surface target, e_unit_relationship relationship, \
           e_entity_category entity>

#define BEHAVIOR_VALUES( surface, relationship, entity ) \
  e_surface::surface, e_unit_relationship::relationship, \
      e_entity_category::entity

#define BEHAVIOR_NS( surface, relationship, entity ) \
  unit_behavior::surface::relationship::entity

#define BEHAVIOR( c, r, e, ... )                              \
  namespace BEHAVIOR_NS( c, r, e ) {                          \
  enum class e_vals { __VA_ARGS__ };                          \
  }                                                           \
  template<>                                                  \
  struct to_behaviors<BEHAVIOR_VALUES( c, r, e )> {           \
    using type = BEHAVIOR_NS( c, r, e )::e_vals;              \
  };                                                          \
  template<>                                                  \
  [[maybe_unused]] to_behaviors_t<BEHAVIOR_VALUES( c, r, e )> \
  behavior<BEHAVIOR_VALUES( c, r, e )>(                       \
      UnitTypeAttributes const& desc )

enum class e_unit_relationship { neutral, friendly, foreign };
enum class e_entity_category { empty, unit, colony, village };

TEMPLATE_BEHAVIOR
struct to_behaviors {
  // This must be void, it is used as a fallback in code that
  // wants to verify that a combination of parameters is NOT
  // implemented.
  using type = void;
};

TEMPLATE_BEHAVIOR
using to_behaviors_t =
    typename to_behaviors<target, relationship, entity>::type;

TEMPLATE_BEHAVIOR
[[maybe_unused]] to_behaviors_t<target, relationship, entity>
behavior( UnitTypeAttributes const& desc );

/****************************************************************/
BEHAVIOR( land, foreign, unit, no_attack, attack, no_bombard,
          bombard, attack_land_ship );
BEHAVIOR( land, foreign, colony, never, attack, trade );
// BEHAVIOR( land, foreign, village, unused );
BEHAVIOR( land, neutral, empty, never, always, unload );
BEHAVIOR( land, friendly, unit, always, never, unload );
BEHAVIOR( land, friendly, colony, always );
BEHAVIOR( water, foreign, unit, no_attack, attack, no_bombard,
          bombard );
BEHAVIOR( water, neutral, empty, never, always, high_seas );
BEHAVIOR( water, friendly, unit, always, never, move_onto_ship,
          high_seas );
/****************************************************************/

// The macros below are for users of the above functions.

#define CALL_BEHAVIOR( surface_, relationship_, entity_ ) \
  behavior<e_surface::surface_,                           \
           e_unit_relationship::relationship_,            \
           e_entity_category::entity_>( unit.desc() )

// This is a bit of an abuse of the init-statement if the if
// statement. Here we are using it just to automate the calling
// of the behavior function, not to test the result of the
// behavior function.
#define IF_BEHAVIOR( surface_, relationship_, entity_ )     \
  if( surface == e_surface::surface_ &&                     \
      relationship == e_unit_relationship::relationship_ && \
      category == e_entity_category::entity_ )

// This one is used to assert that there is no specialization for
// the given combination of parameters. These can be used for all
// combinations of parameters that are not specialized so that,
// eventually, when they are specialized, the compiler can notify
// us of where we need to insert logic to handle that situation.
#define STATIC_ASSERT_NO_BEHAVIOR( surface, relationship, \
                                   entity )               \
  static_assert(                                          \
      is_same_v<void, decltype( CALL_BEHAVIOR(            \
                          surface, relationship, entity ) )> )

/****************************************************************
** Movement Points
*****************************************************************/
// This one is for when we need to manually specify the required
// movement points, e.g. if a unit is moving onto a colony
// square.
maybe<MovementPoints> check_movement_points(
    TS& ts, Unit const& unit, MovementPoints needed ) {
  MovementPointsAnalysis const analysis =
      can_unit_move_based_on_mv_points( ts, unit, needed );
  if( analysis.allowed() ) {
    if( analysis.using_start_of_turn_exemption )
      lg.debug( "move allowed by start-of-turn exemption." );
    if( analysis.using_overdraw_allowance )
      lg.debug( "move allowed by overdraw allowance." );
    return analysis.points_to_subtract();
  }
  return nothing;
}

// This one is for those cases where the movement points are
// simply determined by the src/dst MapSquare objects. An example
// of when this would not be the case is what there is a colony
// on the destination square.
maybe<MovementPoints> check_movement_points(
    TS& ts, Unit const& unit, MapSquare const& src_square,
    MapSquare const& dst_square, e_direction direction ) {
  MovementPoints const needed = movement_points_required(
      src_square, dst_square, direction );
  return check_movement_points( ts, unit, needed );
}

/****************************************************************
** TravelHandler
*****************************************************************/
struct TravelHandler : public OrdersHandler {
  TravelHandler( Planes& planes, SS& ss, TS& ts, UnitId unit_id_,
                 e_direction d, Player& player )
    : planes_( planes ),
      ss_( ss ),
      ts_( ts ),
      unit_id( unit_id_ ),
      direction( d ),
      player_( player ) {}

  enum class e_travel_verdict {
    // Cancelled.
    cancelled,

    // Forfeight. This one happens when the unit doesn't have
    // enough movement points to move, and is not selected (but
    // the random mechanism) to proceed anyway, and so it just
    // stops and loses the remainder of its points.
    consume_remaining_points,

    // Non-allowed moves (errors).
    map_edge,
    land_forbidden,
    water_forbidden,
    board_ship_full,

    // Allowed moves.
    map_edge_high_seas,
    map_to_map,
    board_ship,
    offboard_ship,
    ship_into_port,
    land_fall,
    sail_high_seas,
  };

  wait<bool> confirm() override {
    verdict = co_await confirm_travel_impl();

    switch( verdict ) {
      // Cancelled by user
      case e_travel_verdict::cancelled: //
        co_return false;

      // Lost remaining movement points in an attempt to move but
      // without enough points.
      case e_travel_verdict::consume_remaining_points: //
        CHECK( checked_mv_points_ );
        break;

      // Non-allowed moves (errors)
      case e_travel_verdict::map_edge:
      case e_travel_verdict::land_forbidden:
      case e_travel_verdict::water_forbidden: //
        // We should not have checked movement points in these
        // cases since they are never allowed.
        CHECK( !checked_mv_points_ );
        co_return false;
      case e_travel_verdict::board_ship_full:
        co_await ts_.gui.message_box(
            "None of the ships on this square have enough free "
            "space to hold this unit!" );
        // We should not have checked movement points in this
        // case since it was an impossible move.
        CHECK( !checked_mv_points_ );
        co_return false;

      // Allowed moves
      case e_travel_verdict::map_edge_high_seas:
        // Don't need to have checked movement points here.
        break;
      case e_travel_verdict::map_to_map:
      case e_travel_verdict::board_ship:
      case e_travel_verdict::ship_into_port:
      case e_travel_verdict::offboard_ship:
      case e_travel_verdict::sail_high_seas: //
        CHECK( checked_mv_points_ );
        break;
      case e_travel_verdict::land_fall: //
        // Shouldn't have checked movement points here because
        // none are needed for making landfall.
        break;
    }

    // Just some sanity checks. Note that for map_edge_high_seas,
    // the move_dst square will not be on the map.
    CHECK( move_src != move_dst );
    CHECK( find( prioritize.begin(), prioritize.end(),
                 unit_id ) == prioritize.end() );
    CHECK( move_src == coord_for_unit_indirect_or_die(
                           ss_.units, unit_id ) );
    CHECK( move_src.is_adjacent_to( move_dst ) );
    CHECK( target_unit != unit_id );

    co_return true;
  }

  wait<> animate() const override {
    // TODO: in the case of board_ship we need to make sure that
    // the ship being borded gets rendered on top because there
    // may be a stack of ships in the square, otherwise it will
    // be deceiving to the player. This is because when a land
    // unit enters a water square it will just automatically pick
    // a ship and board it.
    if( verdict == e_travel_verdict::land_fall ) co_return;

    if( verdict == e_travel_verdict::consume_remaining_points )
      co_return;

    co_await planes_.land_view().landview_animate_move(
        unit_id, direction );
  }

  wait<> perform() override;

  wait<> post() const override {
    // !! Note that the unit theoretically may not exist here if
    // they were destroyed as part of this action, e.g. lost in a
    // lost city rumor.
    co_return;
  }

  vector<UnitId> units_to_prioritize() const override {
    return prioritize;
  }

  wait<e_travel_verdict> confirm_travel_impl();
  wait<e_travel_verdict> analyze_unload() const;

  wait<maybe<ui::e_confirm>> ask_sail_high_seas() const;
  wait<e_travel_verdict>     confirm_sail_high_seas() const;
  wait<e_travel_verdict> confirm_sail_high_seas_map_edge() const;

  Planes& planes_;
  SS&     ss_;
  TS&     ts_;

  // The unit that is moving.
  UnitId      unit_id;
  e_direction direction;

  vector<UnitId> prioritize = {};

  // If this move is allowed and executed, will the unit actually
  // move to the target square as a result? Normally the answer
  // is yes, however there are cases when the answer is no, such
  // as when a ship makes landfall.
  bool unit_would_move = true;

  // The square on which the unit resides.
  Coord move_src{};

  // The square toward which the move is aimed; note that if/when
  // this move is executed the unit will not necessarily move to
  // this square (it depends on the kind of move being made).
  // That said, this field will always contain a valid and mean-
  // ingful value since there must always be a move order in
  // order for this data structure to even be populated.
  Coord move_dst{};

  // Description of what would happen if the move were carried
  // out. This can also serve as a binary indicator of whether
  // the move is possible by checking the type held, as the can_-
  // move() function does as a convenience.
  e_travel_verdict verdict;

  // Unit that is the target of an action, e.g., ship to be
  // boarded, etc. Not relevant in all contexts.
  maybe<UnitId> target_unit = nothing;

  MovementPoints mv_points_to_subtract_ = {};

  // This is used to catch us if we ever forget to check movement
  // points for a given (successful) move.
  bool checked_mv_points_ = false;

  Player& player_;
};

wait<TravelHandler::e_travel_verdict>
TravelHandler::analyze_unload() const {
  std::vector<UnitId> to_offload;
  auto&               unit = ss_.units.unit_for( unit_id );
  for( auto unit_item :
       unit.cargo().items_of_type<Cargo::unit>() ) {
    auto const& cargo_unit = ss_.units.unit_for( unit_item.id );
    if( !cargo_unit.mv_pts_exhausted() )
      to_offload.push_back( unit_item.id );
  }
  if( !to_offload.empty() ) {
    if( ss_.colonies.maybe_from_coord( move_src ) ) {
      co_await ts_.gui.message_box(
          "A ship containing units cannot make landfall while "
          "in port." );
      co_return e_travel_verdict::land_forbidden;
    }
    // We have at least one unit in the cargo that is able to
    // make landfall. So we will indicate that the unit is al-
    // lowed to make this move.
    string msg = "Would you like to make landfall?";
    if( to_offload.size() <
        unit.cargo().items_of_type<Cargo::unit>().size() )
      msg =
          "Some units have already  moved this turn.  Would you "
          "like the remaining units to make landfall anyway?";
    maybe<ui::e_confirm> const answer =
        co_await ts_.gui.optional_yes_no(
            { .msg            = msg,
              .yes_label      = "Make landfall",
              .no_label       = "Stay with ships",
              .no_comes_first = true } );
    co_return ( answer == ui::e_confirm::yes )
        ? e_travel_verdict::land_fall
        : e_travel_verdict::cancelled;
  } else {
    co_return e_travel_verdict::land_forbidden;
  }
}

bool is_high_seas( TerrainState const& terrain_state, Coord c ) {
  return terrain_state.square_at( c ).sea_lane;
}

wait<maybe<ui::e_confirm>> TravelHandler::ask_sail_high_seas()
    const {
  return ts_.gui.optional_yes_no(
      { .msg       = "Would you like to sail the high seas?",
        .yes_label = "Yes, steady as she goes!",
        .no_label  = "No, let us remain in these waters." } );
}

// This is for the case where the destination square is on the
// map. The function below is for when the ship attempts to sail
// off of the map.
wait<TravelHandler::e_travel_verdict>
TravelHandler::confirm_sail_high_seas() const {
  CHECK( is_high_seas( ss_.terrain, move_dst ) );
  // The original game seems to ask to sail the high seas if and
  // only if the following conditions are met:
  //
  //   1. The desination square is high seas.
  //   2. The source square is high seas, but only for atlantic
  //      sea lanes.
  //   3. You are moving in either the ne, e, or se directions
  //      (that's for atlantic sea lanes; the opposite for pa-
  //      cific sea lanes).
  //
  // Not sure the reason for #2, but the benefit of #3 is that
  // you can freely move north/south without getting the prompt
  // which is useful for traveling around the map generally
  // (sometimes continents that are above/below each other are
  // separated by a line of sea lane which would make it perpetu-
  // ally frustrating to travel between if you were prompted to
  // sail the high seas when moving north south), and you can
  // also move west without getting the prompt, which allows
  // starting the ship in the middle of sea lane at the start of
  // the game.
  bool is_atlantic =
      ( move_src.x >= 0 + ss_.terrain.world_size_tiles().w / 2 );
  bool is_pacific  = !is_atlantic;
  bool correct_dst = is_high_seas( ss_.terrain, move_dst );
  bool correct_src =
      is_pacific || is_high_seas( ss_.terrain, move_src );
  UNWRAP_CHECK( d, move_src.direction_to( move_dst ) );
  bool correct_direction = is_atlantic
                               ? ( ( d == e_direction::ne ) ||
                                   ( d == e_direction::e ) ||
                                   ( d == e_direction::se ) )
                               : ( ( d == e_direction::nw ) ||
                                   ( d == e_direction::w ) ||
                                   ( d == e_direction::sw ) );
  bool ask = correct_src && correct_dst && correct_direction;
  if( !ask ) co_return e_travel_verdict::map_to_map;
  maybe<ui::e_confirm> const confirmed =
      co_await ask_sail_high_seas();
  co_return confirmed == ui::e_confirm::yes
      ? TravelHandler::e_travel_verdict::sail_high_seas
      : TravelHandler::e_travel_verdict::map_to_map;
}

// This version is for when the ship sails off of the left or
// right edge of the map.
wait<TravelHandler::e_travel_verdict>
TravelHandler::confirm_sail_high_seas_map_edge() const {
  // The original game asks the player if they want to sail the
  // high seas when a ship attempts to sail off of the left or
  // right edge of the map.
  bool ask = ( move_dst.x == -1 ||
               move_dst.x == ss_.terrain.world_size_tiles().w );
  if( !ask ) co_return e_travel_verdict::cancelled;
  maybe<ui::e_confirm> const confirmed =
      co_await ask_sail_high_seas();
  co_return confirmed == ui::e_confirm::yes
      ? TravelHandler::e_travel_verdict::map_edge_high_seas
      : TravelHandler::e_travel_verdict::cancelled;
}

wait<TravelHandler::e_travel_verdict>
TravelHandler::confirm_travel_impl() {
  UnitId      id   = unit_id;
  Unit const& unit = ss_.units.unit_for( id );
  move_src = coord_for_unit_indirect_or_die( ss_.units, id );
  move_dst = move_src.moved( direction );

  if( !move_dst.is_inside( ss_.terrain.world_rect_tiles() ) ) {
    if( unit.desc().ship )
      co_return co_await confirm_sail_high_seas_map_edge();
    co_return e_travel_verdict::map_edge;
  }

  auto& src_square = ss_.terrain.square_at( move_src );
  auto& dst_square = ss_.terrain.square_at( move_dst );
  UNWRAP_CHECK( direction, move_src.direction_to( move_dst ) );

  // This is for checking if the unit has enough movement points
  // remaining to make the move. The reason that we defer this
  // calculation by wrapping it into a lambda is because it would
  // be premature to run that here, since we need to determine
  // first 1) whether the unit is ever allowed to make the move,
  // what lies on the target square, e.g. a colony. If we didn't
  // do this then we'd have problems in some cases, such as the
  // following: 1) a unit attempting to move from land to a water
  // tile with no ship on it but with a 1/3 of a movement point
  // may have its movement points consumed (and turn ended) even
  // though that unit is never allowed to move onto that square;
  // 2) a unit moving onto a hills tile but which contains a
  // colony would have too many movement points subtracted.
  //
  // On the other hand, if we just check this once at the very
  // end then we run the risk of presenting a confirmation mes-
  // sage to the player regarding a move that will fail anyway on
  // account of movement points; an example of that is a scout
  // attempting to enter an indian village without enough move-
  // ment points.
  //
  // So what we do is we defer this logic by wrapping in a lambda
  // and then making a special call to it in each specific loca-
  // tion where we determine that the unit is allowed to move in
  // general and where we can know exactly how many movement
  // points are needed, but before presenting any messages to the
  // user.
  auto check_points =
      [&, this]( maybe<MovementPoints> needed =
                     nothing ) -> base::NoDiscard<bool> {
    this->checked_mv_points_ = true;
    maybe<MovementPoints> const to_subtract =
        needed.has_value()
            ? check_movement_points( ts_, unit, *needed )
            : check_movement_points( ts_, unit, src_square,
                                     dst_square, direction );
    if( !to_subtract.has_value() ) {
      unit_would_move = false;
      return false;
    }
    mv_points_to_subtract_ = *to_subtract;
    return true;
  };

  CHECK( !unit.mv_pts_exhausted() );

  auto surface = surface_type( dst_square );

  e_unit_relationship relationship =
      e_unit_relationship::neutral;
  if( auto dst_nation =
          nation_from_coord( ss_.units, ss_.colonies, move_dst );
      dst_nation.has_value() ) {
    CHECK( *dst_nation == unit.nation() );
    relationship = e_unit_relationship::friendly;
  }

  auto units_at_dst = ss_.units.from_coord( move_dst );

  e_entity_category category = e_entity_category::empty;
  if( !units_at_dst.empty() ) category = e_entity_category::unit;
  // This must override the above for units.
  if( ss_.colonies.maybe_from_coord( move_dst ).has_value() )
    category = e_entity_category::colony;

  // We are entering an empty land square.
  IF_BEHAVIOR( land, neutral, empty ) {
    using bh_t = unit_behavior::land::neutral::empty::e_vals;
    // Possible results: never, always, unload
    bh_t bh = unit.desc().ship ? bh_t::unload : bh_t::always;
    switch( bh ) {
      case bh_t::never:
        co_return e_travel_verdict::land_forbidden;
      case bh_t::always:
        // `holder` will be a valid value if the unit
        // is cargo of an- other unit; the holder's id
        // in that case will be *holder.
        if( auto holder =
                is_unit_onboard( ss_.units, unit.id() );
            holder ) {
          // We have a unit onboard a ship moving onto land. As-
          // sume a single movement point for this regardless of
          // terrain type. Note: when a unit boards a ship from
          // land then it will consume all of its movement points
          // when doing so, but when a ship departs a colony and
          // brings units with it, their movement points are not
          // consumed, so they may have partial points, thus we
          // need to check here against 1 point; i.e., we can't
          // assume the unit will always just have the start of
          // turn exemption.
          if( !check_points( /*needed=*/MovementPoints( 1 ) ) )
            co_return e_travel_verdict::consume_remaining_points;
          co_return e_travel_verdict::offboard_ship;
        } else {
          if( !check_points() )
            co_return e_travel_verdict::consume_remaining_points;
          co_return e_travel_verdict::map_to_map;
        }
      case bh_t::unload: {
        unit_would_move = false;
        // We don't check movement points here because non are
        // needed for making landfall.
        co_return co_await analyze_unload();
      }
    }
  }
  // We are entering a land square containing a friendly unit.
  IF_BEHAVIOR( land, friendly, unit ) {
    using bh_t = unit_behavior::land::friendly::unit::e_vals;
    // Possible results: always, never, unload
    bh_t bh = unit.desc().ship ? bh_t::unload : bh_t::always;
    switch( bh ) {
      case bh_t::never:
        co_return e_travel_verdict::land_forbidden;
      case bh_t::always:
        // `holder` will be a valid value if the unit
        // is cargo of an- other unit; the holder's id
        // in that case will be *holder.
        if( auto holder =
                is_unit_onboard( ss_.units, unit.id() );
            holder ) {
          // We have a unit onboard a ship moving onto land. See
          // comment for similar line above for explanation of
          // this.
          if( !check_points( /*needed=*/MovementPoints( 1 ) ) )
            co_return e_travel_verdict::consume_remaining_points;
          co_return e_travel_verdict::offboard_ship;
        } else {
          if( !check_points() )
            co_return e_travel_verdict::consume_remaining_points;
          co_return e_travel_verdict::map_to_map;
        }
      case bh_t::unload: {
        unit_would_move = false;
        // We don't check movement points here because non are
        // needed for making landfall.
        co_return co_await analyze_unload();
      }
    }
  }
  // We are entering a land square containing a friendly colony.
  IF_BEHAVIOR( land, friendly, colony ) {
    using bh_t = unit_behavior::land::friendly::colony::e_vals;
    // Possible results: always
    bh_t bh = bh_t::always;
    switch( bh ) {
      case bh_t::always:
        // If we are moving into the colony along a road or
        // river, then it will cost the usual 1/3 movement point.
        // But in the original game, the cost otherwise seems to
        // be capped at one movement points; in other words, if
        // we are moving into a colony square that is located on
        // hills (normally costs 2 points) but is not connected
        // with a road or river, it will only cost 1 movement
        // point. friendly colony square, whether from land or a
        // ship.
        //
        // First get the movement points as if the colony were
        // not present (but the road under it still is).
        MovementPoints const land_only_pts =
            movement_points_required( src_square, dst_square,
                                      direction );
        MovementPoints const needed_pts =
            std::min( land_only_pts, MovementPoints( 1 ) );
        if( !check_points( needed_pts ) )
          co_return e_travel_verdict::consume_remaining_points;
        // NOTE: In the original game, when a wagon train enters
        // a colony it ends its turn. But that is not likely
        // something that we want to replicate in this game be-
        // cause it can be annoying and there doesn't appear to
        // be a good reason for it.
        if( unit.desc().ship )
          co_return e_travel_verdict::ship_into_port;
        // `holder` will be a valid value if the unit is cargo of
        // another unit; the holder's id in that case will be
        // *holder.
        if( auto holder =
                is_unit_onboard( ss_.units, unit.id() );
            holder ) {
          // We have a unit onboard a ship moving onto
          // a land square with a friendly colony.
          co_return e_travel_verdict::offboard_ship;
        } else {
          co_return e_travel_verdict::map_to_map;
        }
    }
  }
  // We are entering an empty water square.
  IF_BEHAVIOR( water, neutral, empty ) {
    using bh_t = unit_behavior::water::neutral::empty::e_vals;
    // Possible results: never, always, high_seas.
    bh_t bh = unit.desc().ship ? bh_t::always : bh_t::never;
    if( unit.desc().ship &&
        is_high_seas( ss_.terrain, move_dst ) )
      bh = bh_t::high_seas;
    switch( bh ) {
      case bh_t::never:
        co_return e_travel_verdict::water_forbidden;
      case bh_t::always:
        if( !check_points() ) {
          // This shouldn't happen in practice since it will al-
          // ways be a ship and they don't have partial movement
          // points, but we include it for consistency.
          co_return e_travel_verdict::consume_remaining_points;
        }
        co_return e_travel_verdict::map_to_map;
      case bh_t::high_seas:
        if( !check_points() ) {
          // This shouldn't happen in practice since it will al-
          // ways be a ship and they don't have partial movement
          // points, but we include it for consistency.
          co_return e_travel_verdict::consume_remaining_points;
        }
        co_return co_await confirm_sail_high_seas();
    }
  }
  // We are entering a water square containing a friendly unit.
  IF_BEHAVIOR( water, friendly, unit ) {
    using bh_t = unit_behavior::water::friendly::unit::e_vals;
    bh_t bh;
    // Possible results: always, never, move_onto_ship,
    //                   high_seas.
    if( unit.desc().ship ) {
      if( is_high_seas( ss_.terrain, move_dst ) )
        bh = bh_t::high_seas;
      else
        bh = bh_t::always;
    } else {
      bh = unit.desc().cargo_slots_occupies.has_value()
               ? bh_t::move_onto_ship
               : bh_t::never;
    }
    switch( bh ) {
      case bh_t::never:
        co_return e_travel_verdict::water_forbidden;
      case bh_t::always:
        if( !check_points() ) {
          // This shouldn't happen in practice since it will al-
          // ways be a ship and they don't have partial movement
          // points, but we include it for consistency.
          co_return e_travel_verdict::consume_remaining_points;
        }
        co_return e_travel_verdict::map_to_map;
      case bh_t::move_onto_ship: {
        auto const& ships = units_at_dst;
        if( ships.empty() )
          co_return e_travel_verdict::water_forbidden;
        // We have at least one ship, so iterate
        // through and find the first one (if any) that
        // the unit can board.
        for( auto ship_id : ships ) {
          auto const& ship_unit = ss_.units.unit_for( ship_id );
          CHECK( ship_unit.desc().ship );
          lg.debug( "checking ship cargo: {}",
                    ship_unit.cargo() );
          if( auto const& cargo = ship_unit.cargo();
              cargo.fits_somewhere( ss_.units,
                                    Cargo::unit{ id } ) ) {
            prioritize  = { ship_id };
            target_unit = ship_id;
            // Assume that it always takes one movement points to
            // board a ships.
            if( !check_points( /*needed=*/MovementPoints( 1 ) ) )
              co_return e_travel_verdict::
                  consume_remaining_points;
            co_return e_travel_verdict::board_ship;
          }
        }
        co_return e_travel_verdict::board_ship_full;
      }
      case bh_t::high_seas:
        if( !check_points() ) {
          // This shouldn't happen in practice since it will al-
          // ways be a ship and they don't have partial movement
          // points, but we include it for consistency.
          co_return e_travel_verdict::consume_remaining_points;
        }
        co_return co_await confirm_sail_high_seas();
    }
  }

  // If one of these triggers then that means that:
  //
  //   1) The line should be removed
  //   2) Some logic should be added above to deal
  //      with that particular situation.
  //   3) Don't do #1 without doing #2
  //
  STATIC_ASSERT_NO_BEHAVIOR( land, friendly, empty );
  STATIC_ASSERT_NO_BEHAVIOR( land, friendly, village );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, unit );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, village );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, empty );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, village );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, unit );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, village );

  SHOULD_NOT_BE_HERE;
}

wait<> TravelHandler::perform() {
  auto  id   = unit_id;
  auto& unit = ss_.units.unit_for( id );

  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders() == e_unit_orders::none );

  // This will throw if the unit has no coords, but I think it
  // should always be ok at this point if we're moving it.
  auto old_coord =
      coord_for_unit_indirect_or_die( ss_.units, id );

  maybe<UnitDeleted> unit_deleted;

  switch( verdict ) {
    case e_travel_verdict::cancelled:
    case e_travel_verdict::map_edge:
    case e_travel_verdict::land_forbidden:
    case e_travel_verdict::water_forbidden:
    case e_travel_verdict::board_ship_full: //
      SHOULD_NOT_BE_HERE;
    case e_travel_verdict::consume_remaining_points: {
      unit.forfeight_mv_points();
      break;
    }
    // Allowed moves.
    case e_travel_verdict::map_to_map: {
      // If it's a ship then sentry all its units before it
      // moves.
      if( unit.desc().ship ) {
        for( Cargo::unit u :
             unit.cargo().items_of_type<Cargo::unit>() ) {
          auto& cargo_unit = ss_.units.unit_for( u.id );
          cargo_unit.sentry();
        }
      }
      unit_deleted =
          co_await unit_to_map_square( ss_, ts_, id, move_dst );
      CHECK_GT( mv_points_to_subtract_, 0 );
      if( unit_deleted.has_value() ) break;
      unit.consume_mv_points( mv_points_to_subtract_ );
      break;
    }
    case e_travel_verdict::board_ship: {
      CHECK( target_unit.has_value() );
      ss_.units.change_to_cargo_somewhere( *target_unit, id );
      unit.forfeight_mv_points();
      unit.sentry();
      // If the ship is sentried then clear it's orders because
      // the player will likely want to start moving it now that
      // a unit has boarded.
      auto& ship_unit = ss_.units.unit_for( *target_unit );
      ship_unit.clear_orders();
      break;
    }
    case e_travel_verdict::offboard_ship:
      unit_deleted =
          co_await unit_to_map_square( ss_, ts_, id, move_dst );
      if( unit_deleted.has_value() ) break;
      unit.forfeight_mv_points();
      CHECK( unit.orders() == e_unit_orders::none );
      break;
    case e_travel_verdict::ship_into_port: {
      unit_deleted =
          co_await unit_to_map_square( ss_, ts_, id, move_dst );
      CHECK( !unit_deleted.has_value() );
      // When a ship moves into port it forfeights its movement
      // points.
      unit.forfeight_mv_points();
      CHECK( unit.orders() == e_unit_orders::none );
      UNWRAP_CHECK( colony_id,
                    ss_.colonies.maybe_from_coord( move_dst ) );
      // TODO: by default we should not open the colony view when
      // a ship moves into port. But it would be convenient to
      // allow the user to specify that they want to open it by
      // holding SHIFT while moving the unit.
      //
      // TODO: consider prioritizing units that are brought in by
      // the ship.
      e_colony_abandoned const abandoned =
          co_await show_colony_view(
              planes_, ss_, ts_,
              ss_.colonies.colony_for( colony_id ) );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        break;
      break;
    }
    case e_travel_verdict::land_fall:
      // First activate and prioritize all the units on the ship
      // that have not completed their turns, so that they will
      // ask for orders. But for the first unit, in addition, we
      // will also just give it orders to move onto land. This
      // way, when the player makes land fall, the first unit el-
      // igible for moving will move automatically, then the sub-
      // sequent ones will ask for orders immediately after. This
      // reproduces the behavior of the original game and is a
      // good user experience.
      //
      // Note that the ship's movement points are not consumed.
      prioritize.clear();
      for( auto unit_item :
           unit.cargo().items_of_type<Cargo::unit>() ) {
        auto& cargo_unit = ss_.units.unit_for( unit_item.id );
        if( !cargo_unit.mv_pts_exhausted() ) {
          cargo_unit.clear_orders();
          prioritize.push_back( unit_item.id );
        }
      }
      // First unit in list should move first.
      reverse( prioritize.begin(), prioritize.end() );
      for( auto unit_item :
           unit.cargo().items_of_type<Cargo::unit>() ) {
        auto& cargo_unit = ss_.units.unit_for( unit_item.id );
        if( !cargo_unit.mv_pts_exhausted() ) {
          UNWRAP_CHECK( direction,
                        old_coord.direction_to( move_dst ) );
          orders_t orders = orders::move{ direction };
          push_unit_orders( unit_item.id, orders );
          // Stop after first eligible unit. The rest of the
          // units will just ask for orders on the ship.
          break;
        }
      }
      break;
    case e_travel_verdict::sail_high_seas:
    case e_travel_verdict::map_edge_high_seas: {
      unit_sail_to_harbor( ss_.terrain, ss_.units, player_, id );
      // Don't process it again this turn.
      unit.forfeight_mv_points();
      break;
    }
  }

  // !! NOTE: unit could be gone here.

  if( unit_deleted.has_value() ) co_return;

  // Now do a sanity check for units that are on the map. The
  // vast majority of the time they are on the map. An example of
  // a case where the unit is no longer on the map at this point
  // would be a ship that was sent to sail the high seas.
  if( is_unit_on_map_indirect( ss_.units, id ) ) {
    auto new_coord =
        coord_for_unit_indirect_or_die( ss_.units, id );
    CHECK( unit_would_move == ( new_coord == move_dst ) );
  }

  co_return; //
}

/****************************************************************
** AttackHandler
*****************************************************************/
struct AttackHandler : public OrdersHandler {
  AttackHandler( Planes& planes, SS& ss, TS& ts, UnitId unit_id_,
                 e_direction d, Player& player )
    : planes_( planes ),
      ss_( ss ),
      ts_( ts ),
      unit_id( unit_id_ ),
      direction( d ),
      player_( player ) {}

  enum class e_attack_verdict {
    cancelled,
    // Allowed moves
    colony_undefended,
    colony_defended,
    eu_land_unit,
    ship_on_ship,
    // Non-allowed (errors)
    land_unit_attack_ship,
    ship_attack_land_unit,
    unit_cannot_attack,
    attack_from_ship,
    land_unit_attack_ship_on_land,
  };

  wait<bool> confirm() override {
    verdict = co_await confirm_attack_impl();

    switch( verdict ) {
      case e_attack_verdict::cancelled: co_return false;
      // Non-allowed (errors)
      case e_attack_verdict::land_unit_attack_ship:
        co_await ts_.gui.message_box(
            "Land units cannot attack ships that are at sea." );
        co_return false;
      case e_attack_verdict::ship_attack_land_unit:
        co_await ts_.gui.message_box(
            "Ships cannot attack land units." );
        co_return false;
      case e_attack_verdict::unit_cannot_attack:
        co_await ts_.gui.message_box(
            "This unit cannot attack." );
        co_return false;
      case e_attack_verdict::attack_from_ship:
        co_await ts_.gui.message_box(
            "Cannot attack from a ship." );
        co_return false;
      // Allowed moves
      case e_attack_verdict::colony_undefended:
      case e_attack_verdict::colony_defended:
      case e_attack_verdict::eu_land_unit:
      case e_attack_verdict::land_unit_attack_ship_on_land:
      case e_attack_verdict::ship_on_ship: //
        break;
    }

    CHECK( attack_src != attack_dst );
    CHECK( attack_src == coord_for_unit_indirect_or_die(
                             ss_.units, unit_id ) );
    CHECK( attack_src.is_adjacent_to( attack_dst ) );
    CHECK( target_unit != unit_id );
    CHECK( target_unit.has_value() );
    CHECK( fight_stats.has_value() );

    co_return true;
  }

  wait<> animate() const override {
    if( verdict == e_attack_verdict::colony_undefended &&
        fight_stats->attacker_wins ) {
      UNWRAP_CHECK( colony_id, ss_.colonies.maybe_from_coord(
                                   attack_dst ) );
      auto attacker_id = unit_id;
      auto defender_id = *target_unit;
      return planes_.land_view().landview_animate_colony_capture(
          attacker_id, defender_id, colony_id );
    }

    auto attacker = unit_id;
    UNWRAP_CHECK( defender, target_unit );
    UNWRAP_CHECK( stats, fight_stats );
    auto const& defender_unit = ss_.units.unit_for( defender );
    auto const& attacker_unit = ss_.units.unit_for( attacker );

    e_depixelate_anim dp_anim =
        stats.attacker_wins
            ? ( defender_unit.desc()
                        .on_death
                        .holds<UnitDeathAction::demote>()
                    ? e_depixelate_anim::demote
                    : e_depixelate_anim::death )
            : ( attacker_unit.desc()
                        .on_death
                        .holds<UnitDeathAction::demote>()
                    ? e_depixelate_anim::demote
                    : e_depixelate_anim::death );
    return planes_.land_view().landview_animate_attack(
        attacker, defender, stats.attacker_wins, dp_anim );
  }

  wait<> perform() override;

  wait<> post() const override {
    // !! Note that the unit theoretically may not exist here if
    // they were destroyed as part of this action, e.g. a ship
    // losing a battle.

    if( verdict == e_attack_verdict::colony_undefended &&
        fight_stats->attacker_wins ) {
      conductor::play_request(
          ts_.rand, conductor::e_request::fife_drum_happy,
          conductor::e_request_probability::always );
      UNWRAP_CHECK( colony_id, ss_.colonies.maybe_from_coord(
                                   attack_dst ) );
      Colony const& colony =
          ss_.colonies.colony_for( colony_id );
      Nationality const& attacker_nation =
          nation_obj( ss_.units.unit_for( unit_id ).nation() );
      co_await ts_.gui.message_box(
          "The @[H]{}@[] have captured the colony of @[H]{}@[]!",
          attacker_nation.display_name, colony.name );
      e_colony_abandoned const abandoned =
          co_await show_colony_view(
              planes_, ss_, ts_,
              ss_.colonies.colony_for( colony_id ) );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        co_return;
    }
  }

  wait<e_attack_verdict> confirm_attack_impl();

  Planes& planes_;
  SS&     ss_;
  TS&     ts_;

  // The unit doing the attacking.
  UnitId      unit_id;
  e_direction direction;

  // The square on which the unit resides.
  Coord attack_src{};

  // The square toward which the attack is aimed; this is the
  // same as the square of the unit being attacked.
  Coord attack_dst{};

  // Description of what would happen if the move were carried
  // out. This can also serve as a binary indicator of whether
  // the move is possible by checking the type held, as the can_-
  // move() function does as a convenience.
  e_attack_verdict verdict{};

  // Unit being attacked.
  maybe<UnitId> target_unit{};

  // If the fight is allowed then this will hold the numerical
  // breakdown of the statistics contributing to the final proba-
  // bilities.
  maybe<FightStatistics> fight_stats{};

  Player& player_;
};

wait<AttackHandler::e_attack_verdict>
AttackHandler::confirm_attack_impl() {
  auto id    = unit_id;
  attack_src = coord_for_unit_indirect_or_die( ss_.units, id );
  attack_dst = attack_src.moved( direction );

  auto& unit = ss_.units.unit_for( id );
  CHECK( !unit.mv_pts_exhausted() );

  if( is_unit_onboard( ss_.units, id ) )
    co_return e_attack_verdict::attack_from_ship;

  if( !can_attack( unit.type() ) )
    co_return e_attack_verdict::unit_cannot_attack;

  if( unit.movement_points() < 1 ) {
    if( co_await ts_.gui.optional_yes_no(
            { .msg = fmt::format(
                  "This unit only has @[H]{}@[] movement points "
                  "and so will not be fighting at full "
                  "strength.  Continue?",
                  unit.movement_points() ),
              .yes_label =
                  "Yes, let us proceed with full force!",
              .no_label = "No, do not attack." } ) !=
        ui::e_confirm::yes )
      co_return e_attack_verdict::cancelled;
  }

  // Make sure there is a foreign entity in the square otherwise
  // there can be no combat.
  auto dst_nation =
      nation_from_coord( ss_.units, ss_.colonies, attack_dst );
  CHECK( dst_nation.has_value() &&
         *dst_nation != unit.nation() );
  CHECK(
      attack_dst.is_inside( ss_.terrain.world_rect_tiles() ) );

  auto& square = ss_.terrain.square_at( attack_dst );

  auto surface      = surface_type( square );
  auto relationship = e_unit_relationship::foreign;
  auto category     = e_entity_category::unit;
  if( ss_.colonies.maybe_from_coord( attack_dst ).has_value() )
    category = e_entity_category::colony;

  auto const& units_at_dst_set =
      ss_.units.from_coord( attack_dst );
  vector<UnitId> units_at_dst( units_at_dst_set.begin(),
                               units_at_dst_set.end() );
  auto           colony_at_dst =
      ss_.colonies.maybe_from_coord( attack_dst );

  // If we have a colony then we only want to get units that are
  // military units (and not ships), since we want the following
  // behavior: attacking a colony first attacks all military
  // units, then once those are gone, the next attack will attack
  // a colonist working in the colony (and if the attack suc-
  // ceeds, the colony is taken) even if there are free colonists
  // on the colony map square.
  if( colony_at_dst ) {
    erase_if( units_at_dst,
              LC( ss_.units.unit_for( _ ).desc().ship ) );
    erase_if( units_at_dst,
              LC( !is_military_unit(
                  ss_.units.unit_for( _ ).type() ) ) );
  }

  // If military units are exhausted then attack the colony.
  if( colony_at_dst && units_at_dst.empty() ) {
    unordered_set<UnitId> const& units_working_in_colony =
        ss_.units.from_colony(
            ss_.colonies.colony_for( *colony_at_dst ) );
    vector<UnitId> sorted( units_working_in_colony.begin(),
                           units_working_in_colony.end() );
    CHECK( sorted.size() > 0 );
    // Sort since order is otherwise unspecified.
    sort( sorted.begin(), sorted.end() );
    units_at_dst.push_back( sorted[0] );
  }
  CHECK( !units_at_dst.empty() );
  // Now let's find the unit with the highest defense points
  // among the units in the target square.
  vector<UnitId> sorted_by_defense( units_at_dst.begin(),
                                    units_at_dst.end() );
  util::sort_by_key(
      sorted_by_defense,
      LC( ss_.units.unit_for( _ ).desc().defense_points ) );
  CHECK( !sorted_by_defense.empty() );
  UnitId highest_defense_unit_id = sorted_by_defense.back();
  Unit&  highest_defense_unit =
      ss_.units.unit_for( highest_defense_unit_id );
  lg.info( "unit in target square with highest defense: {}",
           debug_string( ss_.units, highest_defense_unit_id ) );

  // Deferred evaluation until we know that the attack makes
  // sense.
  auto run_stats = [this, id, highest_defense_unit_id] {
    return fight_statistics(
        ts_.rand, ss_.units.unit_for( id ),
        ss_.units.unit_for( highest_defense_unit_id ) );
  };

  // We are entering a land square containing a foreign unit.
  IF_BEHAVIOR( land, foreign, unit ) {
    using bh_t = unit_behavior::land::foreign::unit::e_vals;
    bh_t bh;
    // Possible results: nothing, attack, bombard, no_bombard
    if( unit.desc().ship ) {
      bh = bh_t::no_bombard;
    } else {
      bh = can_attack( unit.type() ) ? bh_t::attack
                                     : bh_t::no_attack;
      if( bh == bh_t::attack ) {
        // We have a land unit attacking a square with some for-
        // eign land units on it.
        if( highest_defense_unit.desc().ship ) {
          // The unit being attacked is a ship. This is a special
          // case that can happen when a ship is left on land
          // after a colony is abandoned.
          bh = bh_t::attack_land_ship;
        }
      }
    }
    switch( bh ) {
      case bh_t::no_attack:
        co_return e_attack_verdict::unit_cannot_attack;
      case bh_t::attack:
        target_unit = highest_defense_unit_id;
        fight_stats = run_stats();
        co_return e_attack_verdict::eu_land_unit;
      case bh_t::no_bombard:
        co_return e_attack_verdict::ship_attack_land_unit;
      case bh_t::bombard:
        target_unit = highest_defense_unit_id;
        fight_stats = run_stats();
        co_return e_attack_verdict::eu_land_unit;
      case bh_t::attack_land_ship:
        target_unit = highest_defense_unit_id; // ship.
        CHECK( ss_.units.unit_for( *target_unit ).desc().ship );
        // In the original game a ship can be left on land after
        // a colony is abandoned, but if a land unit then tries
        // to attack it the game panics. We will handle it prop-
        // erly, but we don't want a normal battle to ensue, be-
        // cause then the player could "cheat" by leaving a bunch
        // of fortified frigates on land that would be too strong
        // for normal land units to take down. So what this game
        // does is, when a ship is left on land, the player is
        // given a message that they should move it off land as
        // soon as possible because it is vulnerable to attack.
        // And by vulnerable, we mean that if it is attacked by a
        // land unit (no matter how weak) then the land unit will
        // always win. This will prevent the scenario above where
        // the player accumultes ships on land as a "wall."
        fight_stats = FightStatistics{
            .attacker_wins = true,
        };
        co_return e_attack_verdict::
            land_unit_attack_ship_on_land;
    }
  }
  // We are entering a land square containing a foreign unit.
  IF_BEHAVIOR( land, foreign, colony ) {
    using bh_t = unit_behavior::land::foreign::colony::e_vals;
    bh_t bh;
    // Possible results: never, attack, trade.
    if( unit.desc().ship )
      bh = bh_t::trade;
    else if( is_military_unit( unit.type() ) )
      bh = bh_t::attack;
    else
      bh = bh_t::never;
    switch( bh ) {
      case bh_t::never:
        co_return e_attack_verdict::unit_cannot_attack;
      case bh_t::attack: {
        // TODO: Paul Revere will allow a colonist to pick up
        // stockpiled muskets. In the original game it appears
        // that this will only happen once no matter how many
        // muskets are in the colony. For this reason Paul Revere
        // is considered not to be worth much. However, in order
        // to make him worth more, we can expand his effects to
        // have the colonist (a veteran if available) keep taking
        // up as many muskets and horses as are available repeat-
        // edly until they are gone, fighting as if he was a dra-
        // goon or soldier each time (possibly with a fortifica-
        // tion bonus). Only when they are all depeleted does he
        // fight like a normal colonist, and that will be the
        // last battle as usual if he loses. This would allow
        // e.g. a colony with a warehouse expansion to have 300
        // horses, and 300 muskets, and a veteran colonist
        // working somewhere in the colony, but no military de-
        // fending its gates, and this would be equivalent to
        // having six fortified veteran dragoons defending the
        // colony! This might actually make Revere too powerful,
        // maybe the colonists should only pick up muskets and
        // not also horses.
        //
        // TODO: Figure out how to deal with military units that
        // are in the cargo of ships in the port of the colony
        // being attacked. This issue doesn't come up in the
        // original game. Maybe Paul Revere could enable them to
        // fight?
        //
        // TODO: In the original game, things in the colony can
        // be damaged as a result of the attack. Not sure if this
        // requires the attacker winning. This has been observed
        // to include ships in port, which then need to be sent
        // for repair to another colony or europe. Not sure if
        // this includes buildings.
        e_attack_verdict which =
            is_military_unit(
                ss_.units.unit_for( highest_defense_unit_id )
                    .type() )
                ? e_attack_verdict::colony_defended
                : e_attack_verdict::colony_undefended;
        target_unit = highest_defense_unit_id;
        fight_stats = run_stats();
        co_return which;
      }
      case bh_t::trade:
        // FIXME: implement trade.
        co_return e_attack_verdict::unit_cannot_attack;
    }
  }
  // We are entering a water square containing a foreign unit.
  IF_BEHAVIOR( water, foreign, unit ) {
    using bh_t = unit_behavior::water::foreign::unit::e_vals;
    bh_t bh;
    // Possible results: nothing, attack, bombard
    if( !unit.desc().ship )
      bh = bh_t::no_bombard;
    else
      bh = can_attack( unit.type() ) ? bh_t::attack
                                     : bh_t::no_attack;
    switch( bh ) {
      case bh_t::no_attack:
        co_return e_attack_verdict::unit_cannot_attack;
      case bh_t::attack:
        target_unit = highest_defense_unit_id;
        fight_stats = run_stats();
        co_return e_attack_verdict::ship_on_ship;
      case bh_t::no_bombard:;
        co_return e_attack_verdict::land_unit_attack_ship;
      case bh_t::bombard:;
        target_unit = highest_defense_unit_id;
        fight_stats = run_stats();
        co_return e_attack_verdict::ship_on_ship;
    }
  }

  // If one of these triggers then that means that:
  //
  //   1) The line should be removed
  //   2) Some logic should be added above to deal
  //      with that particular situation.
  //   3) Don't do #1 without doing #2
  //
  STATIC_ASSERT_NO_BEHAVIOR( land, foreign, village );

  SHOULD_NOT_BE_HERE;
}

wait<> AttackHandler::perform() {
  auto  id   = unit_id;
  auto& unit = ss_.units.unit_for( id );

  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders() == e_unit_orders::none );
  CHECK( target_unit.has_value() );
  CHECK( fight_stats.has_value() );

  auto& attacker = unit;
  auto& defender = ss_.units.unit_for( *target_unit );
  auto& winner =
      fight_stats->attacker_wins ? attacker : defender;
  auto& loser = fight_stats->attacker_wins ? defender : attacker;

  // The original game seems to consume all movement points of a
  // unit when attacking.
  attacker.forfeight_mv_points();

  switch( verdict ) {
    // Non-allowed (errors)
    case e_attack_verdict::cancelled:
    case e_attack_verdict::land_unit_attack_ship:
    case e_attack_verdict::ship_attack_land_unit:
    case e_attack_verdict::unit_cannot_attack:
    case e_attack_verdict::attack_from_ship: //
      SHOULD_NOT_BE_HERE;
    case e_attack_verdict::colony_undefended: {
      if( !fight_stats->attacker_wins )
        // break since in this case the attacker lost, so nothing
        // special happens; we just do what we normally do when
        // an attacker loses a battle.
        break;
      UNWRAP_CHECK( colony_id, ss_.colonies.maybe_from_coord(
                                   attack_dst ) );
      // 1. The colony changes ownership, as well as all of the
      // units that are working in it and who are on the map at
      // the colony location.
      change_colony_nation( ss_.colonies.colony_for( colony_id ),
                            ss_.units, attacker.nation() );
      // 2. The attacker moves into the colony square.
      maybe<UnitDeleted> unit_deleted =
          co_await unit_to_map_square( ss_, ts_, attacker.id(),
                                       attack_dst );
      CHECK( !unit_deleted.has_value() );
      // 3. The attacker has all movement points consumed.
      attacker.forfeight_mv_points();
      // TODO: what if there are trade routes that involve this
      // colony?
      lg.info( "the {} have captured the colony of {}.",
               nation_obj( attacker.nation() ).display_name,
               ss_.colonies.colony_for( colony_id ).name );
      co_return;
    }
    case e_attack_verdict::colony_defended:
    case e_attack_verdict::eu_land_unit:
    case e_attack_verdict::land_unit_attack_ship_on_land:
    case e_attack_verdict::ship_on_ship: //
      break;
  }

  auto capture_unit = [&]() -> wait<> {
    loser.change_nation( ss_.units, winner.nation() );
    maybe<UnitDeleted> unit_deleted =
        co_await unit_to_map_square(
            ss_, ts_, loser.id(),
            coord_for_unit_indirect_or_die( ss_.units,
                                            winner.id() ) );
    CHECK( !unit_deleted.has_value() );
    // This is so that the captured unit won't ask for orders
    // in the same turn that it is captured.
    loser.forfeight_mv_points();
    loser.clear_orders();
  };

  switch( loser.desc().on_death.to_enum() ) {
    using namespace UnitDeathAction;
    case e::destroy: {
      e_unit_type loser_type   = loser.type();
      e_nation    loser_nation = loser.nation();
      ss_.units.destroy_unit( loser.id() );
      if( loser_type == e_unit_type::scout ||
          loser_type == e_unit_type::seasoned_scout )
        co_await ts_.gui.message_box(
            "@[H]{}@[] scout has been lost!",
            nation_obj( loser_nation ).adjective );
      break;
    }
    case e::naval: {
      if( !attacker.desc().ship ) {
        // NOTE: this is a rare edge case here where the ship
        // being attacked could be on land and the attacker could
        // be a land unit. This can happen when a ship is left on
        // land after a colony is abandoned.
        CHECK( defender.desc().ship );
        // The ship always loses when attacked on land.
        CHECK( loser.id() == defender.id() );
        ss_.units.destroy_unit( loser.id() );
        string msg =
            "Our ship, which was vulnerable in the abandoned "
            "colony port, has been lost due to an attack.";
        co_await ts_.gui.message_box( msg );
        break;
      }
      auto num_units_lost =
          loser.cargo().items_of_type<Cargo::unit>().size();
      lg.info( "ship sunk: {} units onboard lost.",
               num_units_lost );
      string msg =
          fmt::format( "{} @[H]{}@[] sunk by @[H]{}@[] {}",
                       nation_obj( loser.nation() ).adjective,
                       loser.desc().name,
                       nation_obj( winner.nation() ).adjective,
                       winner.desc().name );
      if( num_units_lost == 1 )
        msg += fmt::format(
            ", @[H]1@[] unit onboard has been lost" );
      else if( num_units_lost > 1 )
        msg += fmt::format(
            ", @[H]{}@[] units onboard have been lost",
            num_units_lost );
      msg += '.';
      // Need to destroy unit first before displaying message
      // otherwise the unit will reappear on the map while the
      // message is open.
      ss_.units.destroy_unit( loser.id() );
      co_await ts_.gui.message_box( msg );
      break;
    }
    case e::capture: //
      co_await capture_unit();
      break;
    case e::demote:
      loser.demote_from_lost_battle();
      // TODO: if a unit loses, should it lose all of its move-
      // ment points? Check the original game.
      break;
    case e::capture_and_demote:
      // Need to do this first before demoting the unit.
      string msg = "Unit demoted upon capture!";
      if( loser.type() == e_unit_type::veteran_colonist )
        msg = "Veteran status lost upon capture!";
      co_await capture_unit();
      loser.demote_from_capture();
      co_await ts_.gui.message_box( msg );
      break;
  }
  co_return;
}

/****************************************************************
** Dispatch
*****************************************************************/
unique_ptr<OrdersHandler> dispatch( Planes& planes, SS& ss,
                                    TS& ts, UnitId id,
                                    e_direction d ) {
  Coord dst =
      coord_for_unit_indirect_or_die( ss.units, id ).moved( d );
  auto& unit = ss.units.unit_for( id );

  UNWRAP_CHECK( player, ss.players.players[unit.nation()] );

  if( !dst.is_inside( ss.terrain.world_rect_tiles() ) )
    // This is an invalid move, but the TravelHandler is the one
    // that knows how to handle it.
    return make_unique<TravelHandler>( planes, ss, ts, id, d,
                                       player );

  auto dst_nation =
      nation_from_coord( ss.units, ss.colonies, dst );

  if( !dst_nation.has_value() )
    // No units on target sqaure, so it is just a travel.
    return make_unique<TravelHandler>( planes, ss, ts, id, d,
                                       player );

  if( *dst_nation == unit.nation() )
    // Friendly unit on target square, so not an attack.
    return make_unique<TravelHandler>( planes, ss, ts, id, d,
                                       player );

  // Must be an attack.
  return make_unique<AttackHandler>( planes, ss, ts, id, d,
                                     player );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    Planes& planes, SS& ss, TS& ts, UnitId id,
    orders::move const& mv ) {
  return dispatch( planes, ss, ts, id, mv.d );
}

} // namespace rn
