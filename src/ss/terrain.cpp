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

/****************************************************************
** TerrainState
*****************************************************************/
base::valid_or<std::string> wrapped::TerrainState::validate()
    const {
  REFL_VALIDATE( int( pacific_ocean_endpoints.size() ) ==
                     world_map.size().h,
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

Matrix<MapSquare> const& TerrainState::world_map() const {
  return o_.world_map;
}

void TerrainState::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  mutator( o_.world_map );
  // Maintain the invariant that pacific_ocean_tiles should
  // have one element for each map row.
  o_.pacific_ocean_endpoints.resize( o_.world_map.size().h );
}

Delta TerrainState::world_size_tiles() const {
  return o_.world_map.size();
}

Rect TerrainState::world_rect_tiles() const {
  return { .x = 0,
           .y = 0,
           .w = world_size_tiles().w,
           .h = world_size_tiles().h };
}

bool TerrainState::square_exists( Coord coord ) const {
  if( coord.x < 0 || coord.y < 0 ) return false;
  return coord.is_inside(
      Rect::from( Coord{}, world_map().size() ) );
}

base::maybe<MapSquare&> TerrainState::mutable_maybe_square_at(
    Coord coord ) {
  if( !square_exists( coord ) ) return base::nothing;
  return o_.world_map[coord.y][coord.x];
}

base::maybe<PlayerTerrain const&> TerrainState::player_terrain(
    e_nation nation ) const {
  return o_.player_terrain[nation];
}

PlayerTerrain& TerrainState::mutable_player_terrain(
    e_nation nation ) {
  UNWRAP_CHECK( res, o_.player_terrain[nation] );
  return res;
}

base::maybe<MapSquare const&> TerrainState::maybe_square_at(
    Coord coord ) const {
  if( !square_exists( coord ) ) return base::nothing;
  return o_.world_map[coord.y][coord.x];
}

MapSquare const& TerrainState::total_square_at(
    Coord coord ) const {
  base::maybe<e_cardinal_direction> d =
      proto_square_direction_for_tile( coord );
  if( d.has_value() ) return proto_square( *d );
  // This should never fail since coord should now be on the map.
  return square_at( coord );
}

base::maybe<e_cardinal_direction>
TerrainState::proto_square_direction_for_tile(
    Coord coord ) const {
  Rect rect = world_rect_tiles();
  if( coord.x < rect.left_edge() )
    return e_cardinal_direction::w;
  if( coord.y < rect.top_edge() ) //
    return e_cardinal_direction::n;
  if( coord.x >= rect.right_edge() )
    return e_cardinal_direction::e;
  if( coord.y >= rect.bottom_edge() )
    return e_cardinal_direction::s;
  return base::nothing;
}

MapSquare const& TerrainState::proto_square(
    e_cardinal_direction d ) const {
  return o_.proto_squares[d];
}

MapSquare& TerrainState::mutable_proto_square(
    e_cardinal_direction d ) {
  return o_.proto_squares[d];
}

MapSquare const& TerrainState::square_at( Coord coord ) const {
  base::maybe<MapSquare const&> res = maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

MapSquare& TerrainState::mutable_square_at( Coord coord ) {
  base::maybe<MapSquare&> res = mutable_maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

bool TerrainState::is_land( Coord coord ) const {
  return square_at( coord ).surface == e_surface::land;
}

void TerrainState::initialize_player_terrain( e_nation nation,
                                              bool visible ) {
  if( !o_.player_terrain[nation].has_value() )
    o_.player_terrain[nation].emplace();
  Matrix<base::maybe<FogSquare>>& map =
      o_.player_terrain[nation]->map;
  map = Matrix<base::maybe<FogSquare>>( o_.world_map.size() );
  if( visible ) {
    Matrix<MapSquare> const& world_map = o_.world_map;
    for( Rect const tile :
         gfx::subrects( o_.world_map.rect() ) ) {
      map[tile.upper_left()].emplace();
      map[tile.upper_left()]->square =
          world_map[tile.upper_left()];
    }
  }
}

bool TerrainState::is_pacific_ocean( Coord coord ) const {
  CHECK_GE( coord.y, 0 );
  CHECK_LT( coord.y, o_.world_map.size().h );
  // Recall that the endpoint is one-past-the-end.
  return ( coord.x < o_.pacific_ocean_endpoints[coord.y] );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // PlayerTerrainMatrix.
  [&] {
    using U = ::rn::PlayerTerrainMatrix;

    auto u = st.usertype.create<U>();

    u["size"]          = &U::size;
    u["square_exists"] = []( U& o, Coord tile ) {
      return tile.is_inside( o.rect() );
    };
    u["square_at"] =
        []( U& o, Coord tile ) -> base::maybe<FogSquare&> {
      if( !tile.is_inside( o.rect() ) ) return base::nothing;
      return o[tile];
    };
  }();

  // PlayerTerrain.
  [&] {
    using U = ::rn::PlayerTerrain;
    auto u  = st.usertype.create<U>();

    u["map"] = &U::map;
  }();

  // PlayerTerrainMap.
  [&] {
    using U = ::rn::PlayerTerrainMap;
    auto u  = st.usertype.create<U>();

    u["for_nation"] = [&]( U& obj, e_nation c )
        -> base::maybe<PlayerTerrain&> { return obj[c]; };

    u["add_nation"] = [&]( U&       obj,
                           e_nation c ) -> PlayerTerrain& {
      if( !obj[c].has_value() ) obj[c].emplace();
      return *obj[c];
    };
  }();

  // ProtoSquaresMap.
  // TODO: make this generic.
  [&] {
    using U = ::rn::ProtoSquaresMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_cardinal_direction c ) -> MapSquare& {
      return obj[c];
    };
  }();

  // TerrainState.
  [&] {
    using U = ::rn::TerrainState;

    auto u = st.usertype.create<U>();

    u["placement_seed"]     = &U::placement_seed;
    u["set_placement_seed"] = &U::set_placement_seed;
    u["size"]               = &U::world_size_tiles;
    u["square_exists"]      = &U::square_exists;
    u["square_at"]          = &U::mutable_square_at;
    u["proto_square"]       = &U::mutable_proto_square;
    u["initialize_player_terrain"] =
        &U::initialize_player_terrain;

    u["reset"] = []( U& o, Delta size ) {
      o.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
        m = Matrix<MapSquare>( size );
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
