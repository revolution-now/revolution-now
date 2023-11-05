/****************************************************************
** Classic Colonization Save File Structure.
*****************************************************************/
// NOTE: this file was auto-generated. DO NOT MODIFY!

// sav
#include "sav-struct.hpp"

// cdr
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "base/binary-data.hpp"

// C++ standard libary
#include <map>

/****************************************************************
** Macros.
*****************************************************************/
#define BAD_ENUM_VALUE( typename, value )                \
  FATAL( "unrecognized value for type " typename ": {}", \
      static_cast<std::underlying_type_t<has_city_1bit_type>>( o ) )

#define BAD_ENUM_STR_VALUE( typename, str_value )           \
  conv.err( "unreognize value for enum " typename ": '{}'", \
             str_value )

namespace sav {

/****************************************************************
** cargo_4bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         cargo_4bit_type const& o,
                         cdr::tag_t<cargo_4bit_type> ) {
  switch( o ) {
    case cargo_4bit_type::food: return "food";
    case cargo_4bit_type::sugar: return "sugar";
    case cargo_4bit_type::tobacco: return "tobacco";
    case cargo_4bit_type::cotton: return "cotton";
    case cargo_4bit_type::furs: return "furs";
    case cargo_4bit_type::lumber: return "lumber";
    case cargo_4bit_type::ore: return "ore";
    case cargo_4bit_type::silver: return "silver";
    case cargo_4bit_type::horses: return "horses";
    case cargo_4bit_type::rum: return "rum";
    case cargo_4bit_type::cigars: return "cigars";
    case cargo_4bit_type::cloth: return "cloth";
    case cargo_4bit_type::coats: return "coats";
    case cargo_4bit_type::goods: return "goods";
    case cargo_4bit_type::tools: return "tools";
    case cargo_4bit_type::muskets: return "muskets";
  }
  BAD_ENUM_VALUE( "cargo_4bit_type", o );
}

cdr::result<cargo_4bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<cargo_4bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, cargo_4bit_type> const m{
    { "food", cargo_4bit_type::food },
    { "sugar", cargo_4bit_type::sugar },
    { "tobacco", cargo_4bit_type::tobacco },
    { "cotton", cargo_4bit_type::cotton },
    { "furs", cargo_4bit_type::furs },
    { "lumber", cargo_4bit_type::lumber },
    { "ore", cargo_4bit_type::ore },
    { "silver", cargo_4bit_type::silver },
    { "horses", cargo_4bit_type::horses },
    { "rum", cargo_4bit_type::rum },
    { "cigars", cargo_4bit_type::cigars },
    { "cloth", cargo_4bit_type::cloth },
    { "coats", cargo_4bit_type::coats },
    { "goods", cargo_4bit_type::goods },
    { "tools", cargo_4bit_type::tools },
    { "muskets", cargo_4bit_type::muskets },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "cargo_4bit_type", str );
}

/****************************************************************
** control_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         control_type const& o,
                         cdr::tag_t<control_type> ) {
  switch( o ) {
    case control_type::player: return "PLAYER";
    case control_type::ai: return "AI";
    case control_type::withdrawn: return "WITHDRAWN";
  }
  BAD_ENUM_VALUE( "control_type", o );
}

cdr::result<control_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<control_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, control_type> const m{
    { "PLAYER", control_type::player },
    { "AI", control_type::ai },
    { "WITHDRAWN", control_type::withdrawn },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "control_type", str );
}

/****************************************************************
** difficulty_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         difficulty_type const& o,
                         cdr::tag_t<difficulty_type> ) {
  switch( o ) {
    case difficulty_type::discoverer: return "Discoverer";
    case difficulty_type::explorer: return "Explorer";
    case difficulty_type::conquistador: return "Conquistador";
    case difficulty_type::governor: return "Governor";
    case difficulty_type::viceroy: return "Viceroy";
  }
  BAD_ENUM_VALUE( "difficulty_type", o );
}

cdr::result<difficulty_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<difficulty_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, difficulty_type> const m{
    { "Discoverer", difficulty_type::discoverer },
    { "Explorer", difficulty_type::explorer },
    { "Conquistador", difficulty_type::conquistador },
    { "Governor", difficulty_type::governor },
    { "Viceroy", difficulty_type::viceroy },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "difficulty_type", str );
}

/****************************************************************
** fortification_level_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         fortification_level_type const& o,
                         cdr::tag_t<fortification_level_type> ) {
  switch( o ) {
    case fortification_level_type::none: return "none";
    case fortification_level_type::stockade: return "stockade";
    case fortification_level_type::fort: return "fort";
    case fortification_level_type::fortress: return "fortress";
  }
  BAD_ENUM_VALUE( "fortification_level_type", o );
}

cdr::result<fortification_level_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<fortification_level_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, fortification_level_type> const m{
    { "none", fortification_level_type::none },
    { "stockade", fortification_level_type::stockade },
    { "fort", fortification_level_type::fort },
    { "fortress", fortification_level_type::fortress },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "fortification_level_type", str );
}

/****************************************************************
** has_city_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         has_city_1bit_type const& o,
                         cdr::tag_t<has_city_1bit_type> ) {
  switch( o ) {
    case has_city_1bit_type::empty: return " ";
    case has_city_1bit_type::c: return "C";
  }
  BAD_ENUM_VALUE( "has_city_1bit_type", o );
}

cdr::result<has_city_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<has_city_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, has_city_1bit_type> const m{
    { " ", has_city_1bit_type::empty },
    { "C", has_city_1bit_type::c },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "has_city_1bit_type", str );
}

/****************************************************************
** has_unit_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         has_unit_1bit_type const& o,
                         cdr::tag_t<has_unit_1bit_type> ) {
  switch( o ) {
    case has_unit_1bit_type::empty: return " ";
    case has_unit_1bit_type::u: return "U";
  }
  BAD_ENUM_VALUE( "has_unit_1bit_type", o );
}

cdr::result<has_unit_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<has_unit_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, has_unit_1bit_type> const m{
    { " ", has_unit_1bit_type::empty },
    { "U", has_unit_1bit_type::u },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "has_unit_1bit_type", str );
}

/****************************************************************
** hills_river_3bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         hills_river_3bit_type const& o,
                         cdr::tag_t<hills_river_3bit_type> ) {
  switch( o ) {
    case hills_river_3bit_type::empty: return "  ";
    case hills_river_3bit_type::c: return "^ ";
    case hills_river_3bit_type::t: return "~ ";
    case hills_river_3bit_type::tc: return "~^";
    case hills_river_3bit_type::qq: return "??";
    case hills_river_3bit_type::cc: return "^^";
    case hills_river_3bit_type::tt: return "~~";
  }
  BAD_ENUM_VALUE( "hills_river_3bit_type", o );
}

cdr::result<hills_river_3bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<hills_river_3bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, hills_river_3bit_type> const m{
    { "  ", hills_river_3bit_type::empty },
    { "^ ", hills_river_3bit_type::c },
    { "~ ", hills_river_3bit_type::t },
    { "~^", hills_river_3bit_type::tc },
    { "??", hills_river_3bit_type::qq },
    { "^^", hills_river_3bit_type::cc },
    { "~~", hills_river_3bit_type::tt },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "hills_river_3bit_type", str );
}

/****************************************************************
** level_2bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         level_2bit_type const& o,
                         cdr::tag_t<level_2bit_type> ) {
  switch( o ) {
    case level_2bit_type::_0: return "0";
    case level_2bit_type::_1: return "1";
    case level_2bit_type::_2: return "2";
  }
  BAD_ENUM_VALUE( "level_2bit_type", o );
}

cdr::result<level_2bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<level_2bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, level_2bit_type> const m{
    { "0", level_2bit_type::_0 },
    { "1", level_2bit_type::_1 },
    { "2", level_2bit_type::_2 },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "level_2bit_type", str );
}

/****************************************************************
** level_3bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         level_3bit_type const& o,
                         cdr::tag_t<level_3bit_type> ) {
  switch( o ) {
    case level_3bit_type::_0: return "0";
    case level_3bit_type::_1: return "1";
    case level_3bit_type::_2: return "2";
    case level_3bit_type::_3: return "3";
  }
  BAD_ENUM_VALUE( "level_3bit_type", o );
}

cdr::result<level_3bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<level_3bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, level_3bit_type> const m{
    { "0", level_3bit_type::_0 },
    { "1", level_3bit_type::_1 },
    { "2", level_3bit_type::_2 },
    { "3", level_3bit_type::_3 },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "level_3bit_type", str );
}

/****************************************************************
** nation_4bit_short_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         nation_4bit_short_type const& o,
                         cdr::tag_t<nation_4bit_short_type> ) {
  switch( o ) {
    case nation_4bit_short_type::en: return "EN";
    case nation_4bit_short_type::fr: return "FR";
    case nation_4bit_short_type::sp: return "SP";
    case nation_4bit_short_type::nl: return "NL";
    case nation_4bit_short_type::in: return "in";
    case nation_4bit_short_type::az: return "az";
    case nation_4bit_short_type::aw: return "aw";
    case nation_4bit_short_type::ir: return "ir";
    case nation_4bit_short_type::ch: return "ch";
    case nation_4bit_short_type::ap: return "ap";
    case nation_4bit_short_type::si: return "si";
    case nation_4bit_short_type::tu: return "tu";
    case nation_4bit_short_type::empty: return "  ";
  }
  BAD_ENUM_VALUE( "nation_4bit_short_type", o );
}

cdr::result<nation_4bit_short_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_4bit_short_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, nation_4bit_short_type> const m{
    { "EN", nation_4bit_short_type::en },
    { "FR", nation_4bit_short_type::fr },
    { "SP", nation_4bit_short_type::sp },
    { "NL", nation_4bit_short_type::nl },
    { "in", nation_4bit_short_type::in },
    { "az", nation_4bit_short_type::az },
    { "aw", nation_4bit_short_type::aw },
    { "ir", nation_4bit_short_type::ir },
    { "ch", nation_4bit_short_type::ch },
    { "ap", nation_4bit_short_type::ap },
    { "si", nation_4bit_short_type::si },
    { "tu", nation_4bit_short_type::tu },
    { "  ", nation_4bit_short_type::empty },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "nation_4bit_short_type", str );
}

/****************************************************************
** nation_4bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         nation_4bit_type const& o,
                         cdr::tag_t<nation_4bit_type> ) {
  switch( o ) {
    case nation_4bit_type::england: return "England";
    case nation_4bit_type::france: return "France";
    case nation_4bit_type::spain: return "Spain";
    case nation_4bit_type::netherlands: return "Netherlands";
    case nation_4bit_type::inca: return "Inca";
    case nation_4bit_type::aztec: return "Aztec";
    case nation_4bit_type::awarak: return "Awarak";
    case nation_4bit_type::iroquois: return "Iroquois";
    case nation_4bit_type::cherokee: return "Cherokee";
    case nation_4bit_type::apache: return "Apache";
    case nation_4bit_type::sioux: return "Sioux";
    case nation_4bit_type::tupi: return "Tupi";
    case nation_4bit_type::none: return "None";
  }
  BAD_ENUM_VALUE( "nation_4bit_type", o );
}

cdr::result<nation_4bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_4bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, nation_4bit_type> const m{
    { "England", nation_4bit_type::england },
    { "France", nation_4bit_type::france },
    { "Spain", nation_4bit_type::spain },
    { "Netherlands", nation_4bit_type::netherlands },
    { "Inca", nation_4bit_type::inca },
    { "Aztec", nation_4bit_type::aztec },
    { "Awarak", nation_4bit_type::awarak },
    { "Iroquois", nation_4bit_type::iroquois },
    { "Cherokee", nation_4bit_type::cherokee },
    { "Apache", nation_4bit_type::apache },
    { "Sioux", nation_4bit_type::sioux },
    { "Tupi", nation_4bit_type::tupi },
    { "None", nation_4bit_type::none },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "nation_4bit_type", str );
}

/****************************************************************
** nation_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         nation_type const& o,
                         cdr::tag_t<nation_type> ) {
  switch( o ) {
    case nation_type::england: return "England";
    case nation_type::france: return "France";
    case nation_type::spain: return "Spain";
    case nation_type::netherlands: return "Netherlands";
    case nation_type::inca: return "Inca";
    case nation_type::aztec: return "Aztec";
    case nation_type::awarak: return "Awarak";
    case nation_type::iroquois: return "Iroquois";
    case nation_type::cherokee: return "Cherokee";
    case nation_type::apache: return "Apache";
    case nation_type::sioux: return "Sioux";
    case nation_type::tupi: return "Tupi";
    case nation_type::none: return "None";
  }
  BAD_ENUM_VALUE( "nation_type", o );
}

cdr::result<nation_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, nation_type> const m{
    { "England", nation_type::england },
    { "France", nation_type::france },
    { "Spain", nation_type::spain },
    { "Netherlands", nation_type::netherlands },
    { "Inca", nation_type::inca },
    { "Aztec", nation_type::aztec },
    { "Awarak", nation_type::awarak },
    { "Iroquois", nation_type::iroquois },
    { "Cherokee", nation_type::cherokee },
    { "Apache", nation_type::apache },
    { "Sioux", nation_type::sioux },
    { "Tupi", nation_type::tupi },
    { "None", nation_type::none },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "nation_type", str );
}

/****************************************************************
** occupation_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         occupation_type const& o,
                         cdr::tag_t<occupation_type> ) {
  switch( o ) {
    case occupation_type::farmer: return "Farmer";
    case occupation_type::sugar_planter: return "Sugar planter";
    case occupation_type::tobacco_planter: return "Tobacco planter";
    case occupation_type::cotton_planter: return "Cotton planter";
    case occupation_type::fur_trapper: return "Fur trapper";
    case occupation_type::lumberjack: return "Lumberjack";
    case occupation_type::ore_miner: return "Ore miner";
    case occupation_type::silver_miner: return "Silver miner";
    case occupation_type::fisherman: return "Fisherman";
    case occupation_type::distiller: return "Distiller";
    case occupation_type::tobacconist: return "Tobacconist";
    case occupation_type::weaver: return "Weaver";
    case occupation_type::fur_trader: return "Fur trader";
    case occupation_type::carpenter: return "Carpenter";
    case occupation_type::blacksmith: return "Blacksmith";
    case occupation_type::gunsmith: return "Gunsmith";
    case occupation_type::preacher: return "Preacher";
    case occupation_type::statesman: return "Statesman";
    case occupation_type::teacher: return "Teacher";
    case occupation_type::qqqqqqqqqq: return "??????????";
  }
  BAD_ENUM_VALUE( "occupation_type", o );
}

cdr::result<occupation_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<occupation_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, occupation_type> const m{
    { "Farmer", occupation_type::farmer },
    { "Sugar planter", occupation_type::sugar_planter },
    { "Tobacco planter", occupation_type::tobacco_planter },
    { "Cotton planter", occupation_type::cotton_planter },
    { "Fur trapper", occupation_type::fur_trapper },
    { "Lumberjack", occupation_type::lumberjack },
    { "Ore miner", occupation_type::ore_miner },
    { "Silver miner", occupation_type::silver_miner },
    { "Fisherman", occupation_type::fisherman },
    { "Distiller", occupation_type::distiller },
    { "Tobacconist", occupation_type::tobacconist },
    { "Weaver", occupation_type::weaver },
    { "Fur trader", occupation_type::fur_trader },
    { "Carpenter", occupation_type::carpenter },
    { "Blacksmith", occupation_type::blacksmith },
    { "Gunsmith", occupation_type::gunsmith },
    { "Preacher", occupation_type::preacher },
    { "Statesman", occupation_type::statesman },
    { "Teacher", occupation_type::teacher },
    { "??????????", occupation_type::qqqqqqqqqq },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "occupation_type", str );
}

/****************************************************************
** orders_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         orders_type const& o,
                         cdr::tag_t<orders_type> ) {
  switch( o ) {
    case orders_type::none: return "none";
    case orders_type::sentry: return "sentry";
    case orders_type::trading: return "trading";
    case orders_type::g0to: return "goto";
    case orders_type::fortified: return "fortified";
    case orders_type::fortify: return "fortify";
    case orders_type::plow: return "plow";
    case orders_type::road: return "road";
    case orders_type::unknowna: return "unknowna";
    case orders_type::unknownb: return "unknownb";
    case orders_type::unknownc: return "unknownc";
  }
  BAD_ENUM_VALUE( "orders_type", o );
}

cdr::result<orders_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<orders_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, orders_type> const m{
    { "none", orders_type::none },
    { "sentry", orders_type::sentry },
    { "trading", orders_type::trading },
    { "goto", orders_type::g0to },
    { "fortified", orders_type::fortified },
    { "fortify", orders_type::fortify },
    { "plow", orders_type::plow },
    { "road", orders_type::road },
    { "unknowna", orders_type::unknowna },
    { "unknownb", orders_type::unknownb },
    { "unknownc", orders_type::unknownc },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "orders_type", str );
}

/****************************************************************
** pacific_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         pacific_1bit_type const& o,
                         cdr::tag_t<pacific_1bit_type> ) {
  switch( o ) {
    case pacific_1bit_type::empty: return " ";
    case pacific_1bit_type::t: return "~";
  }
  BAD_ENUM_VALUE( "pacific_1bit_type", o );
}

cdr::result<pacific_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<pacific_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, pacific_1bit_type> const m{
    { " ", pacific_1bit_type::empty },
    { "~", pacific_1bit_type::t },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "pacific_1bit_type", str );
}

/****************************************************************
** plowed_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         plowed_1bit_type const& o,
                         cdr::tag_t<plowed_1bit_type> ) {
  switch( o ) {
    case plowed_1bit_type::empty: return " ";
    case plowed_1bit_type::h: return "#";
  }
  BAD_ENUM_VALUE( "plowed_1bit_type", o );
}

cdr::result<plowed_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<plowed_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, plowed_1bit_type> const m{
    { " ", plowed_1bit_type::empty },
    { "#", plowed_1bit_type::h },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "plowed_1bit_type", str );
}

/****************************************************************
** profession_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         profession_type const& o,
                         cdr::tag_t<profession_type> ) {
  switch( o ) {
    case profession_type::expert_farmer: return "Expert farmer";
    case profession_type::master_sugar_planter: return "Master sugar planter";
    case profession_type::master_tobacco_planter: return "Master tobacco planter";
    case profession_type::master_cotton_planter: return "Master cotton planter";
    case profession_type::expert_fur_trapper: return "Expert fur trapper";
    case profession_type::expert_lumberjack: return "Expert lumberjack";
    case profession_type::expert_ore_miner: return "Expert ore miner";
    case profession_type::expert_silver_miner: return "Expert silver miner";
    case profession_type::expert_fisherman: return "Expert fisherman";
    case profession_type::master_distiller: return "Master distiller";
    case profession_type::master_tobacconist: return "Master tobacconist";
    case profession_type::master_weaver: return "Master weaver";
    case profession_type::master_fur_trader: return "Master fur trader";
    case profession_type::master_carpenter: return "Master carpenter";
    case profession_type::master_blacksmith: return "Master blacksmith";
    case profession_type::master_gunsmith: return "Master gunsmith";
    case profession_type::firebrand_preacher: return "Firebrand preacher";
    case profession_type::elder_statesman: return "Elder statesman";
    case profession_type::a_student: return "*(Student)";
    case profession_type::a_free_colonist: return "*(Free colonist)";
    case profession_type::hardy_pioneer: return "Hardy pioneer";
    case profession_type::veteran_soldier: return "Veteran soldier";
    case profession_type::seasoned_scout: return "Seasoned scout";
    case profession_type::veteran_dragoon: return "Veteran dragoon";
    case profession_type::jesuit_missionary: return "Jesuit missionary";
    case profession_type::indentured_servant: return "Indentured servant";
    case profession_type::petty_criminal: return "Petty criminal";
    case profession_type::indian_convert: return "Indian convert";
    case profession_type::free_colonist: return "Free colonist";
  }
  BAD_ENUM_VALUE( "profession_type", o );
}

cdr::result<profession_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<profession_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, profession_type> const m{
    { "Expert farmer", profession_type::expert_farmer },
    { "Master sugar planter", profession_type::master_sugar_planter },
    { "Master tobacco planter", profession_type::master_tobacco_planter },
    { "Master cotton planter", profession_type::master_cotton_planter },
    { "Expert fur trapper", profession_type::expert_fur_trapper },
    { "Expert lumberjack", profession_type::expert_lumberjack },
    { "Expert ore miner", profession_type::expert_ore_miner },
    { "Expert silver miner", profession_type::expert_silver_miner },
    { "Expert fisherman", profession_type::expert_fisherman },
    { "Master distiller", profession_type::master_distiller },
    { "Master tobacconist", profession_type::master_tobacconist },
    { "Master weaver", profession_type::master_weaver },
    { "Master fur trader", profession_type::master_fur_trader },
    { "Master carpenter", profession_type::master_carpenter },
    { "Master blacksmith", profession_type::master_blacksmith },
    { "Master gunsmith", profession_type::master_gunsmith },
    { "Firebrand preacher", profession_type::firebrand_preacher },
    { "Elder statesman", profession_type::elder_statesman },
    { "*(Student)", profession_type::a_student },
    { "*(Free colonist)", profession_type::a_free_colonist },
    { "Hardy pioneer", profession_type::hardy_pioneer },
    { "Veteran soldier", profession_type::veteran_soldier },
    { "Seasoned scout", profession_type::seasoned_scout },
    { "Veteran dragoon", profession_type::veteran_dragoon },
    { "Jesuit missionary", profession_type::jesuit_missionary },
    { "Indentured servant", profession_type::indentured_servant },
    { "Petty criminal", profession_type::petty_criminal },
    { "Indian convert", profession_type::indian_convert },
    { "Free colonist", profession_type::free_colonist },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "profession_type", str );
}

/****************************************************************
** purchased_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         purchased_1bit_type const& o,
                         cdr::tag_t<purchased_1bit_type> ) {
  switch( o ) {
    case purchased_1bit_type::empty: return " ";
    case purchased_1bit_type::a: return "*";
  }
  BAD_ENUM_VALUE( "purchased_1bit_type", o );
}

cdr::result<purchased_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<purchased_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, purchased_1bit_type> const m{
    { " ", purchased_1bit_type::empty },
    { "*", purchased_1bit_type::a },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "purchased_1bit_type", str );
}

/****************************************************************
** region_id_4bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         region_id_4bit_type const& o,
                         cdr::tag_t<region_id_4bit_type> ) {
  switch( o ) {
    case region_id_4bit_type::_0: return " 0";
    case region_id_4bit_type::_1: return " 1";
    case region_id_4bit_type::_2: return " 2";
    case region_id_4bit_type::_3: return " 3";
    case region_id_4bit_type::_4: return " 4";
    case region_id_4bit_type::_5: return " 5";
    case region_id_4bit_type::_6: return " 6";
    case region_id_4bit_type::_7: return " 7";
    case region_id_4bit_type::_8: return " 8";
    case region_id_4bit_type::_9: return " 9";
    case region_id_4bit_type::_10: return "10";
    case region_id_4bit_type::_11: return "11";
    case region_id_4bit_type::_12: return "12";
    case region_id_4bit_type::_13: return "13";
    case region_id_4bit_type::_14: return "14";
    case region_id_4bit_type::_15: return "15";
  }
  BAD_ENUM_VALUE( "region_id_4bit_type", o );
}

cdr::result<region_id_4bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<region_id_4bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, region_id_4bit_type> const m{
    { " 0", region_id_4bit_type::_0 },
    { " 1", region_id_4bit_type::_1 },
    { " 2", region_id_4bit_type::_2 },
    { " 3", region_id_4bit_type::_3 },
    { " 4", region_id_4bit_type::_4 },
    { " 5", region_id_4bit_type::_5 },
    { " 6", region_id_4bit_type::_6 },
    { " 7", region_id_4bit_type::_7 },
    { " 8", region_id_4bit_type::_8 },
    { " 9", region_id_4bit_type::_9 },
    { "10", region_id_4bit_type::_10 },
    { "11", region_id_4bit_type::_11 },
    { "12", region_id_4bit_type::_12 },
    { "13", region_id_4bit_type::_13 },
    { "14", region_id_4bit_type::_14 },
    { "15", region_id_4bit_type::_15 },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "region_id_4bit_type", str );
}

/****************************************************************
** relation_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         relation_type const& o,
                         cdr::tag_t<relation_type> ) {
  switch( o ) {
    case relation_type::not_met: return "not met";
    case relation_type::war: return "war";
    case relation_type::peace: return "peace";
    case relation_type::unknown_rel2: return "unknown_rel2";
    case relation_type::unknown_rel: return "unknown_rel";
  }
  BAD_ENUM_VALUE( "relation_type", o );
}

cdr::result<relation_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<relation_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, relation_type> const m{
    { "not met", relation_type::not_met },
    { "war", relation_type::war },
    { "peace", relation_type::peace },
    { "unknown_rel2", relation_type::unknown_rel2 },
    { "unknown_rel", relation_type::unknown_rel },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "relation_type", str );
}

/****************************************************************
** road_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         road_1bit_type const& o,
                         cdr::tag_t<road_1bit_type> ) {
  switch( o ) {
    case road_1bit_type::empty: return " ";
    case road_1bit_type::e: return "=";
  }
  BAD_ENUM_VALUE( "road_1bit_type", o );
}

cdr::result<road_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<road_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, road_1bit_type> const m{
    { " ", road_1bit_type::empty },
    { "=", road_1bit_type::e },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "road_1bit_type", str );
}

/****************************************************************
** season_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         season_type const& o,
                         cdr::tag_t<season_type> ) {
  switch( o ) {
    case season_type::spring: return "spring";
    case season_type::autumn: return "autumn";
  }
  BAD_ENUM_VALUE( "season_type", o );
}

cdr::result<season_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<season_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, season_type> const m{
    { "spring", season_type::spring },
    { "autumn", season_type::autumn },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "season_type", str );
}

/****************************************************************
** suppress_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         suppress_1bit_type const& o,
                         cdr::tag_t<suppress_1bit_type> ) {
  switch( o ) {
    case suppress_1bit_type::empty: return " ";
    case suppress_1bit_type::_: return "_";
  }
  BAD_ENUM_VALUE( "suppress_1bit_type", o );
}

cdr::result<suppress_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<suppress_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, suppress_1bit_type> const m{
    { " ", suppress_1bit_type::empty },
    { "_", suppress_1bit_type::_ },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "suppress_1bit_type", str );
}

/****************************************************************
** tech_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         tech_type const& o,
                         cdr::tag_t<tech_type> ) {
  switch( o ) {
    case tech_type::semi_nomadic: return "Semi-Nomadic";
    case tech_type::agrarian: return "Agrarian";
    case tech_type::advanced: return "Advanced";
    case tech_type::civilized: return "Civilized";
  }
  BAD_ENUM_VALUE( "tech_type", o );
}

cdr::result<tech_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<tech_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, tech_type> const m{
    { "Semi-Nomadic", tech_type::semi_nomadic },
    { "Agrarian", tech_type::agrarian },
    { "Advanced", tech_type::advanced },
    { "Civilized", tech_type::civilized },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "tech_type", str );
}

/****************************************************************
** terrain_5bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         terrain_5bit_type const& o,
                         cdr::tag_t<terrain_5bit_type> ) {
  switch( o ) {
    case terrain_5bit_type::tu: return "tu ";
    case terrain_5bit_type::de: return "de ";
    case terrain_5bit_type::pl: return "pl ";
    case terrain_5bit_type::pr: return "pr ";
    case terrain_5bit_type::gr: return "gr ";
    case terrain_5bit_type::sa: return "sa ";
    case terrain_5bit_type::sw: return "sw ";
    case terrain_5bit_type::mr: return "mr ";
    case terrain_5bit_type::tuf: return "tuF";
    case terrain_5bit_type::def: return "deF";
    case terrain_5bit_type::plf: return "plF";
    case terrain_5bit_type::prf: return "prF";
    case terrain_5bit_type::grf: return "grF";
    case terrain_5bit_type::saf: return "saF";
    case terrain_5bit_type::swf: return "swF";
    case terrain_5bit_type::mrf: return "mrF";
    case terrain_5bit_type::arc: return "arc";
    case terrain_5bit_type::ttt: return "~~~";
    case terrain_5bit_type::tnt: return "~:~";
  }
  BAD_ENUM_VALUE( "terrain_5bit_type", o );
}

cdr::result<terrain_5bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<terrain_5bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, terrain_5bit_type> const m{
    { "tu ", terrain_5bit_type::tu },
    { "de ", terrain_5bit_type::de },
    { "pl ", terrain_5bit_type::pl },
    { "pr ", terrain_5bit_type::pr },
    { "gr ", terrain_5bit_type::gr },
    { "sa ", terrain_5bit_type::sa },
    { "sw ", terrain_5bit_type::sw },
    { "mr ", terrain_5bit_type::mr },
    { "tuF", terrain_5bit_type::tuf },
    { "deF", terrain_5bit_type::def },
    { "plF", terrain_5bit_type::plf },
    { "prF", terrain_5bit_type::prf },
    { "grF", terrain_5bit_type::grf },
    { "saF", terrain_5bit_type::saf },
    { "swF", terrain_5bit_type::swf },
    { "mrF", terrain_5bit_type::mrf },
    { "arc", terrain_5bit_type::arc },
    { "~~~", terrain_5bit_type::ttt },
    { "~:~", terrain_5bit_type::tnt },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "terrain_5bit_type", str );
}

/****************************************************************
** trade_route_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         trade_route_type const& o,
                         cdr::tag_t<trade_route_type> ) {
  switch( o ) {
    case trade_route_type::land: return "land";
    case trade_route_type::sea: return "sea";
  }
  BAD_ENUM_VALUE( "trade_route_type", o );
}

cdr::result<trade_route_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<trade_route_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, trade_route_type> const m{
    { "land", trade_route_type::land },
    { "sea", trade_route_type::sea },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "trade_route_type", str );
}

/****************************************************************
** unit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         unit_type const& o,
                         cdr::tag_t<unit_type> ) {
  switch( o ) {
    case unit_type::colonist: return "Colonist";
    case unit_type::soldier: return "Soldier";
    case unit_type::pioneer: return "Pioneer";
    case unit_type::missionary: return "Missionary";
    case unit_type::dragoon: return "Dragoon";
    case unit_type::scout: return "Scout";
    case unit_type::tory_regular: return "Tory regular";
    case unit_type::continental_cavalry: return "Continental cavalry";
    case unit_type::tory_cavalry: return "Tory cavalry";
    case unit_type::continental_army: return "Continental army";
    case unit_type::treasure: return "Treasure";
    case unit_type::artillery: return "Artillery";
    case unit_type::wagon_train: return "Wagon train";
    case unit_type::caravel: return "Caravel";
    case unit_type::merchantman: return "Merchantman";
    case unit_type::galleon: return "Galleon";
    case unit_type::privateer: return "Privateer";
    case unit_type::frigate: return "Frigate";
    case unit_type::man_o_war: return "Man-O-War";
    case unit_type::brave: return "Brave";
    case unit_type::armed_brave: return "Armed brave";
    case unit_type::mounted_brave: return "Mounted brave";
    case unit_type::mounted_warrior: return "Mounted warrior";
  }
  BAD_ENUM_VALUE( "unit_type", o );
}

cdr::result<unit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<unit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, unit_type> const m{
    { "Colonist", unit_type::colonist },
    { "Soldier", unit_type::soldier },
    { "Pioneer", unit_type::pioneer },
    { "Missionary", unit_type::missionary },
    { "Dragoon", unit_type::dragoon },
    { "Scout", unit_type::scout },
    { "Tory regular", unit_type::tory_regular },
    { "Continental cavalry", unit_type::continental_cavalry },
    { "Tory cavalry", unit_type::tory_cavalry },
    { "Continental army", unit_type::continental_army },
    { "Treasure", unit_type::treasure },
    { "Artillery", unit_type::artillery },
    { "Wagon train", unit_type::wagon_train },
    { "Caravel", unit_type::caravel },
    { "Merchantman", unit_type::merchantman },
    { "Galleon", unit_type::galleon },
    { "Privateer", unit_type::privateer },
    { "Frigate", unit_type::frigate },
    { "Man-O-War", unit_type::man_o_war },
    { "Brave", unit_type::brave },
    { "Armed brave", unit_type::armed_brave },
    { "Mounted brave", unit_type::mounted_brave },
    { "Mounted warrior", unit_type::mounted_warrior },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "unit_type", str );
}

/****************************************************************
** visible_to_dutch_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         visible_to_dutch_1bit_type const& o,
                         cdr::tag_t<visible_to_dutch_1bit_type> ) {
  switch( o ) {
    case visible_to_dutch_1bit_type::empty: return " ";
    case visible_to_dutch_1bit_type::d: return "D";
  }
  BAD_ENUM_VALUE( "visible_to_dutch_1bit_type", o );
}

cdr::result<visible_to_dutch_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_dutch_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, visible_to_dutch_1bit_type> const m{
    { " ", visible_to_dutch_1bit_type::empty },
    { "D", visible_to_dutch_1bit_type::d },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "visible_to_dutch_1bit_type", str );
}

/****************************************************************
** visible_to_english_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         visible_to_english_1bit_type const& o,
                         cdr::tag_t<visible_to_english_1bit_type> ) {
  switch( o ) {
    case visible_to_english_1bit_type::empty: return " ";
    case visible_to_english_1bit_type::e: return "E";
  }
  BAD_ENUM_VALUE( "visible_to_english_1bit_type", o );
}

cdr::result<visible_to_english_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_english_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, visible_to_english_1bit_type> const m{
    { " ", visible_to_english_1bit_type::empty },
    { "E", visible_to_english_1bit_type::e },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "visible_to_english_1bit_type", str );
}

/****************************************************************
** visible_to_french_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         visible_to_french_1bit_type const& o,
                         cdr::tag_t<visible_to_french_1bit_type> ) {
  switch( o ) {
    case visible_to_french_1bit_type::empty: return " ";
    case visible_to_french_1bit_type::f: return "F";
  }
  BAD_ENUM_VALUE( "visible_to_french_1bit_type", o );
}

cdr::result<visible_to_french_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_french_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, visible_to_french_1bit_type> const m{
    { " ", visible_to_french_1bit_type::empty },
    { "F", visible_to_french_1bit_type::f },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "visible_to_french_1bit_type", str );
}

/****************************************************************
** visible_to_spanish_1bit_type
*****************************************************************/
cdr::value to_canonical( cdr::converter&,
                         visible_to_spanish_1bit_type const& o,
                         cdr::tag_t<visible_to_spanish_1bit_type> ) {
  switch( o ) {
    case visible_to_spanish_1bit_type::empty: return " ";
    case visible_to_spanish_1bit_type::s: return "S";
  }
  BAD_ENUM_VALUE( "visible_to_spanish_1bit_type", o );
}

cdr::result<visible_to_spanish_1bit_type> from_canoncal(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_spanish_1bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, visible_to_spanish_1bit_type> const m{
    { " ", visible_to_spanish_1bit_type::empty },
    { "S", visible_to_spanish_1bit_type::s },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "visible_to_spanish_1bit_type", str );
}

/****************************************************************
** GameOptions
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, GameOptions& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.unused01 = (bits & 0b1111111); bits >>= 7;
  o.tutorial_hints = (bits & 0b1); bits >>= 1;
  o.water_color_cycling = (bits & 0b1); bits >>= 1;
  o.combat_analysis = (bits & 0b1); bits >>= 1;
  o.autosave = (bits & 0b1); bits >>= 1;
  o.end_of_turn = (bits & 0b1); bits >>= 1;
  o.fast_piece_slide = (bits & 0b1); bits >>= 1;
  o.cheats_enabled = (bits & 0b1); bits >>= 1;
  o.show_foreign_moves = (bits & 0b1); bits >>= 1;
  o.show_indian_moves = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, GameOptions const& o ) {
  uint16_t bits = 0;
  bits |= (o.show_indian_moves & 0b1); bits <<= 1;
  bits |= (o.show_foreign_moves & 0b1); bits <<= 1;
  bits |= (o.cheats_enabled & 0b1); bits <<= 1;
  bits |= (o.fast_piece_slide & 0b1); bits <<= 1;
  bits |= (o.end_of_turn & 0b1); bits <<= 1;
  bits |= (o.autosave & 0b1); bits <<= 1;
  bits |= (o.combat_analysis & 0b1); bits <<= 1;
  bits |= (o.water_color_cycling & 0b1); bits <<= 1;
  bits |= (o.tutorial_hints & 0b1); bits <<= 7;
  bits |= (o.unused01 & 0b1111111); bits <<= 0;
  return b.write_bytes<2>( bits );
}

/****************************************************************
** ColonyReportOptions
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, ColonyReportOptions& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.labels_on_cargo_and_terrain = (bits & 0b1); bits >>= 1;
  o.labels_on_buildings = (bits & 0b1); bits >>= 1;
  o.report_new_cargos_available = (bits & 0b1); bits >>= 1;
  o.report_inefficient_government = (bits & 0b1); bits >>= 1;
  o.report_tools_needed_for_production = (bits & 0b1); bits >>= 1;
  o.report_raw_materials_shortages = (bits & 0b1); bits >>= 1;
  o.report_food_shortages = (bits & 0b1); bits >>= 1;
  o.report_when_colonists_trained = (bits & 0b1); bits >>= 1;
  o.report_sons_of_liberty_membership = (bits & 0b1); bits >>= 1;
  o.report_rebel_majorities = (bits & 0b1); bits >>= 1;
  o.unused03 = (bits & 0b111111); bits >>= 6;
  return true;
}

bool write_binary( base::BinaryData& b, ColonyReportOptions const& o ) {
  uint16_t bits = 0;
  bits |= (o.unused03 & 0b111111); bits <<= 1;
  bits |= (o.report_rebel_majorities & 0b1); bits <<= 1;
  bits |= (o.report_sons_of_liberty_membership & 0b1); bits <<= 1;
  bits |= (o.report_when_colonists_trained & 0b1); bits <<= 1;
  bits |= (o.report_food_shortages & 0b1); bits <<= 1;
  bits |= (o.report_raw_materials_shortages & 0b1); bits <<= 1;
  bits |= (o.report_tools_needed_for_production & 0b1); bits <<= 1;
  bits |= (o.report_inefficient_government & 0b1); bits <<= 1;
  bits |= (o.report_new_cargos_available & 0b1); bits <<= 1;
  bits |= (o.labels_on_buildings & 0b1); bits <<= 1;
  bits |= (o.labels_on_cargo_and_terrain & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

/****************************************************************
** Event
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Event& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.discovery_of_the_new_world = (bits & 0b1); bits >>= 1;
  o.building_a_colony = (bits & 0b1); bits >>= 1;
  o.meeting_the_natives = (bits & 0b1); bits >>= 1;
  o.the_aztec_empire = (bits & 0b1); bits >>= 1;
  o.the_inca_nation = (bits & 0b1); bits >>= 1;
  o.discovery_of_the_pacific_ocean = (bits & 0b1); bits >>= 1;
  o.entering_indian_village = (bits & 0b1); bits >>= 1;
  o.the_fountain_of_youth = (bits & 0b1); bits >>= 1;
  o.cargo_from_the_new_world = (bits & 0b1); bits >>= 1;
  o.meeting_fellow_europeans = (bits & 0b1); bits >>= 1;
  o.colony_burning = (bits & 0b1); bits >>= 1;
  o.colony_destroyed = (bits & 0b1); bits >>= 1;
  o.indian_raid = (bits & 0b1); bits >>= 1;
  o.woodcut14 = (bits & 0b1); bits >>= 1;
  o.woodcut15 = (bits & 0b1); bits >>= 1;
  o.woodcut16 = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, Event const& o ) {
  uint16_t bits = 0;
  bits |= (o.woodcut16 & 0b1); bits <<= 1;
  bits |= (o.woodcut15 & 0b1); bits <<= 1;
  bits |= (o.woodcut14 & 0b1); bits <<= 1;
  bits |= (o.indian_raid & 0b1); bits <<= 1;
  bits |= (o.colony_destroyed & 0b1); bits <<= 1;
  bits |= (o.colony_burning & 0b1); bits <<= 1;
  bits |= (o.meeting_fellow_europeans & 0b1); bits <<= 1;
  bits |= (o.cargo_from_the_new_world & 0b1); bits <<= 1;
  bits |= (o.the_fountain_of_youth & 0b1); bits <<= 1;
  bits |= (o.entering_indian_village & 0b1); bits <<= 1;
  bits |= (o.discovery_of_the_pacific_ocean & 0b1); bits <<= 1;
  bits |= (o.the_inca_nation & 0b1); bits <<= 1;
  bits |= (o.the_aztec_empire & 0b1); bits <<= 1;
  bits |= (o.meeting_the_natives & 0b1); bits <<= 1;
  bits |= (o.building_a_colony & 0b1); bits <<= 1;
  bits |= (o.discovery_of_the_new_world & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

/****************************************************************
** Duration
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Duration& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.dur_1 = (bits & 0b1111); bits >>= 4;
  o.dur_2 = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Duration const& o ) {
  uint8_t bits = 0;
  bits |= (o.dur_2 & 0b1111); bits <<= 4;
  bits |= (o.dur_1 & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Buildings
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Buildings& o ) {
  uint64_t bits = 0;
  if( !b.read_bytes<6>( bits ) ) return false;
  o.fortification = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.armory = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.docks = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.town_hall = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.schoolhouse = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.warehouse = (bits & 0b1); bits >>= 1;
  o.unused05a = (bits & 0b1); bits >>= 1;
  o.stables = (bits & 0b1); bits >>= 1;
  o.custom_house = (bits & 0b1); bits >>= 1;
  o.printing_press = static_cast<level_2bit_type>( bits & 0b11 ); bits >>= 2;
  o.weavers_house = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.tobacconists_house = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.rum_distillers_house = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.capitol_unused = static_cast<level_2bit_type>( bits & 0b11 ); bits >>= 2;
  o.fur_traders_house = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.carpenters_shop = static_cast<level_2bit_type>( bits & 0b11 ); bits >>= 2;
  o.church = static_cast<level_2bit_type>( bits & 0b11 ); bits >>= 2;
  o.blacksmiths_house = static_cast<level_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.unused05b = (bits & 0b111111); bits >>= 6;
  return true;
}

bool write_binary( base::BinaryData& b, Buildings const& o ) {
  uint64_t bits = 0;
  bits |= (o.unused05b & 0b111111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.blacksmiths_house ) & 0b111); bits <<= 2;
  bits |= (static_cast<uint64_t>( o.church ) & 0b11); bits <<= 2;
  bits |= (static_cast<uint64_t>( o.carpenters_shop ) & 0b11); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.fur_traders_house ) & 0b111); bits <<= 2;
  bits |= (static_cast<uint64_t>( o.capitol_unused ) & 0b11); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.rum_distillers_house ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.tobacconists_house ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.weavers_house ) & 0b111); bits <<= 2;
  bits |= (static_cast<uint64_t>( o.printing_press ) & 0b11); bits <<= 1;
  bits |= (o.custom_house & 0b1); bits <<= 1;
  bits |= (o.stables & 0b1); bits <<= 1;
  bits |= (o.unused05a & 0b1); bits <<= 1;
  bits |= (o.warehouse & 0b1); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.schoolhouse ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.town_hall ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.docks ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.armory ) & 0b111); bits <<= 3;
  bits |= (static_cast<uint64_t>( o.fortification ) & 0b111); bits <<= 0;
  return b.write_bytes<6>( bits );
}

/****************************************************************
** CustomHouseFlags
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, CustomHouseFlags& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.food = (bits & 0b1); bits >>= 1;
  o.sugar = (bits & 0b1); bits >>= 1;
  o.tobacco = (bits & 0b1); bits >>= 1;
  o.cotton = (bits & 0b1); bits >>= 1;
  o.furs = (bits & 0b1); bits >>= 1;
  o.lumber = (bits & 0b1); bits >>= 1;
  o.ore = (bits & 0b1); bits >>= 1;
  o.silver = (bits & 0b1); bits >>= 1;
  o.horses = (bits & 0b1); bits >>= 1;
  o.rum = (bits & 0b1); bits >>= 1;
  o.cigars = (bits & 0b1); bits >>= 1;
  o.cloth = (bits & 0b1); bits >>= 1;
  o.coats = (bits & 0b1); bits >>= 1;
  o.trade_goods = (bits & 0b1); bits >>= 1;
  o.tools = (bits & 0b1); bits >>= 1;
  o.muskets = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, CustomHouseFlags const& o ) {
  uint16_t bits = 0;
  bits |= (o.muskets & 0b1); bits <<= 1;
  bits |= (o.tools & 0b1); bits <<= 1;
  bits |= (o.trade_goods & 0b1); bits <<= 1;
  bits |= (o.coats & 0b1); bits <<= 1;
  bits |= (o.cloth & 0b1); bits <<= 1;
  bits |= (o.cigars & 0b1); bits <<= 1;
  bits |= (o.rum & 0b1); bits <<= 1;
  bits |= (o.horses & 0b1); bits <<= 1;
  bits |= (o.silver & 0b1); bits <<= 1;
  bits |= (o.ore & 0b1); bits <<= 1;
  bits |= (o.lumber & 0b1); bits <<= 1;
  bits |= (o.furs & 0b1); bits <<= 1;
  bits |= (o.cotton & 0b1); bits <<= 1;
  bits |= (o.tobacco & 0b1); bits <<= 1;
  bits |= (o.sugar & 0b1); bits <<= 1;
  bits |= (o.food & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

/****************************************************************
** NationInfo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, NationInfo& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.nation_id = static_cast<nation_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.vis_to_english = (bits & 0b1); bits >>= 1;
  o.vis_to_french = (bits & 0b1); bits >>= 1;
  o.vis_to_spanish = (bits & 0b1); bits >>= 1;
  o.vis_to_dutch = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, NationInfo const& o ) {
  uint8_t bits = 0;
  bits |= (o.vis_to_dutch & 0b1); bits <<= 1;
  bits |= (o.vis_to_spanish & 0b1); bits <<= 1;
  bits |= (o.vis_to_french & 0b1); bits <<= 1;
  bits |= (o.vis_to_english & 0b1); bits <<= 4;
  bits |= (static_cast<uint8_t>( o.nation_id ) & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Unknown15
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Unknown15& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unknown15a = (bits & 0b1111111); bits >>= 7;
  o.damaged = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, Unknown15 const& o ) {
  uint8_t bits = 0;
  bits |= (o.damaged & 0b1); bits <<= 7;
  bits |= (o.unknown15a & 0b1111111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** CargoItems
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, CargoItems& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, CargoItems const& o ) {
  uint8_t bits = 0;
  bits |= (static_cast<uint8_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint8_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** BoycottBitmap
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, BoycottBitmap& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.food = (bits & 0b1); bits >>= 1;
  o.sugar = (bits & 0b1); bits >>= 1;
  o.tobacco = (bits & 0b1); bits >>= 1;
  o.cotton = (bits & 0b1); bits >>= 1;
  o.furs = (bits & 0b1); bits >>= 1;
  o.lumber = (bits & 0b1); bits >>= 1;
  o.ore = (bits & 0b1); bits >>= 1;
  o.silver = (bits & 0b1); bits >>= 1;
  o.horses = (bits & 0b1); bits >>= 1;
  o.rum = (bits & 0b1); bits >>= 1;
  o.cigars = (bits & 0b1); bits >>= 1;
  o.cloth = (bits & 0b1); bits >>= 1;
  o.coats = (bits & 0b1); bits >>= 1;
  o.trade_goods = (bits & 0b1); bits >>= 1;
  o.tools = (bits & 0b1); bits >>= 1;
  o.muskets = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, BoycottBitmap const& o ) {
  uint16_t bits = 0;
  bits |= (o.muskets & 0b1); bits <<= 1;
  bits |= (o.tools & 0b1); bits <<= 1;
  bits |= (o.trade_goods & 0b1); bits <<= 1;
  bits |= (o.coats & 0b1); bits <<= 1;
  bits |= (o.cloth & 0b1); bits <<= 1;
  bits |= (o.cigars & 0b1); bits <<= 1;
  bits |= (o.rum & 0b1); bits <<= 1;
  bits |= (o.horses & 0b1); bits <<= 1;
  bits |= (o.silver & 0b1); bits <<= 1;
  bits |= (o.ore & 0b1); bits <<= 1;
  bits |= (o.lumber & 0b1); bits <<= 1;
  bits |= (o.furs & 0b1); bits <<= 1;
  bits |= (o.cotton & 0b1); bits <<= 1;
  bits |= (o.tobacco & 0b1); bits <<= 1;
  bits |= (o.sugar & 0b1); bits <<= 1;
  bits |= (o.food & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

/****************************************************************
** ALCS
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, ALCS& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.artillery_near = (bits & 0b1); bits >>= 1;
  o.learned = (bits & 0b1); bits >>= 1;
  o.capital = (bits & 0b1); bits >>= 1;
  o.scouted = (bits & 0b1); bits >>= 1;
  o.unused09 = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, ALCS const& o ) {
  uint8_t bits = 0;
  bits |= (o.unused09 & 0b1111); bits <<= 1;
  bits |= (o.scouted & 0b1); bits <<= 1;
  bits |= (o.capital & 0b1); bits <<= 1;
  bits |= (o.learned & 0b1); bits <<= 1;
  bits |= (o.artillery_near & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** TILE
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, TILE& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.tile = static_cast<terrain_5bit_type>( bits & 0b11111 ); bits >>= 5;
  o.hill_river = static_cast<hills_river_3bit_type>( bits & 0b111 ); bits >>= 3;
  return true;
}

bool write_binary( base::BinaryData& b, TILE const& o ) {
  uint8_t bits = 0;
  bits |= (static_cast<uint8_t>( o.hill_river ) & 0b111); bits <<= 5;
  bits |= (static_cast<uint8_t>( o.tile ) & 0b11111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** MASK
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, MASK& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.has_unit = static_cast<has_unit_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.has_city = static_cast<has_city_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.suppress = static_cast<suppress_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.road = static_cast<road_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.purchased = static_cast<purchased_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.pacific = static_cast<pacific_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.plowed = static_cast<plowed_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.unused = static_cast<suppress_1bit_type>( bits & 0b1 ); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, MASK const& o ) {
  uint8_t bits = 0;
  bits |= (static_cast<uint8_t>( o.unused ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.plowed ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.pacific ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.purchased ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.road ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.suppress ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.has_city ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.has_unit ) & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** PATH
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, PATH& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.region_id = static_cast<region_id_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.visitor_nation = static_cast<nation_4bit_short_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, PATH const& o ) {
  uint8_t bits = 0;
  bits |= (static_cast<uint8_t>( o.visitor_nation ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint8_t>( o.region_id ) & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** SEEN
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, SEEN& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.score = static_cast<region_id_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.vis2en = static_cast<visible_to_english_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2fr = static_cast<visible_to_french_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2sp = static_cast<visible_to_spanish_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2du = static_cast<visible_to_dutch_1bit_type>( bits & 0b1 ); bits >>= 1;
  return true;
}

bool write_binary( base::BinaryData& b, SEEN const& o ) {
  uint8_t bits = 0;
  bits |= (static_cast<uint8_t>( o.vis2du ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.vis2sp ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.vis2fr ) & 0b1); bits <<= 1;
  bits |= (static_cast<uint8_t>( o.vis2en ) & 0b1); bits <<= 4;
  bits |= (static_cast<uint8_t>( o.score ) & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop1LoadsAndUnloadsCount const& o ) {
  uint8_t bits = 0;
  bits |= (o.loads_count & 0b1111); bits <<= 4;
  bits |= (o.unloads_count & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Stop1LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1LoadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop1LoadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop1UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1UnloadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop1UnloadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop2LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop2LoadsAndUnloadsCount const& o ) {
  uint8_t bits = 0;
  bits |= (o.loads_count & 0b1111); bits <<= 4;
  bits |= (o.unloads_count & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Stop2LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2LoadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop2LoadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop2UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2UnloadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop2UnloadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop3LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop3LoadsAndUnloadsCount const& o ) {
  uint8_t bits = 0;
  bits |= (o.loads_count & 0b1111); bits <<= 4;
  bits |= (o.unloads_count & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Stop3LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3LoadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop3LoadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop3UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3UnloadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop3UnloadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop4LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop4LoadsAndUnloadsCount const& o ) {
  uint8_t bits = 0;
  bits |= (o.loads_count & 0b1111); bits <<= 4;
  bits |= (o.unloads_count & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

/****************************************************************
** Stop4LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4LoadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop4LoadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** Stop4UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4UnloadsCargo& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<3>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_3 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_4 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_5 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_6 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, Stop4UnloadsCargo const& o ) {
  uint32_t bits = 0;
  bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111); bits <<= 4;
  bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111); bits <<= 0;
  return b.write_bytes<3>( bits );
}

/****************************************************************
** ExpeditionaryForce
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, ExpeditionaryForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::BinaryData& b, ExpeditionaryForce const& o ) {
  return true
    && write_binary( b, o.regulars )
    && write_binary( b, o.dragoons )
    && write_binary( b, o.man_o_wars )
    && write_binary( b, o.artillery )
    ;
}

/****************************************************************
** BackupForce
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, BackupForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::BinaryData& b, BackupForce const& o ) {
  return true
    && write_binary( b, o.regulars )
    && write_binary( b, o.dragoons )
    && write_binary( b, o.man_o_wars )
    && write_binary( b, o.artillery )
    ;
}

/****************************************************************
** PriceGroupState
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, PriceGroupState& o ) {
  return true
    && read_binary( b, o.food )
    && read_binary( b, o.sugar )
    && read_binary( b, o.tobacco )
    && read_binary( b, o.cotton )
    && read_binary( b, o.furs )
    && read_binary( b, o.lumber )
    && read_binary( b, o.ore )
    && read_binary( b, o.silver )
    && read_binary( b, o.horses )
    && read_binary( b, o.rum )
    && read_binary( b, o.cigars )
    && read_binary( b, o.cloth )
    && read_binary( b, o.coats )
    && read_binary( b, o.trade_goods )
    && read_binary( b, o.tools )
    && read_binary( b, o.muskets )
    ;
}

bool write_binary( base::BinaryData& b, PriceGroupState const& o ) {
  return true
    && write_binary( b, o.food )
    && write_binary( b, o.sugar )
    && write_binary( b, o.tobacco )
    && write_binary( b, o.cotton )
    && write_binary( b, o.furs )
    && write_binary( b, o.lumber )
    && write_binary( b, o.ore )
    && write_binary( b, o.silver )
    && write_binary( b, o.horses )
    && write_binary( b, o.rum )
    && write_binary( b, o.cigars )
    && write_binary( b, o.cloth )
    && write_binary( b, o.coats )
    && write_binary( b, o.trade_goods )
    && write_binary( b, o.tools )
    && write_binary( b, o.muskets )
    ;
}

/****************************************************************
** HEAD
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, HEAD& o ) {
  return true
    && read_binary( b, o.colonize )
    && read_binary( b, o.unknown00 )
    && read_binary( b, o.map_size_x )
    && read_binary( b, o.map_size_y )
    && read_binary( b, o.tut1 )
    && read_binary( b, o.unknown03 )
    && read_binary( b, o.game_options )
    && read_binary( b, o.colony_report_options )
    && read_binary( b, o.tut2 )
    && read_binary( b, o.tut3 )
    && read_binary( b, o.unknown39 )
    && read_binary( b, o.year )
    && read_binary( b, o.season )
    && read_binary( b, o.turn )
    && read_binary( b, o.tile_selection_mode )
    && read_binary( b, o.unknown40 )
    && read_binary( b, o.active_unit )
    && read_binary( b, o.unknown41 )
    && read_binary( b, o.tribe_count )
    && read_binary( b, o.unit_count )
    && read_binary( b, o.colony_count )
    && read_binary( b, o.trade_route_count )
    && read_binary( b, o.unknown42 )
    && read_binary( b, o.difficulty )
    && read_binary( b, o.unknown43a )
    && read_binary( b, o.unknown43b )
    && read_binary( b, o.founding_father )
    && read_binary( b, o.unknown44 )
    && read_binary( b, o.nation_relation )
    && read_binary( b, o.unknown45 )
    && read_binary( b, o.expeditionary_force )
    && read_binary( b, o.backup_force )
    && read_binary( b, o.price_group_state )
    && read_binary( b, o.event )
    && read_binary( b, o.unknown05 )
    ;
}

bool write_binary( base::BinaryData& b, HEAD const& o ) {
  return true
    && write_binary( b, o.colonize )
    && write_binary( b, o.unknown00 )
    && write_binary( b, o.map_size_x )
    && write_binary( b, o.map_size_y )
    && write_binary( b, o.tut1 )
    && write_binary( b, o.unknown03 )
    && write_binary( b, o.game_options )
    && write_binary( b, o.colony_report_options )
    && write_binary( b, o.tut2 )
    && write_binary( b, o.tut3 )
    && write_binary( b, o.unknown39 )
    && write_binary( b, o.year )
    && write_binary( b, o.season )
    && write_binary( b, o.turn )
    && write_binary( b, o.tile_selection_mode )
    && write_binary( b, o.unknown40 )
    && write_binary( b, o.active_unit )
    && write_binary( b, o.unknown41 )
    && write_binary( b, o.tribe_count )
    && write_binary( b, o.unit_count )
    && write_binary( b, o.colony_count )
    && write_binary( b, o.trade_route_count )
    && write_binary( b, o.unknown42 )
    && write_binary( b, o.difficulty )
    && write_binary( b, o.unknown43a )
    && write_binary( b, o.unknown43b )
    && write_binary( b, o.founding_father )
    && write_binary( b, o.unknown44 )
    && write_binary( b, o.nation_relation )
    && write_binary( b, o.unknown45 )
    && write_binary( b, o.expeditionary_force )
    && write_binary( b, o.backup_force )
    && write_binary( b, o.price_group_state )
    && write_binary( b, o.event )
    && write_binary( b, o.unknown05 )
    ;
}

/****************************************************************
** PLAYER
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, PLAYER& o ) {
  return true
    && read_binary( b, o.name )
    && read_binary( b, o.country_name )
    && read_binary( b, o.unknown06 )
    && read_binary( b, o.control )
    && read_binary( b, o.founded_colonies )
    && read_binary( b, o.diplomacy )
    ;
}

bool write_binary( base::BinaryData& b, PLAYER const& o ) {
  return true
    && write_binary( b, o.name )
    && write_binary( b, o.country_name )
    && write_binary( b, o.unknown06 )
    && write_binary( b, o.control )
    && write_binary( b, o.founded_colonies )
    && write_binary( b, o.diplomacy )
    ;
}

/****************************************************************
** Tiles
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Tiles& o ) {
  return true
    && read_binary( b, o.tile_n )
    && read_binary( b, o.tile_e )
    && read_binary( b, o.tile_s )
    && read_binary( b, o.tile_w )
    && read_binary( b, o.tile_nw )
    && read_binary( b, o.tile_ne )
    && read_binary( b, o.tile_se )
    && read_binary( b, o.tile_sw )
    ;
}

bool write_binary( base::BinaryData& b, Tiles const& o ) {
  return true
    && write_binary( b, o.tile_n )
    && write_binary( b, o.tile_e )
    && write_binary( b, o.tile_s )
    && write_binary( b, o.tile_w )
    && write_binary( b, o.tile_nw )
    && write_binary( b, o.tile_ne )
    && write_binary( b, o.tile_se )
    && write_binary( b, o.tile_sw )
    ;
}

/****************************************************************
** Stock
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Stock& o ) {
  return true
    && read_binary( b, o.food )
    && read_binary( b, o.sugar )
    && read_binary( b, o.tobacco )
    && read_binary( b, o.cotton )
    && read_binary( b, o.furs )
    && read_binary( b, o.lumber )
    && read_binary( b, o.ore )
    && read_binary( b, o.silver )
    && read_binary( b, o.horses )
    && read_binary( b, o.rum )
    && read_binary( b, o.cigars )
    && read_binary( b, o.cloth )
    && read_binary( b, o.coats )
    && read_binary( b, o.trade_goods )
    && read_binary( b, o.tools )
    && read_binary( b, o.muskets )
    ;
}

bool write_binary( base::BinaryData& b, Stock const& o ) {
  return true
    && write_binary( b, o.food )
    && write_binary( b, o.sugar )
    && write_binary( b, o.tobacco )
    && write_binary( b, o.cotton )
    && write_binary( b, o.furs )
    && write_binary( b, o.lumber )
    && write_binary( b, o.ore )
    && write_binary( b, o.silver )
    && write_binary( b, o.horses )
    && write_binary( b, o.rum )
    && write_binary( b, o.cigars )
    && write_binary( b, o.cloth )
    && write_binary( b, o.coats )
    && write_binary( b, o.trade_goods )
    && write_binary( b, o.tools )
    && write_binary( b, o.muskets )
    ;
}

/****************************************************************
** PopulationOnMap
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, PopulationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::BinaryData& b, PopulationOnMap const& o ) {
  return true
    && write_binary( b, o.for_english )
    && write_binary( b, o.for_french )
    && write_binary( b, o.for_spanish )
    && write_binary( b, o.for_dutch )
    ;
}

/****************************************************************
** FortificationOnMap
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, FortificationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::BinaryData& b, FortificationOnMap const& o ) {
  return true
    && write_binary( b, o.for_english )
    && write_binary( b, o.for_french )
    && write_binary( b, o.for_spanish )
    && write_binary( b, o.for_dutch )
    ;
}

/****************************************************************
** COLONY
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, COLONY& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.name )
    && read_binary( b, o.nation_id )
    && read_binary( b, o.unknown08 )
    && read_binary( b, o.population )
    && read_binary( b, o.occupation )
    && read_binary( b, o.profession )
    && read_binary( b, o.duration )
    && read_binary( b, o.tiles )
    && read_binary( b, o.unknown10 )
    && read_binary( b, o.buildings )
    && read_binary( b, o.custom_house_flags )
    && read_binary( b, o.unknown11 )
    && read_binary( b, o.hammers )
    && read_binary( b, o.building_in_production )
    && read_binary( b, o.warehouse_level )
    && read_binary( b, o.unknown12 )
    && read_binary( b, o.stock )
    && read_binary( b, o.population_on_map )
    && read_binary( b, o.fortification_on_map )
    && read_binary( b, o.rebel_dividend )
    && read_binary( b, o.rebel_divisor )
    ;
}

bool write_binary( base::BinaryData& b, COLONY const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.name )
    && write_binary( b, o.nation_id )
    && write_binary( b, o.unknown08 )
    && write_binary( b, o.population )
    && write_binary( b, o.occupation )
    && write_binary( b, o.profession )
    && write_binary( b, o.duration )
    && write_binary( b, o.tiles )
    && write_binary( b, o.unknown10 )
    && write_binary( b, o.buildings )
    && write_binary( b, o.custom_house_flags )
    && write_binary( b, o.unknown11 )
    && write_binary( b, o.hammers )
    && write_binary( b, o.building_in_production )
    && write_binary( b, o.warehouse_level )
    && write_binary( b, o.unknown12 )
    && write_binary( b, o.stock )
    && write_binary( b, o.population_on_map )
    && write_binary( b, o.fortification_on_map )
    && write_binary( b, o.rebel_dividend )
    && write_binary( b, o.rebel_divisor )
    ;
}

/****************************************************************
** TransportChain
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, TransportChain& o ) {
  return true
    && read_binary( b, o.next_unit_idx )
    && read_binary( b, o.prev_unit_idx )
    ;
}

bool write_binary( base::BinaryData& b, TransportChain const& o ) {
  return true
    && write_binary( b, o.next_unit_idx )
    && write_binary( b, o.prev_unit_idx )
    ;
}

/****************************************************************
** UNIT
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, UNIT& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.type )
    && read_binary( b, o.nation_info )
    && read_binary( b, o.unknown15 )
    && read_binary( b, o.moves )
    && read_binary( b, o.origin_settlement )
    && read_binary( b, o.unknown16b )
    && read_binary( b, o.orders )
    && read_binary( b, o.goto_x )
    && read_binary( b, o.goto_y )
    && read_binary( b, o.unknown18 )
    && read_binary( b, o.holds_occupied )
    && read_binary( b, o.cargo_items )
    && read_binary( b, o.cargo_hold )
    && read_binary( b, o.turns_worked )
    && read_binary( b, o.profession_or_treasure_amount )
    && read_binary( b, o.transport_chain )
    ;
}

bool write_binary( base::BinaryData& b, UNIT const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.type )
    && write_binary( b, o.nation_info )
    && write_binary( b, o.unknown15 )
    && write_binary( b, o.moves )
    && write_binary( b, o.origin_settlement )
    && write_binary( b, o.unknown16b )
    && write_binary( b, o.orders )
    && write_binary( b, o.goto_x )
    && write_binary( b, o.goto_y )
    && write_binary( b, o.unknown18 )
    && write_binary( b, o.holds_occupied )
    && write_binary( b, o.cargo_items )
    && write_binary( b, o.cargo_hold )
    && write_binary( b, o.turns_worked )
    && write_binary( b, o.profession_or_treasure_amount )
    && write_binary( b, o.transport_chain )
    ;
}

/****************************************************************
** RelationByIndian
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, RelationByIndian& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.awarak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::BinaryData& b, RelationByIndian const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.awarak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

/****************************************************************
** Trade
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Trade& o ) {
  return true
    && read_binary( b, o.eu_prc )
    && read_binary( b, o.nr )
    && read_binary( b, o.gold )
    && read_binary( b, o.tons )
    && read_binary( b, o.tons2 )
    ;
}

bool write_binary( base::BinaryData& b, Trade const& o ) {
  return true
    && write_binary( b, o.eu_prc )
    && write_binary( b, o.nr )
    && write_binary( b, o.gold )
    && write_binary( b, o.tons )
    && write_binary( b, o.tons2 )
    ;
}

/****************************************************************
** NATION
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, NATION& o ) {
  return true
    && read_binary( b, o.unknown19 )
    && read_binary( b, o.tax_rate )
    && read_binary( b, o.recruit )
    && read_binary( b, o.unused07 )
    && read_binary( b, o.recruit_count )
    && read_binary( b, o.founding_fathers )
    && read_binary( b, o.unknown21 )
    && read_binary( b, o.liberty_bells_total )
    && read_binary( b, o.liberty_bells_last_turn )
    && read_binary( b, o.unknown22 )
    && read_binary( b, o.next_founding_father )
    && read_binary( b, o.founding_father_count )
    && read_binary( b, o.prob_founding_father_count_end )
    && read_binary( b, o.villages_burned )
    && read_binary( b, o.unknown23 )
    && read_binary( b, o.artillery_bought_count )
    && read_binary( b, o.boycott_bitmap )
    && read_binary( b, o.royal_money )
    && read_binary( b, o.unknown24b )
    && read_binary( b, o.gold )
    && read_binary( b, o.current_crosses )
    && read_binary( b, o.needed_crosses )
    && read_binary( b, o.point_return_from_europe )
    && read_binary( b, o.unknown25b )
    && read_binary( b, o.relation_by_indian )
    && read_binary( b, o.unknown26a )
    && read_binary( b, o.unknown26b )
    && read_binary( b, o.unknown26c )
    && read_binary( b, o.trade )
    ;
}

bool write_binary( base::BinaryData& b, NATION const& o ) {
  return true
    && write_binary( b, o.unknown19 )
    && write_binary( b, o.tax_rate )
    && write_binary( b, o.recruit )
    && write_binary( b, o.unused07 )
    && write_binary( b, o.recruit_count )
    && write_binary( b, o.founding_fathers )
    && write_binary( b, o.unknown21 )
    && write_binary( b, o.liberty_bells_total )
    && write_binary( b, o.liberty_bells_last_turn )
    && write_binary( b, o.unknown22 )
    && write_binary( b, o.next_founding_father )
    && write_binary( b, o.founding_father_count )
    && write_binary( b, o.prob_founding_father_count_end )
    && write_binary( b, o.villages_burned )
    && write_binary( b, o.unknown23 )
    && write_binary( b, o.artillery_bought_count )
    && write_binary( b, o.boycott_bitmap )
    && write_binary( b, o.royal_money )
    && write_binary( b, o.unknown24b )
    && write_binary( b, o.gold )
    && write_binary( b, o.current_crosses )
    && write_binary( b, o.needed_crosses )
    && write_binary( b, o.point_return_from_europe )
    && write_binary( b, o.unknown25b )
    && write_binary( b, o.relation_by_indian )
    && write_binary( b, o.unknown26a )
    && write_binary( b, o.unknown26b )
    && write_binary( b, o.unknown26c )
    && write_binary( b, o.trade )
    ;
}

/****************************************************************
** Alarm
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, Alarm& o ) {
  return true
    && read_binary( b, o.friction )
    && read_binary( b, o.attacks )
    ;
}

bool write_binary( base::BinaryData& b, Alarm const& o ) {
  return true
    && write_binary( b, o.friction )
    && write_binary( b, o.attacks )
    ;
}

/****************************************************************
** TRIBE
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, TRIBE& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.nation_id )
    && read_binary( b, o.alcs )
    && read_binary( b, o.population )
    && read_binary( b, o.mission )
    && read_binary( b, o.unknown28 )
    && read_binary( b, o.last_bought )
    && read_binary( b, o.last_sold )
    && read_binary( b, o.alarm )
    ;
}

bool write_binary( base::BinaryData& b, TRIBE const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.nation_id )
    && write_binary( b, o.alcs )
    && write_binary( b, o.population )
    && write_binary( b, o.mission )
    && write_binary( b, o.unknown28 )
    && write_binary( b, o.last_bought )
    && write_binary( b, o.last_sold )
    && write_binary( b, o.alarm )
    ;
}

/****************************************************************
** RelationByNations
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, RelationByNations& o ) {
  return true
    && read_binary( b, o.england )
    && read_binary( b, o.france )
    && read_binary( b, o.spain )
    && read_binary( b, o.netherlands )
    ;
}

bool write_binary( base::BinaryData& b, RelationByNations const& o ) {
  return true
    && write_binary( b, o.england )
    && write_binary( b, o.france )
    && write_binary( b, o.spain )
    && write_binary( b, o.netherlands )
    ;
}

/****************************************************************
** INDIAN
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, INDIAN& o ) {
  return true
    && read_binary( b, o.capitol_x_y )
    && read_binary( b, o.tech )
    && read_binary( b, o.unknown31a )
    && read_binary( b, o.muskets )
    && read_binary( b, o.horse_herds )
    && read_binary( b, o.unknown31b )
    && read_binary( b, o.tons )
    && read_binary( b, o.unknown32 )
    && read_binary( b, o.relation_by_nations )
    && read_binary( b, o.unknown33 )
    && read_binary( b, o.alarm_by_player )
    ;
}

bool write_binary( base::BinaryData& b, INDIAN const& o ) {
  return true
    && write_binary( b, o.capitol_x_y )
    && write_binary( b, o.tech )
    && write_binary( b, o.unknown31a )
    && write_binary( b, o.muskets )
    && write_binary( b, o.horse_herds )
    && write_binary( b, o.unknown31b )
    && write_binary( b, o.tons )
    && write_binary( b, o.unknown32 )
    && write_binary( b, o.relation_by_nations )
    && write_binary( b, o.unknown33 )
    && write_binary( b, o.alarm_by_player )
    ;
}

/****************************************************************
** STUFF
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, STUFF& o ) {
  return true
    && read_binary( b, o.unknown34 )
    && read_binary( b, o.counter_decreasing_on_new_colony )
    && read_binary( b, o.unknown35 )
    && read_binary( b, o.counter_increasing_on_new_colony )
    && read_binary( b, o.unknown36 )
    && read_binary( b, o.x )
    && read_binary( b, o.y )
    && read_binary( b, o.zoom_level )
    && read_binary( b, o.unknown37 )
    && read_binary( b, o.viewport_x )
    && read_binary( b, o.viewport_y )
    ;
}

bool write_binary( base::BinaryData& b, STUFF const& o ) {
  return true
    && write_binary( b, o.unknown34 )
    && write_binary( b, o.counter_decreasing_on_new_colony )
    && write_binary( b, o.unknown35 )
    && write_binary( b, o.counter_increasing_on_new_colony )
    && write_binary( b, o.unknown36 )
    && write_binary( b, o.x )
    && write_binary( b, o.y )
    && write_binary( b, o.zoom_level )
    && write_binary( b, o.unknown37 )
    && write_binary( b, o.viewport_x )
    && write_binary( b, o.viewport_y )
    ;
}

/****************************************************************
** TRADEROUTE
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryData& b, TRADEROUTE& o ) {
  return true
    && read_binary( b, o.name )
    && read_binary( b, o.land_or_sea )
    && read_binary( b, o.stops_count )
    && read_binary( b, o.stop_1_colony_index )
    && read_binary( b, o.stop_1_loads_and_unloads_count )
    && read_binary( b, o.stop_1_loads_cargo )
    && read_binary( b, o.stop_1_unloads_cargo )
    && read_binary( b, o.unknown47 )
    && read_binary( b, o.stop_2_colony_index )
    && read_binary( b, o.stop_2_loads_and_unloads_count )
    && read_binary( b, o.stop_2_loads_cargo )
    && read_binary( b, o.stop_2_unloads_cargo )
    && read_binary( b, o.unknown48 )
    && read_binary( b, o.stop_3_colony_index )
    && read_binary( b, o.stop_3_loads_and_unloads_count )
    && read_binary( b, o.stop_3_loads_cargo )
    && read_binary( b, o.stop_3_unloads_cargo )
    && read_binary( b, o.unknown49 )
    && read_binary( b, o.stop_4_colony_index )
    && read_binary( b, o.stop_4_loads_and_unloads_count )
    && read_binary( b, o.stop_4_loads_cargo )
    && read_binary( b, o.stop_4_unloads_cargo )
    && read_binary( b, o.unknown50 )
    ;
}

bool write_binary( base::BinaryData& b, TRADEROUTE const& o ) {
  return true
    && write_binary( b, o.name )
    && write_binary( b, o.land_or_sea )
    && write_binary( b, o.stops_count )
    && write_binary( b, o.stop_1_colony_index )
    && write_binary( b, o.stop_1_loads_and_unloads_count )
    && write_binary( b, o.stop_1_loads_cargo )
    && write_binary( b, o.stop_1_unloads_cargo )
    && write_binary( b, o.unknown47 )
    && write_binary( b, o.stop_2_colony_index )
    && write_binary( b, o.stop_2_loads_and_unloads_count )
    && write_binary( b, o.stop_2_loads_cargo )
    && write_binary( b, o.stop_2_unloads_cargo )
    && write_binary( b, o.unknown48 )
    && write_binary( b, o.stop_3_colony_index )
    && write_binary( b, o.stop_3_loads_and_unloads_count )
    && write_binary( b, o.stop_3_loads_cargo )
    && write_binary( b, o.stop_3_unloads_cargo )
    && write_binary( b, o.unknown49 )
    && write_binary( b, o.stop_4_colony_index )
    && write_binary( b, o.stop_4_loads_and_unloads_count )
    && write_binary( b, o.stop_4_loads_cargo )
    && write_binary( b, o.stop_4_unloads_cargo )
    && write_binary( b, o.unknown50 )
    ;
}

/****************************************************************
** ColonySAV
*****************************************************************/
// NOTE: manually implemented.

}  // namespace sav
