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
  FLD( int, depixelate_pixels_per_frame )
  OBJ( main_window,
    FLD( Str, title )
  )
  OBJ( viewport,
    FLD( double, pan_speed )
    FLD( double, zoom_min )
    FLD( double, zoom_speed )
    FLD( double, zoom_accel_coeff )
    FLD( double, zoom_accel_drag_coeff )
    FLD( double, pan_accel_init_coeff )
    FLD( double, pan_accel_drag_init_coeff )
  )
  OBJ( controls,
    FLD( int, drag_buffer )
  )
)

/****************************************************************
* GUI Config File
*****************************************************************/
CFG( ui,
  OBJ( window,
    FLD( int, border_width )
    FLD( int, window_padding )
    FLD( Color, border_color )
    FLD( int, ui_padding )
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

  OBJ( menus,
    FLD( rn::W, first_menu_start )
    FLD( rn::W, padding )
    FLD( rn::W, spacing )
    FLD( rn::W, body_min_width )
  )
)

/****************************************************************
* Art Config File
*****************************************************************/
CFG( art,
  OBJ( images,
    FLD( fs::path, old_world )
  )
  OBJ( tiles,

    OBJ( world,
      FLD( fs::path, img )
      OBJ( coords,
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
      )
    )

    OBJ( wood,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, wood_middle )
        FLD( Coord, wood_left_edge )
      )
    )

    OBJ( units,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, caravel )
        FLD( Coord, privateer )
        FLD( Coord, free_colonist )
        FLD( Coord, soldier )
      )
    )

    OBJ( menu,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, menu_top_left )
        FLD( Coord, menu_body )
        FLD( Coord, menu_top )
        FLD( Coord, menu_left )
        FLD( Coord, menu_bottom )
        FLD( Coord, menu_bottom_left )
        FLD( Coord, menu_right )
        FLD( Coord, menu_top_right )
        FLD( Coord, menu_bottom_right )
        FLD( Coord, menu_sel_body )
        FLD( Coord, menu_sel_left )
        FLD( Coord, menu_sel_right )
      )
    )

    OBJ( menu16,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, menu_bar_0 )
        FLD( Coord, menu_bar_1 )
        FLD( Coord, menu_bar_2 )
      )
    )

    OBJ( menu_sel,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, menu_item_sel_back )
        FLD( Coord, menu_hdr_sel_back )
      )
    )

    OBJ( button,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, button_up_ul )
        FLD( Coord, button_up_um )
        FLD( Coord, button_up_ur )
        FLD( Coord, button_up_ml )
        FLD( Coord, button_up_mm )
        FLD( Coord, button_up_mr )
        FLD( Coord, button_up_ll )
        FLD( Coord, button_up_lm )
        FLD( Coord, button_up_lr )
        FLD( Coord, button_down_ul )
        FLD( Coord, button_down_um )
        FLD( Coord, button_down_ur )
        FLD( Coord, button_down_ml )
        FLD( Coord, button_down_mm )
        FLD( Coord, button_down_mr )
        FLD( Coord, button_down_ll )
        FLD( Coord, button_down_lm )
        FLD( Coord, button_down_lr )
      )
    )
  )
)

/****************************************************************
* Units Config File
*****************************************************************/
#define UNIT_SCHEMA( __unit )                       \
  OBJ( __unit,                                      \
    FLD( Str,              name                   ) \
    FLD( bool,             boat                   ) \
    FLD( bool,             nat_icon_front         ) \
    FLD( e_direction,      nat_icon_position      ) \
    FLD( int,              visibility             ) \
    FLD( MvPoints,         movement_points        ) \
    FLD( int,              attack_points          ) \
    FLD( int,              defense_points         ) \
    FLD( e_unit_death,     on_death               ) \
    FLD( Opt<e_unit_type>, demoted                ) \
    FLD( int,              cargo_slots            ) \
    FLD( Opt<int>,         cargo_slots_occupies   ) \
                                                  )

CFG( units,
  UNIT_SCHEMA( caravel       )
  UNIT_SCHEMA( privateer     )
  UNIT_SCHEMA( free_colonist )
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

/****************************************************************
* Sound
*****************************************************************/
CFG( sound,
  OBJ( general,
    FLD( int, channels )
    FLD( int, frequency )
    FLD( int, chunk_size )
  )
  OBJ( sfx,
    FLD( Str, move )
    FLD( Str, attacker_lost )
    FLD( Str, attacker_won )

    OBJ( volume,
      FLD( int, move )
      FLD( int, attacker_lost )
      FLD( int, attacker_won )
    )
  )
)

/****************************************************************
* Music
*****************************************************************/
// This is to avoid commas in macro arguments.
using SpecialMusicEventMap = FlatMap<e_special_music_event, Str>;

CFG( music,
  FLD( fs::path, midi_folder )

  FLD( e_music_player, first_choice_music_player )
  FLD( e_music_player, second_choice_music_player )

  FLD( Seconds, threshold_previous_tune_secs )

  FLD( Vec<Tune>, tunes )

  FLD( SpecialMusicEventMap, special_event_tunes )

  FLD( bool, autoplay )
  FLD( double, initial_volume )
)
