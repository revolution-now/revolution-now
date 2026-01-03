/****************************************************************
**world.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-03.
*
* Description: Helper for building fake worlds for unit tests.
*
*****************************************************************/
#include "world.hpp"

// Testing
#include "test/mocks/iagent.hpp"
#include "test/mocks/icolony-viewer.hpp"
#include "test/mocks/icombat.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-agent.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/video/ivideo.hpp"

// Revolution Now
#include "src/agents.hpp"
#include "src/colony-mgr.hpp"
#include "src/commodity.hpp"
#include "src/connectivity.hpp"
#include "src/harbor-units.hpp"
#include "src/irand.hpp"
#include "src/lua.hpp"
#include "src/map-square.hpp"
#include "src/map-updater.hpp"
#include "src/market.hpp"
#include "src/missionary.hpp"
#include "src/plane-stack.hpp"
#include "src/player-mgr.hpp"
#include "src/road.hpp"
#include "src/ts.hpp"
#include "src/unit-mgr.hpp"
#include "src/unit-ownership.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/market.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/production.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "src/ss/nation.hpp"
#include "src/ss/players.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/root.hpp"

// luapp
#include "luapp/ext-refl.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// rand
#include "src/rand/random.hpp"

// base
#include "src/base/keyval.hpp"
#include "src/base/to-str-ext-std.hpp"

using namespace std;

namespace rn::testing {

namespace {

using ::gfx::point;

}

FormatVersion& World::version() { return root().version; }
EventsState& World::events() { return root().events; }
SettingsState& World::settings() { return root().settings; }
UnitsState& World::units() { return root().units; }
PlayersState& World::players() { return root().players; }
TurnState& World::turn() { return root().turn; }
ColoniesState& World::colonies() { return root().colonies; }
NativesState& World::natives() { return root().natives; }
LandViewState& World::land_view() { return root().land_view; }
MapState& World::map() { return root().map; }
TradeRouteState& World::trade_routes() {
  return root().trade_routes;
}
TerrainState& World::terrain() { return root().zzz_terrain; }
RootState& World::root() { return ss_->root; }

FormatVersion const& World::version() const {
  return root().version;
}
EventsState const& World::events() const {
  return root().events;
}
SettingsState const& World::settings() const {
  return root().settings;
}
UnitsState const& World::units() const { return root().units; }
PlayersState const& World::players() const {
  return root().players;
}
TurnState const& World::turn() const { return root().turn; }
ColoniesState const& World::colonies() const {
  return root().colonies;
}
NativesState const& World::natives() const {
  return root().natives;
}
LandViewState const& World::land_view() const {
  return root().land_view;
}
MapState const& World::map() const { return root().map; }
TradeRouteState const& World::trade_routes() const {
  return root().trade_routes;
}
TerrainState const& World::terrain() const {
  return root().zzz_terrain;
}
RootState const& World::root() const { return ss_->root; }

SS& World::ss() { return *ss_; }
SSConst const& World::ss() const { return *ss_const_; }
SS& World::ss_saved() { return *ss_saved_; }

NativeAgents& World::native_agents() {
  if( uninitialized_native_agents_ == nullptr )
    uninitialized_native_agents_ = [] {
      unordered_map<e_tribe, unique_ptr<INativeAgent>> holder;
      for( e_tribe const tribe : refl::enum_values<e_tribe> )
        holder[tribe] = make_unique<MockINativeAgent>( tribe );
      return make_unique<NativeAgents>( std::move( holder ) );
    }();
  return *uninitialized_native_agents_;
}

Agents& World::agents() {
  if( uninitialized_agents_ == nullptr )
    uninitialized_agents_ = [] {
      unordered_map<e_player, unique_ptr<IAgent>> holder;
      for( e_player const player : refl::enum_values<e_player> )
        holder[player] = make_unique<MockIAgent>( player );
      return make_unique<Agents>( std::move( holder ) );
    }();
  return *uninitialized_agents_;
}

MockINativeAgent& World::native_agent( e_tribe tribe ) {
  return static_cast<MockINativeAgent&>(
      native_agents()[tribe] );
}

MockIAgent& World::agent( maybe<e_player> player ) {
  return static_cast<MockIAgent&>(
      agents()[player.value_or( default_player_type_ )] );
}

Planes& World::planes() {
  if( uninitialized_planes_ == nullptr )
    uninitialized_planes_ = make_unique<Planes>();
  return *uninitialized_planes_;
}

lua::state& World::lua() {
  if( uninitialized_lua_ == nullptr ) {
    uninitialized_lua_ = make_unique<lua::state>();
    lua::state& st     = *uninitialized_lua_;
    // FIXME: need to dedupe this logic.
    st["ROOT"]        = root();
    st["SS"]          = ss();
    st["IMapUpdater"] = map_updater();
    st["IRand"]       = static_cast<IRand&>( rand() );
  }
  return *uninitialized_lua_;
}

MockIGui& World::gui() {
  if( uninitialized_gui_ == nullptr )
    uninitialized_gui_ = make_unique<MockIGui>();
  return *uninitialized_gui_;
}

MockIRand& World::rand() {
  if( uninitialized_rand_ == nullptr )
    uninitialized_rand_ = make_unique<MockIRand>();
  return *uninitialized_rand_;
}

MockICombat& World::combat() {
  if( uninitialized_combat_ == nullptr )
    uninitialized_combat_ = make_unique<MockICombat>();
  return *uninitialized_combat_;
}

MockIColonyViewer& World::colony_viewer() {
  if( uninitialized_colony_viewer_ == nullptr )
    uninitialized_colony_viewer_ =
        make_unique<MockIColonyViewer>();
  return *uninitialized_colony_viewer_;
}

MockIEngine& World::engine() {
  if( uninitialized_engine_ == nullptr ) {
    uninitialized_engine_ = make_unique<MockIEngine>();
    // NOTE: this way we don't have to do this in every test.
    uninitialized_engine_->EXPECT__rand().by_default().returns(
        rand() );
  }
  return *uninitialized_engine_;
}

vid::MockIVideo& World::video() {
  if( uninitialized_video_ == nullptr )
    uninitialized_video_ = make_unique<vid::MockIVideo>();
  return *uninitialized_video_;
}

namespace {

// We need this because we can't (yet?) do aggregate initializa-
// tion for an object on the heap. We have to use new and return
// a pointer because the TS objects are not relocatable, since
// they currently store a reference to themselves in the lua
// state (please FIXME).
TS* make_ts( World& world ) {
  auto* ts =
      new TS( world.planes(), world.gui(), world.combat(),
              world.colony_viewer(), world.ss_saved().root );
  ts->set_map_updater_no_restore( world.map_updater() );
  ts->set_native_agents_no_restore( world.native_agents() );
  ts->set_agents_no_restore( world.agents() );
  return ts;
}

} // namespace

TS& World::ts() {
  if( uninitialized_ts_ == nullptr )
    uninitialized_ts_ = unique_ptr<TS>( make_ts( *this ) );
  return *uninitialized_ts_;
}

MapSquare World::make_ocean() {
  return map_square_for_terrain( e_terrain::ocean );
}

MapSquare World::make_sea_lane() {
  MapSquare S = map_square_for_terrain( e_terrain::ocean );
  S.sea_lane  = true;
  return S;
}

MapSquare World::make_grassland() {
  return map_square_for_terrain( e_terrain::grassland );
}

MapSquare World::make_terrain( e_terrain terrain ) {
  return map_square_for_terrain( terrain );
}

void World::build_map( vector<MapSquare> tiles, W width ) {
  map_updater().modify_entire_map_no_redraw(
      [&]( RealTerrain& real_terrain ) {
        real_terrain.map =
            gfx::Matrix<MapSquare>( std::move( tiles ), width );
      } );
  init_player_maps();
}

void World::init_player_maps() {
  for( e_player player : refl::enum_values<e_player> ) {
    if( !ss().players.players[player].has_value() ) continue;
    ss().mutable_terrain_use_with_care.initialize_player_terrain(
        player, /*visible=*/false );
  }
}

MapSquare& World::square( point const p ) {
  ( *connectivity_ ) = {}; // mark as dirty.
  return terrain().mutable_square_at( Coord::from_gfx( p ) );
}

PlayerSquare& World::player_square(
    point const p, maybe<e_player> const player_type ) {
  return ss()
      .mutable_terrain_use_with_care
      .mutable_player_terrain(
          player_type.value_or( default_player_type() ) )
      .map[Coord::from_gfx( p )];
}

void World::add_forest( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::forest;
      } );
}

void World::add_mountains( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::mountains;
      } );
}

void World::add_hills( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.overlay = e_land_overlay::hills;
      } );
}

void World::add_road( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ),
      []( MapSquare& square ) { square.road = true; } );
}

void World::add_plow( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ),
      []( MapSquare& square ) { square.irrigation = true; } );
}

void World::add_minor_river( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.river = e_river::minor;
      } );
}

void World::add_major_river( point const p ) {
  map_updater_->modify_map_square(
      Coord::from_gfx( p ), []( MapSquare& square ) {
        square.river = e_river::major;
      } );
}

Unit& World::add_unit_in_port( e_unit_type type,
                               maybe<e_player> player_type ) {
  return units().unit_for( create_unit_in_harbor(
      ss(),
      player( player_type.value_or( default_player_type_ ) ),
      type ) );
}

Unit& World::add_unit_sailing_to_port(
    e_unit_type const type, maybe<e_player> const player ) {
  UnitId const unit_id = create_unit_in_harbor(
      ss(),
      this->player( player.value_or( default_player_type_ ) ),
      type );
  unit_sail_to_harbor( ss(), unit_id );
  return units().unit_for( unit_id );
}

Unit& World::add_unit_sailing_from_port(
    e_unit_type const type, maybe<e_player> const player ) {
  UnitId const unit_id = create_unit_in_harbor(
      ss(),
      this->player( player.value_or( default_player_type_ ) ),
      type );
  unit_sail_to_new_world( ss(), unit_id );
  return units().unit_for( unit_id );
}

NativeUnit& World::add_native_unit_on_map(
    e_native_unit_type type, point const tile,
    DwellingId dwelling_id ) {
  return units().unit_for( create_unit_on_map_non_interactive(
      ss(), type, tile, dwelling_id ) );
}

Unit& World::add_unit_on_map( UnitComposition const& comp,
                              point const tile,
                              maybe<e_player> player_type ) {
  if( !player_type ) player_type = default_player_type_;
  UnitId const unit_id = create_unit_on_map_non_interactive(
      ss(), ts().map_updater(), player( *player_type ), comp,
      tile );
  return units().unit_for( unit_id );
}

Unit& World::add_missionary_in_dwelling(
    UnitType missionary_type, DwellingId dwelling_id,
    maybe<e_player> player_type ) {
  CHECK( is_missionary( missionary_type.type() ) );
  if( !player_type ) player_type = default_player_type_;
  UnitId const unit_id = create_free_unit(
      units(), player( *player_type ), missionary_type );
  UnitOwnershipChanger( ss(), unit_id )
      .change_to_dwelling( dwelling_id );
  return units().unit_for( unit_id );
}

Unit& World::add_unit_in_cargo( e_unit_type type,
                                UnitId holder ) {
  e_player const player_type =
      units().unit_for( holder ).player_type();
  UnitId held =
      create_free_unit( units(), player( player_type ), type );
  UnitOwnershipChanger( ss(), held )
      .change_to_cargo( holder, /*starting_slot=*/0 );
  return units().unit_for( held );
}

Unit& World::add_free_unit( e_unit_type type,
                            maybe<e_player> player_type ) {
  if( !player_type ) player_type = default_player_type_;
  UnitId const id =
      create_free_unit( units(), player( *player_type ), type );
  return units().unit_for( id );
}

Unit& World::add_unit_indoors( ColonyId colony_id,
                               e_indoor_job indoor_job,
                               e_unit_type type ) {
  Colony& colony  = colonies().colony_for( colony_id );
  Coord const loc = colonies().coord_for( colony_id );
  Unit& unit      = add_unit_on_map( type, loc, colony.player );
  ColonyJob::indoor const job{ .job = indoor_job };
  UnitOwnershipChanger( ss(), unit.id() )
      .change_to_colony( ts(), colony, job );
  return unit;
}

Unit& World::add_expert_unit_indoors( ColonyId colony_id,
                                      e_indoor_job indoor_job ) {
  return add_unit_indoors( colony_id, indoor_job,
                           config_production.indoor_production
                               .expert_for[indoor_job] );
}

void World::ship_to_outbound( UnitId id ) {
  Unit const& unit = units().unit_for( id );
  CHECK( unit.desc().ship );
  unit_sail_to_new_world( ss(), id );
}

Unit& World::add_unit_outdoors( ColonyId colony_id,
                                e_direction d,
                                e_outdoor_job outdoor_job,
                                e_unit_type type ) {
  Colony& colony = colonies().colony_for( colony_id );
  Coord loc      = colonies().coord_for( colony_id );
  Unit& unit     = add_unit_on_map( type, loc, colony.player );
  ColonyJob::outdoor job{ .direction = d, .job = outdoor_job };
  UnitOwnershipChanger( ss(), unit.id() )
      .change_to_colony( ts(), colony, job );
  return unit;
}

Unit& World::add_expert_unit_outdoors(
    ColonyId colony_id, e_direction d,
    e_outdoor_job outdoor_job ) {
  return add_unit_outdoors( colony_id, d, outdoor_job,
                            config_production.outdoor_production
                                .expert_for[outdoor_job] );
}

// Try to add the commodity into the cargo and fail if it
// cannot be added. Returns slot where it was placed.
void World::add_commodity_in_cargo( Commodity const& comm,
                                    UnitId holder,
                                    int starting_slot ) {
  add_commodity_to_cargo( units(), comm,
                          units().unit_for( holder ).cargo(),
                          starting_slot,
                          /*try_other_slots=*/true );
}

// Adds a quantity of 100.
void World::add_commodity_in_cargo( e_commodity type,
                                    UnitId holder,
                                    int starting_slot ) {
  add_commodity_in_cargo(
      Commodity{ .type = type, .quantity = 100 }, holder,
      starting_slot );
}

Player& World::add_player( e_player const player_type ) {
  add_new_player( ss(), player_type );
  return player( player_type );
}

void World::add_all_non_ref_players(
    maybe<e_player> const human ) {
  for( e_player const player_type :
       refl::enum_values<e_player> ) {
    if( is_ref( player_type ) ) continue;
    add_player( player_type );
  }
  set_human_player_and_rest_ai( human );
  if( human.has_value() )
    set_default_player_type( *human );
  else
    set_default_player_type( e_player::english );
}

e_nation World::default_nation() const {
  return nation_for( default_player_type() );
}

void World::add_default_player() {
  add_player( default_player_type() );
}

void World::set_human_player_and_rest_ai(
    maybe<e_player> player_type ) {
  for( e_player const p : refl::enum_values<e_player> ) {
    if( players().players[p].has_value() ) {
      if( p == player_type )
        players().players[p]->control = e_player_control::human;
      else
        players().players[p]->control = e_player_control::ai;
    }
  }
  CHECK_HAS_VALUE( players().validate() );
}

void World::set_default_player_as_human() {
  default_player().control = e_player_control::human;
}

void World::set_player_active( maybe<e_player> const player ) {
  turn().cycle = TurnCycle::player{
    .type = player.value_or( default_player_type() ) };
}

void World::set_no_player_active() {
  turn().cycle = TurnCycle::not_started{};
}

void World::make_clear( point const tile,
                        maybe<e_player> const player ) {
  map_updater().make_squares_visible(
      player.value_or( default_player_type() ), { tile } );
}

void World::make_fogged( point const tile,
                         maybe<e_player> const player ) {
  make_clear( tile, player );
  map_updater().make_squares_fogged(
      player.value_or( default_player_type() ), { tile } );
}

Colony& World::found_colony( UnitId founder ) {
  string name = fmt::to_string(
      colonies().last_colony_id().value_or( 0 ) + 1 );
  ColonyId id = rn::found_colony(
      ss(), ts(),
      player( ss().units.unit_for( founder ).player_type() ),
      founder, name );
  return colonies().colony_for( id );
}

Colony& World::add_colony( Coord where,
                           maybe<e_player> player_type ) {
  string name = fmt::to_string(
      colonies().last_colony_id().value_or( 0 ) + 1 );
  Colony& colony = colonies().colony_for( create_empty_colony(
      colonies(), player_type.value_or( default_player_type_ ),
      where, name ) );
  // Reproduce the things that we need that "found_colony" does.
  colony.buildings = config_colony.initial_colony_buildings;
  set_road( map_updater(), where );
  return colony;
}

pair<Colony&, Unit&> World::found_colony_with_new_unit(
    Coord where, maybe<e_player> player_type ) {
  if( !player_type ) player_type = default_player_type_;
  Unit& founder  = add_unit_on_map( e_unit_type::free_colonist,
                                    where, *player_type );
  Colony& colony = this->found_colony( founder.id() );
  return { colony, founder };
}

void World::kill_all_colonies(
    maybe<e_player> const player_type ) {
  auto const& colonies = ss().colonies.all();
  // We can't destroy the colonies while iterating through the
  // above map since it would then be mutated as we iterate, so
  // we need to extract the IDs.
  vector<ColonyId> colony_ids;
  colony_ids.reserve( colonies.size() );
  for( auto const& [colony_id, colony] : colonies )
    colony_ids.push_back( colony_id );
  for( ColonyId const colony_id : colony_ids ) {
    Colony& colony = ss().colonies.colony_for( colony_id );
    if( player_type.has_value() &&
        colony.player != *player_type )
      continue;
    destroy_colony( ss(), ts(), colony );
  }
}

// --------------------------------------------------------------
// Creating Native Dwellings.
// --------------------------------------------------------------
Dwelling& World::add_dwelling( point const tile,
                               e_tribe tribe ) {
  natives().create_or_get_tribe( tribe );
  DwellingId const id = natives().add_dwelling(
      tribe, Coord::from_gfx( tile ),
      Dwelling{
        .population =
            config_natives.tribes[tribe].max_population,
      } );
  set_road( map_updater(), Coord::from_gfx( tile ) );
  return natives().dwelling_for( id );
}

pair<Dwelling&, NativeUnit&> World::add_dwelling_and_brave(
    Coord where, e_tribe tribe,
    maybe<e_native_unit_type> type ) {
  Dwelling& dwelling = add_dwelling( where, tribe );
  NativeUnit& unit   = add_native_unit_on_map(
      type.value_or( e_native_unit_type::brave ), where,
      dwelling.id );
  return pair<Dwelling&, NativeUnit&>{ dwelling, unit };
}

pair<DwellingId, NativeUnitId> World::add_dwelling_and_brave_ids(
    Coord where, e_tribe tribe,
    maybe<e_native_unit_type> type ) {
  pair<Dwelling&, NativeUnit&> p =
      add_dwelling_and_brave( where, tribe, type );
  return { p.first.id, p.second.id };
}

Tribe& World::add_tribe( e_tribe tribe ) {
  return natives().create_or_get_tribe( tribe );
}

Tribe& World::tribe( e_tribe tribe ) {
  return natives().tribe_for( tribe );
}

Tribe& World::apache() { return tribe( e_tribe::apache ); }

Tribe& World::sioux() { return tribe( e_tribe::sioux ); }

Tribe& World::tupi() { return tribe( e_tribe::tupi ); }

Tribe& World::arawak() { return tribe( e_tribe::arawak ); }

Tribe& World::cherokee() { return tribe( e_tribe::cherokee ); }

Tribe& World::iroquois() { return tribe( e_tribe::iroquois ); }

Tribe& World::aztec() { return tribe( e_tribe::aztec ); }

Tribe& World::inca() { return tribe( e_tribe::inca ); }

// --------------------------------------------------------------
// Colony setup.
// --------------------------------------------------------------
void World::give_all_buildings( Colony& colony ) {
  for( e_colony_building b :
       refl::enum_values<e_colony_building> )
    colony.buildings[b] = true;
}

// ------------------------------------------------------------
// Market Prices.
// ------------------------------------------------------------
void World::set_current_bid_price( e_commodity type,
                                   int price_in_hundreds ) {
  Player& player = default_player();
  auto& comm_config =
      old_world( player ).market.commodities[type];
  comm_config.bid_price = price_in_hundreds;
}

void World::set_stable_bid_price( e_commodity type,
                                  int price_in_hundreds ) {
  CHECK( !is_in_processed_goods_price_group( type ),
         "cannot set the equilibrium price for goods in a price "
         "group." );
  Player& player = default_player();
  auto& comm_config =
      old_world( player ).market.commodities[type];
  comm_config.bid_price        = price_in_hundreds;
  comm_config.intrinsic_volume = 0;
}

void World::init_price_to_average( e_commodity type ) {
  auto const& limits =
      config_market.price_behavior[type].price_limits;
  int const price =
      ( limits.bid_price_min + limits.bid_price_max ) / 2;
  set_current_bid_price( type, price );
}

void World::init_prices_to_average() {
  for( e_commodity type : refl::enum_values<e_commodity> )
    init_price_to_average( type );
}

void World::set_tax_rate( int rate ) {
  old_world().taxes.tax_rate = rate;
}

// --------------------------------------------------------------
// Players.
// --------------------------------------------------------------
Player& World::dutch() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::dutch],
                    "dutch player does not exist." );
  return player;
}

Player& World::english() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::english],
                    "english player does not exist." );
  return player;
}

Player& World::spanish() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::spanish],
                    "spanish player does not exist." );
  return player;
}

Player& World::french() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::french],
                    "french player does not exist." );
  return player;
}

Player const& World::dutch() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::dutch],
                    ":: player does not exist." );
  return player;
}

Player const& World::english() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::english],
                    ":: player does not exist." );
  return player;
}

Player const& World::spanish() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::spanish],
                    ":: player does not exist." );
  return player;
}

Player const& World::french() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::french],
                    ":: player does not exist." );
  return player;
}

Player& World::ref_dutch() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::ref_dutch],
                    "ref_dutch player does not exist." );
  return player;
}

Player& World::ref_english() {
  UNWRAP_CHECK_MSG(
      player, root().players.players[e_player::ref_english],
      "ref_english player does not exist." );
  return player;
}

Player& World::ref_spanish() {
  UNWRAP_CHECK_MSG(
      player, root().players.players[e_player::ref_spanish],
      "ref_spanish player does not exist." );
  return player;
}

Player& World::ref_french() {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::ref_french],
                    "ref_french player does not exist." );
  return player;
}

Player const& World::ref_dutch() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::ref_dutch],
                    ":: player does not exist." );
  return player;
}

Player const& World::ref_english() const {
  UNWRAP_CHECK_MSG(
      player, root().players.players[e_player::ref_english],
      ":: player does not exist." );
  return player;
}

Player const& World::ref_spanish() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::spanish],
                    ":: player does not exist." );
  return player;
}

Player const& World::ref_french() const {
  UNWRAP_CHECK_MSG( player,
                    root().players.players[e_player::ref_french],
                    ":: player does not exist." );
  return player;
}

Player& World::default_player() {
  return player( default_player_type_ );
}

Player const& World::default_player() const {
  return player( default_player_type_ );
}

Player& World::player( maybe<e_player> player_type ) {
  UNWRAP_CHECK( player,
                root().players.players[player_type.value_or(
                    default_player_type_ )] );
  return player;
}

Player const& World::player(
    maybe<e_player> player_type ) const {
  UNWRAP_CHECK( player,
                root().players.players[player_type.value_or(
                    default_player_type_ )] );
  return player;
}

OldWorldState& World::old_world( Player const& player ) {
  return players().old_world[nation_for( player.type )];
}

OldWorldState& World::old_world(
    maybe<e_player> const player_type ) {
  CHECK( ss().players.players[this->player( player_type ).type]
             .has_value() );
  return old_world(
      *ss().players.players[this->player( player_type ).type] );
}

// --------------------------------------------------------------
// Revolution Status.
// --------------------------------------------------------------
void World::declare_independence( maybe<e_player> player_type ) {
  // The approach we take here is to just try to do as little as
  // possible to satisfy the validation methods, as opposed to
  // running through the real declaration process.
  Player& player = this->player( player_type );
  CHECK( !is_ref( player.type ) );
  player.revolution.status = e_revolution_status::declared;
  e_player const ref_player_type =
      ref_player_for( player.nation );
  if( !players().players[ref_player_type].has_value() ) {
    Player& ref_player =
        players().players[ref_player_type].emplace();
    ref_player.type   = ref_player_type;
    ref_player.nation = player.nation;
  }
}

base::valid_or<string> World::validate_colonies() const {
  for( auto const& [colony_id, colony] : colonies().all() ) {
    valid_or<string> res = colony.validate();
    if( !res.valid() )
      return fmt::format( "colony {} failed validation: {}",
                          colony_id, res.error() );
  }
  return base::valid;
}

void World::expensive_run_lua_init() {
  lua::state& st = lua();
  lua_init( engine(), st );
}

void World::initialize_ts() { ts(); }

World::World()
  : ss_( new SS ),
    ss_const_( new SSConst( *ss_ ) ),
    ss_saved_( new SS ),
    connectivity_( new TerrainConnectivity ),
    map_updater_(
        new NonRenderingMapUpdater( *ss_, *connectivity_ ) ),
    // These are left uninitialized until they are needed.
    uninitialized_planes_(),
    uninitialized_lua_(),
    uninitialized_gui_(),
    uninitialized_native_agents_(),
    uninitialized_ts_() {}

World::~World() noexcept = default;

rng::random& World::random() {
  if( uninitialized_random_ == nullptr )
    uninitialized_random_ = make_unique<rng::random>();
  return *uninitialized_random_;
}

} // namespace rn::testing
