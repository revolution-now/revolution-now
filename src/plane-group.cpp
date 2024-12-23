/****************************************************************
**plane-group.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-17.
*
* Description: An IPlaneGroup implementation structured so as to
*              suit typically plane layouts in the game.
*
*****************************************************************/
#include "plane-group.hpp"

// base
#include "base/meta.hpp"

using namespace std;

namespace rn {

/****************************************************************
** PlaneGroup
*****************************************************************/
void PlaneGroup::set_bottom( IPlane& p ATTR_LIFETIMEBOUND ) {
  bottom = &p;
}

IPlane* PlaneGroup::get_bottom() const {
  IPlane* const* res = std::get_if<IPlane*>( &bottom );
  CHECK( res );
  return *res;
}

vector<IPlane*> PlaneGroup::planes() const {
  vector<IPlane*> res;
  res.reserve( 10 );

  auto add = mp::overload{
    [&]( IPlane* p ) {
      if( p ) res.push_back( p );
    },
    [&]( auto const& p ) {
      if( p ) res.push_back( &p.untyped() );
    },
  };

  switch( bottom.index() ) {
    case 0:
      add( get<0>( bottom ) );
      break;
    case 1:
      add( get<1>( bottom ) );
      break;
  }

  add( panel );
  if( menus_enabled ) add( menu );

  add( window );
  add( console );
  add( omni );

  return res;
}

} // namespace rn
