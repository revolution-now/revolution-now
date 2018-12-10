/****************************************************************
* Main config file
*****************************************************************/
CFG( rn,
  FLD( int, one )
  FLD( Str, two )
  FLD( int, hello )
  OBJ( fruit,
    FLD( int,         apples )
    FLD( Vec<Str>,    oranges )
    FLD( Opt<Vec<X>>, grapes )
    FLD( Str,         description )
    OBJ( hello,
      FLD( Opt<int>, world )
    )
  )
)

/****************************************************************
* GUI Config File
*****************************************************************/
CFG( window,
  FLD( Str,    game_title )
  FLD( double, game_version )
  LNK( grapes, rn.fruit.grapes )
  FLD( Coord,  coordinates )
  OBJ( window_error,
    FLD( Str,        title )
    FLD( bool,       show )
    FLD( rn::X,      x_size )
    FLD( Vec<Coord>, positions )
    LNK( world,      rn.fruit.hello.world )
  )
  FLD( Vec<rn::W>, widths )
  LNK( fruit,      rn.fruit )
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
