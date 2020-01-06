/****************************************************************
* Units Config File
*****************************************************************/
#ifndef UNITS_INL
#define UNITS_INL

namespace rn {

#define UNIT_SCHEMA( __unit )                       \
  OBJ( __unit,                                      \
    FLD( Str,              name                   ) \
    FLD( bool,             ship                   ) \
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
  UNIT_SCHEMA( merchantman    )
  UNIT_SCHEMA( privateer      )
  UNIT_SCHEMA( free_colonist  )
  UNIT_SCHEMA( soldier        )
  UNIT_SCHEMA( large_treasure )
  UNIT_SCHEMA( small_treasure )
)

}

#endif
