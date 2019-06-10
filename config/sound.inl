/****************************************************************
* Sound
*****************************************************************/
#ifndef SOUND_INL
#define SOUND_INL

namespace rn {

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

}

#endif
