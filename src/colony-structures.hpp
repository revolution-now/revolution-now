/****************************************************************
**colony-structures.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-18.
*
* Description: All things related to the buildings in colonies.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"

namespace rn {

enum class e_( colony_building,
               // Free buildings.
               blacksmiths_house,    //
               carpenters_shop,      //
               fur_traders_house,    //
               rum_distillers_house, //
               tobacconists_house,   //
               town_hall,            //
               weavers_house,        //

               // Level 1 buildings.
               armory,         //
               docks,          //
               printing_press, //
               schoolhouse,    //
               stable,         //
               stockade,       //
               warehouse,      //

               // Level 2 buildings.
               blacksmiths_shop,    //
               church,              //
               college,             //
               drydock,             //
               fort,                //
               fur_trading_post,    //
               lumber_mill,         //
               magazine,            //
               newspaper,           //
               rum_distillery,      //
               tobacconists_shop,   //
               warehouse_expansion, //
               weavers_shop,        //

               // Level 3 buildings.
               arsenal,       //
               cathedral,     //
               cigar_factory, //
               fortress,      //
               fur_factory,   //
               iron_works,    //
               rum_factory,   //
               shipyard,      //
               textile_mill,  //
               university,    //

               // Special buildings.
               custom_house //
);

} // namespace rn
