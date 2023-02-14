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
#include "anim-builders.hpp"
#include "attack-handlers.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "enter-dwelling.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "mv-calc.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "society.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"
#include "base/vocab.hpp"

using namespace std;

namespace rn {

struct Dwelling;

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
enum class e_entity_category { empty, unit, colony };

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
BEHAVIOR( land, neutral, empty, never, always, unload );
BEHAVIOR( land, friendly, unit, always, never, unload );
BEHAVIOR( land, friendly, colony, always );
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
    TS& ts, Player const& player, Unit const& unit,
    MovementPoints needed ) {
  MovementPointsAnalysis const analysis =
      can_unit_move_based_on_mv_points( ts, player, unit,
                                        needed );
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
    TS& ts, Player const& player, Unit const& unit,
    MapSquare const& src_square, MapSquare const& dst_square,
    e_direction direction ) {
  MovementPoints const needed = movement_points_required(
      src_square, dst_square, direction );
  return check_movement_points( ts, player, unit, needed );
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
    switch( verdict ) {
      case e_travel_verdict::cancelled:
      case e_travel_verdict::map_edge:
      case e_travel_verdict::land_forbidden:
      case e_travel_verdict::water_forbidden:
      case e_travel_verdict::board_ship_full:
        SHOULD_NOT_BE_HERE;
      case e_travel_verdict::consume_remaining_points:
        break;
      case e_travel_verdict::board_ship: {
        // In the case of board_ship we need a special animation
        // which does the slide but also makes sure that the ship
        // being boarded gets rendered on top of its stack so
        // that the player knows which ship is being boarded.
        CHECK( target_unit.has_value() );
        AnimationSequence const seq = anim_seq_for_boarding_ship(
            unit_id, *target_unit, direction );
        co_await planes_.land_view().animate( seq );
        break;
      }
      case e_travel_verdict::land_fall:
        // For land fall the unit in question is the ship of-
        // floading units, which does not need any animation it-
        // self.
        break;
      case e_travel_verdict::map_edge_high_seas:
      case e_travel_verdict::map_to_map:
      case e_travel_verdict::offboard_ship:
      case e_travel_verdict::ship_into_port:
      case e_travel_verdict::sail_high_seas: {
        AnimationSequence const seq =
            anim_seq_for_unit_move( unit_id, direction );
        co_await planes_.land_view().animate( seq );
        break;
      }
    }
  }

  wait<> perform() override;

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
        needed.has_value() ? check_movement_points(
                                 ts_, player_, unit, *needed )
                           : check_movement_points(
                                 ts_, player_, unit, src_square,
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
  if( maybe<Society_t> const society =
          society_on_square( ss_, move_dst );
      society.has_value() ) {
    CHECK( *society == Society_t{ Society::european{
                           .nation = unit.nation() } } );
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
        // Friendly colony square, whether from land or a ship.
        //
        // If we are moving into the colony along a road or
        // river, then it will cost the usual 1/3 movement point.
        // But in the original game, the cost otherwise seems to
        // be capped at one movement points; in other words, if
        // we are moving into a colony square that is located on
        // hills (normally costs 2 points) but is not connected
        // with a road or river, it will only cost 1 movement
        // point.
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
        if( unit.desc().ship )
          co_return e_travel_verdict::ship_into_port;
        if( unit.type() == e_unit_type::wagon_train )
          // Note: In the original game, when a wagon train en-
          // ters a colony it ends its turn. But that is not
          // something that we replicate in this game because it
          // can be annoying and there doesn't appear to be a
          // good reason for it. It seems likely that the spirit
          // of that behavior was to prevent a unit moving into a
          // colony, loading or unloading goods, and then contin-
          // uing to move. So what we do is, we avoid consuming
          // the unit's movement points here (apart from what is
          // needed to move into the colony) and then later in
          // the colony view we will consume the unit's movement
          // points if it loads or unloads any goods under some
          // specific conditions; see that code for more info.
          // But note that a ship's movement points *will* be
          // consumed when it moves into a colony (as in the OG),
          // since that seems more reasonable, i.e. a ship needs
          // to slow down and maneuver into port, causing it to
          // lose its points.
          co_return e_travel_verdict::map_to_map;

        // `holder` will be a valid value if the unit is cargo of
        // another unit; the holder's id in that case will be
        // *holder.
        if( auto holder =
                is_unit_onboard( ss_.units, unit.id() );
            holder )
          // We have a unit onboard a ship moving onto a land
          // square with a friendly colony.
          co_return e_travel_verdict::offboard_ship;
        co_return e_travel_verdict::map_to_map;
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
        // We have at least one ship, so iterate through and find
        // the first one (if any) that the unit can board.
        //
        // TODO: we might want to make this a bit more sophisti-
        // cated and allow the player to choose which ship to
        // board with a popup.
        for( auto generic_ship_id : ships ) {
          UnitId const ship_id =
              ss_.units.check_euro_unit( generic_ship_id );
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
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, unit );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, empty );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, unit );

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
      // points as in the OG.
      unit.forfeight_mv_points();
      CHECK( unit.orders() == e_unit_orders::none );
      UNWRAP_CHECK( colony_id,
                    ss_.colonies.maybe_from_coord( move_dst ) );
      // Unload units and prioritize them.
      vector<UnitId> const held = unit.cargo().units();
      for( UnitId const held_id : held ) {
        unit_deleted = co_await unit_to_map_square(
            ss_, ts_, held_id, move_dst );
        CHECK( !unit_deleted.has_value() );
        ss_.units.unit_for( held_id ).clear_orders();
        prioritize.push_back( held_id );
      }
      // TODO: by default we should not open the colony view when
      // a ship moves into port. But it would be convenient to
      // allow the user to specify that they want to open it by
      // holding SHIFT while moving the unit.
      e_colony_abandoned const abandoned =
          co_await ts_.colony_viewer.show( ts_, colony_id );
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
** NativeDwellingHandler
*****************************************************************/
struct NativeDwellingHandler : public OrdersHandler {
  NativeDwellingHandler( Planes& planes, SS& ss, TS& ts,
                         Player& player, UnitId unit_id,
                         e_direction d, Dwelling& dwelling )
    : planes_( planes ),
      ss_( ss ),
      ts_( ts ),
      player_( player ),
      unit_id_( unit_id ),
      unit_( ss_.units.unit_for( unit_id ) ),
      tribe_( ss.natives.tribe_for( dwelling.id ) ),
      dwelling_( dwelling ),
      direction_( d ),
      move_src_( coord_for_unit_indirect_or_die( ss.units,
                                                 unit_.id() ) ),
      move_dst_( move_src_.moved( d ) ) {}

  EnterDwellingOutcome_t compute_enter_dwelling_outcome(
      e_enter_dwelling_option option ) const {
    switch( option ) {
      case e_enter_dwelling_option::live_among_the_natives:
        return EnterDwellingOutcome::live_among_the_natives{
            .outcome = compute_live_among_the_natives(
                ss_, dwelling_, unit_ ) };
      case e_enter_dwelling_option::speak_with_chief:
        return EnterDwellingOutcome::speak_with_chief{
            .outcome = compute_speak_with_chief(
                ss_, ts_, dwelling_, unit_ ) };
      case e_enter_dwelling_option::attack_village:
        // Outcome handled by the delegated handler.
        return EnterDwellingOutcome::attack_village{};
      case e_enter_dwelling_option::attack_brave_on_dwelling:
        return EnterDwellingOutcome::attack_brave_on_dwelling{};
      case e_enter_dwelling_option::establish_mission:
        return EnterDwellingOutcome::establish_mission{
            .outcome = compute_establish_mission( ss_, player_,
                                                  dwelling_ ) };
      case e_enter_dwelling_option::cancel:
        return EnterDwellingOutcome::cancel{};
    }
  }

  // Returns true if the move is allowed.
  wait<bool> confirm() override {
    if( !unit_.desc().ship &&
        ss_.terrain.square_at( move_src_ ).surface ==
            e_surface::water ) {
      co_await ts_.gui.message_box(
          "A land unit cannot enter a square occupied by an "
          "enemy power directly from a ship.  We must first "
          "move them onto a land square that is either empty or "
          "occupied by friendly forces." );
      co_return false;
    }

    EnterNativeDwellingOptions const options =
        enter_native_dwelling_options( ss_, player_,
                                       unit_.type(), dwelling_ );
    outcome_ = compute_enter_dwelling_outcome(
        co_await present_dwelling_entry_options(
            ss_, ts_, player_, options ) );

    // The move is always allowed for any unit; if the unit can't
    // do anything or if the unit cancels then the unit's move-
    // ment points will still be consumed.
    co_return true;
  }

  unique_ptr<OrdersHandler> switch_handler() override {
    // If we're attacking the dwelling then first check if there
    // is a brave sitting on top of the dwelling. If so, then we
    // delegate to the handler that handles attacking
    // free-standing braves so that we don't have to duplicate
    // that logic here.
    if( outcome_.holds<
            EnterDwellingOutcome::attack_brave_on_dwelling>() ) {
      TribeRelationship& relationship =
          tribe_.relationship[unit_.nation()];
      // The player has already confirmed that they want to at-
      // tack, so no need to re-ask them.
      relationship.nation_has_attacked_tribe = true;
      // Delegate: the order handling process will be restarted
      // with this new handler.
      NativeUnitId const defender_id =
          select_native_unit_defender( ss_, move_dst_ );
      return attack_native_unit_handler(
          planes_, ss_, ts_, player_, unit_id_, defender_id );
    }

    if( outcome_
            .holds<EnterDwellingOutcome::attack_village>() ) {
      TribeRelationship& relationship =
          tribe_.relationship[unit_.nation()];
      // The player has already confirmed that they want to at-
      // tack.
      relationship.nation_has_attacked_tribe = true;
      // Delegate: the order handling process will be restarted
      // with this new handler.
      return attack_dwelling_handler( planes_, ss_, ts_, player_,
                                      unit_id_, dwelling_.id );
    }

    return nullptr; // Continue with this handler.
  }

  wait<> animate() const override {
    // Note that animations for some of the outcomes are handled
    // by the methods called in the perform() function.
    co_return;
  }

  wait<> perform() override {
    // The OG will always drain the movement points of the unit
    // completely (even a scout) when it enters a village, and
    // will do so even when the user then cancels the action.
    unit_.forfeight_mv_points();

    switch( outcome_.to_enum() ) {
      case EnterDwellingOutcome::e::live_among_the_natives: {
        auto& o = outcome_.get<
            EnterDwellingOutcome::live_among_the_natives>();
        co_await do_live_among_the_natives( planes_, ss_, ts_,
                                            dwelling_, player_,
                                            unit_, o.outcome );
        break;
      }
      case EnterDwellingOutcome::e::speak_with_chief: {
        auto& o =
            outcome_
                .get<EnterDwellingOutcome::speak_with_chief>();
        co_await do_speak_with_chief( planes_, ss_, ts_,
                                      dwelling_, player_, unit_,
                                      o.outcome );
        // !! Note that the unit may no longer exist here if the
        // scout was used a target practice.
        break;
      }
      case EnterDwellingOutcome::e::establish_mission: {
        auto& o =
            outcome_
                .get<EnterDwellingOutcome::establish_mission>();
        co_await do_establish_mission(
            ss_, ts_, player_, dwelling_, unit_, o.outcome );
        break;
      }
      case EnterDwellingOutcome::e::attack_village: {
        // This should have been diverted to another handler.
        SHOULD_NOT_BE_HERE;
      }
      case EnterDwellingOutcome::e::attack_brave_on_dwelling: {
        // This should have been diverted to another handler.
        SHOULD_NOT_BE_HERE;
      }
      case EnterDwellingOutcome::e::cancel:
        // Do nothing.
        break;
    }
    // !! Note that the unit may no longer exist here e.g. if a
    // scout was used a target practice or scout lost an attack.
  }

  Planes& planes_;
  SS&     ss_;
  TS&     ts_;
  Player& player_;

  // The unit doing the attacking. We need to record the unit id
  // so that we can test if the unit has been destroyed.
  UnitId    unit_id_;
  Unit&     unit_;
  Tribe&    tribe_;
  Dwelling& dwelling_;

  // Source and destination squares of the move.
  e_direction direction_;
  Coord       move_src_;
  Coord       move_dst_;

  // In case we are attacking the village.
  EnterDwellingOutcome_t outcome_;
};

/****************************************************************
** Dispatch
*****************************************************************/
unique_ptr<OrdersHandler> dispatch( Planes& planes, SS& ss,
                                    TS& ts, Player& player,
                                    UnitId      attacker_id,
                                    e_direction d ) {
  Coord const src =
      coord_for_unit_indirect_or_die( ss.units, attacker_id );
  Coord const dst      = src.moved( d );
  Unit&       attacker = ss.units.unit_for( attacker_id );

  if( !dst.is_inside( ss.terrain.world_rect_tiles() ) )
    // This is an invalid move, but the TravelHandler is the one
    // that knows how to handle it.
    return make_unique<TravelHandler>( planes, ss, ts,
                                       attacker_id, d, player );

  maybe<Society_t> const society = society_on_square( ss, dst );

  if( !society.has_value() )
    // No entities on target sqaure, so it is just a travel.
    return make_unique<TravelHandler>( planes, ss, ts,
                                       attacker_id, d, player );
  CHECK( society.has_value() );

  if( *society == Society_t{ Society::european{
                      .nation = attacker.nation() } } )
    // Friendly unit on target square, so not an attack.
    return make_unique<TravelHandler>( planes, ss, ts,
                                       attacker_id, d, player );

  if( society->holds<Society::native>() ) {
    maybe<DwellingId> const dwelling_id =
        ss.natives.maybe_dwelling_from_coord( dst );

    // First, if there is a dwelling on the tile then allow the
    // unit to enter the dwelling regardless of whether there is
    // a brave on the tile or not.
    if( dwelling_id.has_value() )
      return make_unique<NativeDwellingHandler>(
          planes, ss, ts, player, attacker_id, d,
          ss.natives.dwelling_for( *dwelling_id ) );

    // Must be attacking a brave.
    NativeUnitId const defender_id =
        select_native_unit_defender( ss, dst );
    return attack_native_unit_handler(
        planes, ss, ts, player, attacker_id, defender_id );
  }

  // Must be an attack (or an attempted attack) on a foreign eu-
  // ropean unit or colony. First check for an undefended colony.
  if( maybe<ColonyId> colony_id =
          ss.colonies.maybe_from_coord( dst );
      colony_id.has_value() ) {
    Colony&      colony = ss.colonies.colony_for( *colony_id );
    UnitId const defender_id =
        select_colony_defender( ss, colony );
    Unit const& defender = ss.units.unit_for( defender_id );
    if( is_military_unit( defender.desc().type ) )
      return attack_euro_land_handler(
          planes, ss, ts, player, attacker_id, defender_id );
    else
      return attack_colony_undefended_handler(
          planes, ss, ts, player, attacker_id, defender_id,
          colony );
  }

  UnitId const defender_id =
      select_euro_unit_defender( ss, dst );

  // Must be an attack on a foreign european unit. Check if this
  // is a naval battle, which is defined as the attacker being a
  // ship. Note that a land unit can attack a ship that is stuck
  // on land, in which case that is just a land battle. So it is
  // really the attacker's ship status that determines whether
  // this is a naval battle.
  if( attacker.desc().ship )
    return naval_battle_handler( planes, ss, ts, player,
                                 attacker_id, defender_id );

  // We are attacking a non-ship foreign european unit either
  // outside of a colony or at a colony's gate.
  CHECK( !ss.units.unit_for( defender_id ).desc().ship );
  return attack_euro_land_handler( planes, ss, ts, player,
                                   attacker_id, defender_id );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    Planes& planes, SS& ss, TS& ts, Player& player, UnitId id,
    orders::move const& mv ) {
  return dispatch( planes, ss, ts, player, id, mv.d );
}

} // namespace rn
