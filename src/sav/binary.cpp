/****************************************************************
**binary.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-01.
*
* Description: API for loading and saving games in the classic
*              format of the OG.
*
*****************************************************************/
#include "binary.hpp"

// sav
#include "error.hpp"
#include "sav-struct.hpp"

// base
#include "base/binary-data.hpp"

using namespace std;

namespace sav {

namespace {

using ::base::BinaryBuffer;
using ::base::BinaryData;
using ::base::valid;
using ::base::valid_or;

valid_or<string> sanity_check_count( string_view label,
                                     int count, int max ) {
  if( count > max )
    return fmt::format(
        "the {} count has value {} which failed the sanity "
        "check since it is much larger than expected.",
        label, count );
  return valid;
}

template<typename T>
valid_or<string> read_vector( BinaryData& b, string_view label,
                              int count, int sanity_max,
                              vector<T>& out ) {
  HAS_VALUE_OR_RET(
      sanity_check_count( label, count, sanity_max ) );
  out.resize( count );
  for( int i = 0; i < count; ++i )
    if( !read_binary( b, out[i] ) )
      return fmt::format( "while reading {}[{}].", label, i );
  return valid;
}

template<typename T>
valid_or<string> write_vector( BinaryData& b, string_view label,
                               vector<T> const& in ) {
  for( int i = 0; i < int( in.size() ); ++i )
    if( !write_binary( b, in[i] ) )
      return fmt::format( "while writing {}[{}].", label, i );
  return valid;
}

valid_or<string> read( BinaryData& b, ColonySAV& out ) {
  if( !read_binary( b, out.head ) )
    return fmt::format( "while reading header." );
  if( !read_binary( b, out.player ) )
    return fmt::format( "while reading player array." );
  if( !read_binary( b, out.other ) )
    return fmt::format( "while reading 'other' section." );

  // The OG only has ~4k tiles on the board, so if it is larger
  // than 5k then something is definitely wrong. Actually, some
  // reports say that the OG only allows 38 colonies per player,
  // so this should be more than enough.
  HAS_VALUE_OR_RET( read_vector(
      b, "colony", out.head.colony_count, 5000, out.colony ) );

  // The OG apparently has a limit of 256 units on the map (per
  // player?), which is what this section holds. But we'll be a
  // bit more generous. Note that this section only includes
  // units on the map.
  HAS_VALUE_OR_RET( read_vector( b, "unit", out.head.unit_count,
                                 10000, out.unit ) );

  if( !read_binary( b, out.nation ) )
    return fmt::format( "while reading nation array." );

  // FIXME: the sav structure document uses the word "tribe"
  // when it should use "dwelling".
  HAS_VALUE_OR_RET( read_vector( b, "dwelling",
                                 out.head.tribe_count, 8 * 200,
                                 out.tribe ) );

  if( !read_binary( b, out.indian ) )
    return fmt::format( "while reading tribe array." );
  if( !read_binary( b, out.stuff ) )
    return fmt::format( "while reading 'stuff' section." );

  int tile_count = out.head.map_size_x * out.head.map_size_y;
  if( tile_count == 0 )
    return fmt::format( "map size is zero." );
  // These are the (fixed) size of the map in the OG. Note that
  // they include a "border" of tiles, one tile thick, that are
  // not visible on the map in the game.
  int const kOGMapX = 58;
  int const kOGMapY = 72;
  // In the sanity checks below we will allow maps that are
  // smaller than this since we will use those in unit tests,
  // but we will not allow maps larger, since the OG has various
  // mechanisms that won't really work with arbitrary map sizes.
  int const kMaxTiles = kOGMapX * kOGMapY;

  if( tile_count > kMaxTiles )
    return fmt::format(
        "the map has {} tiles which exceeds the size of the "
        "original game's map, which is {}x{} tiles.",
        tile_count, kOGMapX, kOGMapY );

  HAS_VALUE_OR_RET( read_vector( b, "tile", tile_count,
                                 kMaxTiles, out.tile ) );
  HAS_VALUE_OR_RET( read_vector( b, "mask", tile_count,
                                 kMaxTiles, out.mask ) );
  HAS_VALUE_OR_RET( read_vector( b, "path", tile_count,
                                 kMaxTiles, out.path ) );
  HAS_VALUE_OR_RET( read_vector( b, "seen", tile_count,
                                 kMaxTiles, out.seen ) );

  if( !read_binary( b, out.unknown_map38a ) )
    return fmt::format(
        "while reading unknown_map38a section." );
  if( !read_binary( b, out.unknown_map38b ) )
    return fmt::format(
        "while reading unknown_map38b section." );
  if( !read_binary( b, out.unknown39a ) )
    return fmt::format( "while reading unknown39a section." );
  if( !read_binary( b, out.unknown39b ) )
    return fmt::format( "while reading unknown39b section." );
  if( !read_binary( b, out.prime_resource_seed ) )
    return fmt::format( "while reading prime resource seed." );
  if( !read_binary( b, out.unknown39d ) )
    return fmt::format( "while reading unknown39d section." );
  if( !read_binary( b, out.trade_route ) )
    return fmt::format( "while reading trade route array." );

  return valid;
}

valid_or<string> write( BinaryData& b, ColonySAV const& out ) {
  if( !write_binary( b, out.head ) )
    return fmt::format( "while writing header." );
  if( !write_binary( b, out.player ) )
    return fmt::format( "while writing player array." );
  if( !write_binary( b, out.other ) )
    return fmt::format( "while writing 'other' section." );

  HAS_VALUE_OR_RET( write_vector( b, "colony", out.colony ) );

  HAS_VALUE_OR_RET( write_vector( b, "unit", out.unit ) );

  if( !write_binary( b, out.nation ) )
    return fmt::format( "while writing nation array." );

  // FIXME: the sav structure document uses the word "tribe"
  // when it should use "dwelling".
  HAS_VALUE_OR_RET( write_vector( b, "dwelling", out.tribe ) );

  if( !write_binary( b, out.indian ) )
    return fmt::format( "while writing tribe array." );
  if( !write_binary( b, out.stuff ) )
    return fmt::format( "while writing 'stuff' section." );

  HAS_VALUE_OR_RET( write_vector( b, "tile", out.tile ) );
  HAS_VALUE_OR_RET( write_vector( b, "mask", out.mask ) );
  HAS_VALUE_OR_RET( write_vector( b, "path", out.path ) );
  HAS_VALUE_OR_RET( write_vector( b, "seen", out.seen ) );

  if( !write_binary( b, out.unknown_map38a ) )
    return fmt::format(
        "while writing unknown_map38a section." );
  if( !write_binary( b, out.unknown_map38b ) )
    return fmt::format(
        "while writing unknown_map38b section." );
  if( !write_binary( b, out.unknown39a ) )
    return fmt::format( "while writing unknown39a section." );
  if( !write_binary( b, out.unknown39b ) )
    return fmt::format( "while writing unknown39b section." );
  if( !write_binary( b, out.prime_resource_seed ) )
    return fmt::format( "while writing prime resource seed." );
  if( !write_binary( b, out.unknown39d ) )
    return fmt::format( "while writing unknown39d section." );
  if( !write_binary( b, out.trade_route ) )
    return fmt::format( "while writing trade route array." );

  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> load_binary( string const& path,
                              ColonySAV&    out ) {
  UNWRAP_RETURN( buffer, BinaryBuffer::from_file( path ) );
  base::BinaryData data( buffer );
  if( auto res = read( data, out ); !res )
    return fmt::format(
        "failed while reading save file data at offset {}: {}",
        data.pos(), res.error() );

  if( data.pos() < buffer.size() )
    return fmt::format(
        "{} unexpected residual bytes at end of file.",
        buffer.size() - data.pos() );

  return valid;
}

valid_or<string> save_binary( string const&    path,
                              ColonySAV const& in ) {
  // This should be more than enough. The OG's save files are
  // typically no more than 20-40k.
  BinaryBuffer buffer( 200 * 1024 );

  base::BinaryData data( buffer );
  if( !write( data, in ) )
    return fmt::format(
        "failed to write save data to buffer at offset {}.",
        data.pos() );

  HAS_VALUE_OR_RET( buffer.write_file( path, data.pos() ) );
  return valid;
}

} // namespace sav
