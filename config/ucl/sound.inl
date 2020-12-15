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
    FLD( std::string, move )
    FLD( std::string, attacker_lost )
    FLD( std::string, attacker_won )

    OBJ( volume,
      FLD( int, move )
      FLD( int, attacker_lost )
      FLD( int, attacker_won )
    )
  )
)

}

#endif
