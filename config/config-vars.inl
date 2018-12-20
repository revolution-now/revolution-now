/****************************************************************
* Colors
*****************************************************************/
#define SATURATION( n )    \
    OBJ( sat ## n,         \
      FLD( Color, light0 ) \
      FLD( Color, light1 ) \
      FLD( Color, light2 ) \
      FLD( Color, light3 ) \
      FLD( Color, light4 ) \
      FLD( Color, light5 ) \
      FLD( Color, light6 ) \
      FLD( Color, light7 ) \
      FLD( Color, light8 ) \
      FLD( Color, light9 ) \
    )

#define HUE( name )   \
    OBJ( name,        \
      SATURATION( 0 ) \
      SATURATION( 1 ) \
      SATURATION( 2 ) \
    )

//CFG( palette,
//  HUE( red )
//  HUE( orange )
//  HUE( yellow )
//  HUE( chartreuse_green )
//  HUE( green )
//  HUE( spring_green )
//  HUE( cyan )
//  HUE( azure )
//  HUE( blue )
//  HUE( violet )
//  HUE( magenta )
//  HUE( rose )
//)

/****************************************************************
* Main config file
*****************************************************************/
CFG( rn,
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
)
