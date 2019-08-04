/****************************************************************
* Art Config File
*****************************************************************/
#ifndef ART_INL
#define ART_INL

namespace rn {

CFG( art,
  OBJ( images,
    FLD( fs::path, europe )
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
        FLD( Coord, large_treasure )
        FLD( Coord, small_treasure )
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
    OBJ( commodities,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, commodity_food )
        FLD( Coord, commodity_sugar )
        FLD( Coord, commodity_tobacco )
        FLD( Coord, commodity_cotton )
        FLD( Coord, commodity_fur )
        FLD( Coord, commodity_lumber )
        FLD( Coord, commodity_ore )
        FLD( Coord, commodity_silver )
        FLD( Coord, commodity_horses )
        FLD( Coord, commodity_rum )
        FLD( Coord, commodity_cigars )
        FLD( Coord, commodity_cloth )
        FLD( Coord, commodity_coats )
        FLD( Coord, commodity_trade_goods )
        FLD( Coord, commodity_tools )
        FLD( Coord, commodity_muskets )
      )
    )

    OBJ( testing,
      FLD( fs::path, img )
      OBJ( coords,
        FLD( Coord, checkers )
        FLD( Coord, checkers_inv )
      )
    )
  )
)

}

#endif
