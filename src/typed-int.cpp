/****************************************************************
**typed-int.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: A type safe integer for representing cartesian
*              coordinates and distances.
*
*****************************************************************/
#include "typed-int.hpp"

// luapp
#include "luapp/types.hpp"

// Cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"

using namespace std;

namespace rn {

LUA_TYPED_INT_IMPL( ::rn::X );
LUA_TYPED_INT_IMPL( ::rn::Y );
LUA_TYPED_INT_IMPL( ::rn::W );
LUA_TYPED_INT_IMPL( ::rn::H );

CDR_TYPED_INT_IMPL( ::rn::X );
CDR_TYPED_INT_IMPL( ::rn::Y );
CDR_TYPED_INT_IMPL( ::rn::W );
CDR_TYPED_INT_IMPL( ::rn::H );

} // namespace rn
