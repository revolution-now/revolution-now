/****************************************************************
* Colors
*****************************************************************/
#include "../config/palette.inl"

/****************************************************************
* Main config file
*****************************************************************/
CFG( rn,
  FLD( e_nation, player_nation )
  FLD( int, target_frame_rate )
  OBJ( main_window,
    FLD( Str, title )
  )
)

/****************************************************************
* GUI Config File
*****************************************************************/
CFG( ui,
  OBJ( window,
    FLD( int, border_width )
    FLD( Color, border_color )
  )

  FLD( Str,    game_title )
  FLD( double, game_version )
  FLD( Coord,  coordinates )
  OBJ( window_error,
    FLD( Str,        title )
    FLD( bool,       show )
    FLD( rn::X,      x_size )
    FLD( Vec<Coord>, positions )
  )
  FLD( Vec<rn::W>, widths )
)

/****************************************************************
* Art Config File
*****************************************************************/
CFG( art,
  FLD( Str, tiles_png )

  OBJ( tiles,
    FLD( Coord, water )
    FLD( Coord, land )
    FLD( Coord, land_1_side )
    FLD( Coord, land_2_sides )
    FLD( Coord, land_3_sides )
    FLD( Coord, land_4_sides )
    FLD( Coord, land_corner )

    FLD( Coord, fog )
    FLD( Coord, fog_1_side )
    FLD( Coord, fog_corner )

    FLD( Coord, terrain_grass )

    FLD( Coord, panel )
    FLD( Coord, panel_edge_left )
    FLD( Coord, panel_slate )
    FLD( Coord, panel_slate_1_side )
    FLD( Coord, panel_slate_2_sides )

    FLD( Coord, free_colonist )
    FLD( Coord, caravel )
    FLD( Coord, soldier )
  )
)

/****************************************************************
* Units Config File
*****************************************************************/
#define UNIT_SCHEMA( __unit )             \
  OBJ( __unit,                            \
    FLD( Str,      name                 ) \
    FLD( bool,     boat                 ) \
    FLD( int,      visibility           ) \
    FLD( MvPoints, movement_points      ) \
    FLD( bool,     can_attack           ) \
    FLD( int,      attack_points        ) \
    FLD( int,      defense_points       ) \
    FLD( int,      cargo_slots          ) \
    FLD( Opt<int>, cargo_slots_occupies ) \
  )

CFG( units,
  UNIT_SCHEMA( free_colonist )
  UNIT_SCHEMA( caravel       )
  UNIT_SCHEMA( soldier       )
)

/****************************************************************
* Nation Config File
*****************************************************************/
CFG( nation,
  OBJ( dutch,
    FLD( Str, country_name )
    LNK( flag_color, palette.orange.sat2.lum9 )
  )
  OBJ( french,
    FLD( Str, country_name )
    LNK( flag_color, palette.blue.sat2.lum6 )
  )
  OBJ( english,
    FLD( Str, country_name )
    LNK( flag_color, palette.red.sat2.lum7 )
  )
  OBJ( spanish,
    FLD( Str, country_name )
    LNK( flag_color, palette.yellow.sat2.lum13 )
  )
)
