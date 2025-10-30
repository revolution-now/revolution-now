/****************************************************************
**ext-usertype.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-30.
*
* Description: Customization point for doing usertype setup on
*              types.
*
*****************************************************************/
#pragma once

// luapp
#include "ext.hpp"

namespace lua {

struct state;

/****************************************************************
** Concept: DefinesUsertype
*****************************************************************/
template<typename T>
concept DefinesUsertype = requires( state& st ) {
  {
    define_usertype_for( st, ::lua::tag<T>{} )
  } -> std::same_as<void>;
};

} // namespace lua
