/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Save-game state for terrain data.
*
*****************************************************************/
#include "terrain.hpp"

// ss
#include "ss/fog-square.hpp"
#include "ss/map-square.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

using ::gfx::point;
using ::gfx::rect;

/****************************************************************
** TerrainState
*****************************************************************/
base::valid_or<std::string> wrapped::TerrainState::validate()
    const {
  REFL_VALIDATE( int( pacific_ocean_endpoints.size() ) ==
                     real_terrain.map.size().h,
                 "the pacific_ocean_endpoints array must have "
                 "one element for each row in the map." );
  return base::valid;
}

base::valid_or<std::string> TerrainState::validate() const {
  return base::valid;
}

void TerrainState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

TerrainState::TerrainState( wrapped::TerrainState&& o )
  : o_( std::move( o ) ) {
  // Populate transient state.
  // none.
}

TerrainState::TerrainState()
  : TerrainState( wrapped::TerrainState{} ) {
  validate_or_die();
}

gfx::Matrix<MapSquare> const& TerrainState::world_map() const {
  return o_.real_terrain.map;
}

void TerrainState::modify_entire_map(
    base::function_ref<void( RealTerrain& )> mutator ) {
  mutator( o_.real_terrain );
  // Maintain the invariant that pacific_ocean_tiles should
  // have one element for each map row.
  o_.pacific_ocean_endpoints.resize(
      o_.real_terrain.map.size().h );
}

Delta TerrainState::world_size_tiles() const {
  return o_.real_terrain.map.size();
}

Rect TerrainState::world_rect_tiles() const {
  return { .x = 0,
           .y = 0,
           .w = world_size_tiles().w,
           .h = world_size_tiles().h };
}

bool TerrainState::square_exists( point const tile ) const {
  if( tile.x < 0 || tile.y < 0 ) return false;
  return tile.is_inside(
      rect{ .origin = {}, .size = world_map().size() } );
}

bool TerrainState::square_exists( Coord coord ) const {
  return square_exists( coord.to_gfx() );
}

base::maybe<MapSquare&> TerrainState::mutable_maybe_square_at(
    point const tile ) {
  if( !square_exists( tile ) ) return base::nothing;
  return o_.real_terrain.map[tile.y][tile.x];
}

base::maybe<MapSquare&> TerrainState::mutable_maybe_square_at(
    Coord coord ) {
  return mutable_maybe_square_at( coord.to_gfx() );
}

base::maybe<PlayerTerrain const&> TerrainState::player_terrain(
    e_player player ) const {
  return o_.player_terrain[player];
}

PlayerTerrain& TerrainState::mutable_player_terrain(
    e_player player ) {
  UNWRAP_CHECK( res, o_.player_terrain[player] );
  return res;
}

base::maybe<MapSquare const&> TerrainState::maybe_square_at(
    point const tile ) const {
  if( !square_exists( tile ) ) return base::nothing;
  return o_.real_terrain.map[tile.y][tile.x];
}

base::maybe<MapSquare const&> TerrainState::maybe_square_at(
    Coord coord ) const {
  return maybe_square_at( coord.to_gfx() );
}

MapSquare const& TerrainState::total_square_at(
    point const tile ) const {
  base::maybe<e_cardinal_direction> d =
      proto_square_direction_for_tile( tile );
  if( d.has_value() ) return proto_square( *d );
  // This should never fail since coord should now be on the map.
  return square_at( tile );
}

MapSquare const& TerrainState::total_square_at(
    Coord coord ) const {
  return total_square_at( coord.to_gfx() );
}

base::maybe<e_cardinal_direction>
TerrainState::proto_square_direction_for_tile(
    point const tile ) const {
  Rect rect = world_rect_tiles();
  if( tile.x < rect.left_edge() ) return e_cardinal_direction::w;
  if( tile.y < rect.top_edge() ) //
    return e_cardinal_direction::n;
  if( tile.x >= rect.right_edge() )
    return e_cardinal_direction::e;
  if( tile.y >= rect.bottom_edge() )
    return e_cardinal_direction::s;
  return base::nothing;
}

base::maybe<e_cardinal_direction>
TerrainState::proto_square_direction_for_tile(
    Coord coord ) const {
  return proto_square_direction_for_tile( coord.to_gfx() );
}

MapSquare const& TerrainState::proto_square(
    e_cardinal_direction d ) const {
  return o_.proto_squares[d];
}

MapSquare& TerrainState::mutable_proto_square(
    e_cardinal_direction d ) {
  return o_.proto_squares[d];
}

MapSquare const& TerrainState::square_at(
    point const tile ) const {
  base::maybe<MapSquare const&> res = maybe_square_at( tile );
  CHECK( res, "square {} does not exist!", tile );
  return *res;
}

MapSquare const& TerrainState::square_at( Coord coord ) const {
  return square_at( coord.to_gfx() );
}

MapSquare& TerrainState::mutable_square_at( point const tile ) {
  base::maybe<MapSquare&> res = mutable_maybe_square_at( tile );
  CHECK( res, "square {} does not exist!", tile );
  return *res;
}

MapSquare& TerrainState::mutable_square_at( Coord coord ) {
  return mutable_square_at( coord.to_gfx() );
}

bool TerrainState::is_land( Coord coord ) const {
  return square_at( coord ).surface == e_surface::land;
}

void TerrainState::initialize_player_terrain( e_player player,
                                              bool visible ) {
  if( !o_.player_terrain[player].has_value() )
    o_.player_terrain[player].emplace();
  auto& map = o_.player_terrain[player]->map;
  map.reset( o_.real_terrain.map.size() );
  if( !visible ) return;
  auto const& world_map = o_.real_terrain.map;
  for( Rect const tile :
       gfx::subrects( o_.real_terrain.map.rect() ) )
    map[tile.upper_left()] = PlayerSquare::explored{
      .fog_status = FogStatus::fogged{
        .contents = { .square =
                          world_map[tile.upper_left()] } } };
}

void TerrainState::testing_reset_player_terrain(
    e_player const player ) {
  o_.player_terrain[player].reset();
}

bool TerrainState::is_pacific_ocean( Coord coord ) const {
  CHECK_GE( coord.y, 0 );
  CHECK_LT( coord.y, o_.real_terrain.map.size().h );
  // Recall that the endpoint is one-past-the-end.
  return ( coord.x < o_.pacific_ocean_endpoints[coord.y] );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // TerrainState.
  [&] {
    using U = ::rn::TerrainState;

    auto u = st.usertype.create<U>();

    u["placement_seed"]     = &U::placement_seed;
    u["set_placement_seed"] = &U::set_placement_seed;
    u["size"]               = &U::world_size_tiles;
    u["square_exists"]      = []( U& o, Coord const tile ) {
      return o.square_exists( tile );
    };
    u["square_at"] = []( U& o, Coord const tile ) -> MapSquare& {
      return o.mutable_square_at( tile );
    };
    u["proto_square"] = &U::mutable_proto_square;
    u["initialize_player_terrain"] =
        &U::initialize_player_terrain;

    u["reset"] = []( U& o, Delta size ) {
      o.modify_entire_map( [&]( RealTerrain& real_terrain ) {
        real_terrain.map = gfx::Matrix<MapSquare>( size );
      } );
    };

    u["pacific_ocean_endpoint"] = [&]( U& o, int row ) {
      LUA_CHECK( st,
                 row < int( o.pacific_ocean_endpoints().size() ),
                 "row {} is out of bounds.", row );
      return o.pacific_ocean_endpoints()[row];
    };
    u["set_pacific_ocean_endpoint"] = [&]( U& o, int row,
                                           int endpoint ) {
      LUA_CHECK( st,
                 row < int( o.pacific_ocean_endpoints().size() ),
                 "row {} is out of bounds.", row );
      o.pacific_ocean_endpoints()[row] = endpoint;
    };
  }();
};

} // namespace

} // namespace rn
