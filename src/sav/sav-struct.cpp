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

#define CONV_FROM_FIELD( name, identifier )                       \
  UNWRAP_RETURN(                                                  \
      identifier, conv.from_field<                                \
                std::remove_cvref_t<decltype( res.identifier )>>( \
                tbl, name, used_keys ) );                         \
  res.identifier = std::move( identifier )

#define CONV_FROM_BITSTRING_FIELD( name, identifier, N ) \
  UNWRAP_RETURN( identifier, conv.from_field<bits<N>>(   \
                 tbl, name, used_keys ) );               \
  res.identifier = static_cast<std::remove_cvref_t<      \
                     decltype( res.identifier )          \
                   >>( identifier.n() );

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

cdr::result<cargo_4bit_type> from_canonical(
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

cdr::result<control_type> from_canonical(
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

cdr::result<difficulty_type> from_canonical(
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

cdr::result<fortification_level_type> from_canonical(
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

cdr::result<has_city_1bit_type> from_canonical(
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

cdr::result<has_unit_1bit_type> from_canonical(
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

cdr::result<hills_river_3bit_type> from_canonical(
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

cdr::result<level_2bit_type> from_canonical(
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

cdr::result<level_3bit_type> from_canonical(
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

cdr::result<nation_4bit_short_type> from_canonical(
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

cdr::result<nation_4bit_type> from_canonical(
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

cdr::result<nation_type> from_canonical(
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

cdr::result<occupation_type> from_canonical(
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

cdr::result<orders_type> from_canonical(
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

cdr::result<pacific_1bit_type> from_canonical(
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

cdr::result<plowed_1bit_type> from_canonical(
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

cdr::result<profession_type> from_canonical(
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

cdr::result<purchased_1bit_type> from_canonical(
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

cdr::result<region_id_4bit_type> from_canonical(
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

cdr::result<relation_type> from_canonical(
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

cdr::result<road_1bit_type> from_canonical(
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

cdr::result<season_type> from_canonical(
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

cdr::result<suppress_1bit_type> from_canonical(
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

cdr::result<tech_type> from_canonical(
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

cdr::result<terrain_5bit_type> from_canonical(
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

cdr::result<trade_route_type> from_canonical(
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

cdr::result<unit_type> from_canonical(
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

cdr::result<visible_to_dutch_1bit_type> from_canonical(
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

cdr::result<visible_to_english_1bit_type> from_canonical(
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

cdr::result<visible_to_french_1bit_type> from_canonical(
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

cdr::result<visible_to_spanish_1bit_type> from_canonical(
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

cdr::value to_canonical( cdr::converter& conv,
                         GameOptions const& o,
                         cdr::tag_t<GameOptions> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unused01", bits<7>{ o.unused01 } );
  conv.to_field( tbl, "tutorial_hints", o.tutorial_hints );
  conv.to_field( tbl, "water_color_cycling", o.water_color_cycling );
  conv.to_field( tbl, "combat_analysis", o.combat_analysis );
  conv.to_field( tbl, "autosave", o.autosave );
  conv.to_field( tbl, "end_of_turn", o.end_of_turn );
  conv.to_field( tbl, "fast_piece_slide", o.fast_piece_slide );
  conv.to_field( tbl, "cheats_enabled", o.cheats_enabled );
  conv.to_field( tbl, "show_foreign_moves", o.show_foreign_moves );
  conv.to_field( tbl, "show_indian_moves", o.show_indian_moves );
  tbl["__key_order"] = cdr::list{
    "unused01",
    "tutorial_hints",
    "water_color_cycling",
    "combat_analysis",
    "autosave",
    "end_of_turn",
    "fast_piece_slide",
    "cheats_enabled",
    "show_foreign_moves",
    "show_indian_moves",
  };
  return tbl;
}

cdr::result<GameOptions> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<GameOptions> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  GameOptions res = {};
  std::set<std::string> used_keys;
  CONV_FROM_BITSTRING_FIELD( "unused01", unused01, 7 );
  CONV_FROM_FIELD( "tutorial_hints", tutorial_hints );
  CONV_FROM_FIELD( "water_color_cycling", water_color_cycling );
  CONV_FROM_FIELD( "combat_analysis", combat_analysis );
  CONV_FROM_FIELD( "autosave", autosave );
  CONV_FROM_FIELD( "end_of_turn", end_of_turn );
  CONV_FROM_FIELD( "fast_piece_slide", fast_piece_slide );
  CONV_FROM_FIELD( "cheats_enabled", cheats_enabled );
  CONV_FROM_FIELD( "show_foreign_moves", show_foreign_moves );
  CONV_FROM_FIELD( "show_indian_moves", show_indian_moves );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         ColonyReportOptions const& o,
                         cdr::tag_t<ColonyReportOptions> ) {
  cdr::table tbl;
  conv.to_field( tbl, "labels_on_cargo_and_terrain", o.labels_on_cargo_and_terrain );
  conv.to_field( tbl, "labels_on_buildings", o.labels_on_buildings );
  conv.to_field( tbl, "report_new_cargos_available", o.report_new_cargos_available );
  conv.to_field( tbl, "report_inefficient_government", o.report_inefficient_government );
  conv.to_field( tbl, "report_tools_needed_for_production", o.report_tools_needed_for_production );
  conv.to_field( tbl, "report_raw_materials_shortages", o.report_raw_materials_shortages );
  conv.to_field( tbl, "report_food_shortages", o.report_food_shortages );
  conv.to_field( tbl, "report_when_colonists_trained", o.report_when_colonists_trained );
  conv.to_field( tbl, "report_sons_of_liberty_membership", o.report_sons_of_liberty_membership );
  conv.to_field( tbl, "report_rebel_majorities", o.report_rebel_majorities );
  conv.to_field( tbl, "unused03", bits<6>{ o.unused03 } );
  tbl["__key_order"] = cdr::list{
    "labels_on_cargo_and_terrain",
    "labels_on_buildings",
    "report_new_cargos_available",
    "report_inefficient_government",
    "report_tools_needed_for_production",
    "report_raw_materials_shortages",
    "report_food_shortages",
    "report_when_colonists_trained",
    "report_sons_of_liberty_membership",
    "report_rebel_majorities",
    "unused03",
  };
  return tbl;
}

cdr::result<ColonyReportOptions> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonyReportOptions> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ColonyReportOptions res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "labels_on_cargo_and_terrain", labels_on_cargo_and_terrain );
  CONV_FROM_FIELD( "labels_on_buildings", labels_on_buildings );
  CONV_FROM_FIELD( "report_new_cargos_available", report_new_cargos_available );
  CONV_FROM_FIELD( "report_inefficient_government", report_inefficient_government );
  CONV_FROM_FIELD( "report_tools_needed_for_production", report_tools_needed_for_production );
  CONV_FROM_FIELD( "report_raw_materials_shortages", report_raw_materials_shortages );
  CONV_FROM_FIELD( "report_food_shortages", report_food_shortages );
  CONV_FROM_FIELD( "report_when_colonists_trained", report_when_colonists_trained );
  CONV_FROM_FIELD( "report_sons_of_liberty_membership", report_sons_of_liberty_membership );
  CONV_FROM_FIELD( "report_rebel_majorities", report_rebel_majorities );
  CONV_FROM_BITSTRING_FIELD( "unused03", unused03, 6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Event const& o,
                         cdr::tag_t<Event> ) {
  cdr::table tbl;
  conv.to_field( tbl, "discovery_of_the_new_world", o.discovery_of_the_new_world );
  conv.to_field( tbl, "building_a_colony", o.building_a_colony );
  conv.to_field( tbl, "meeting_the_natives", o.meeting_the_natives );
  conv.to_field( tbl, "the_aztec_empire", o.the_aztec_empire );
  conv.to_field( tbl, "the_inca_nation", o.the_inca_nation );
  conv.to_field( tbl, "discovery_of_the_pacific_ocean", o.discovery_of_the_pacific_ocean );
  conv.to_field( tbl, "entering_indian_village", o.entering_indian_village );
  conv.to_field( tbl, "the_fountain_of_youth", o.the_fountain_of_youth );
  conv.to_field( tbl, "cargo_from_the_new_world", o.cargo_from_the_new_world );
  conv.to_field( tbl, "meeting_fellow_europeans", o.meeting_fellow_europeans );
  conv.to_field( tbl, "colony_burning", o.colony_burning );
  conv.to_field( tbl, "colony_destroyed", o.colony_destroyed );
  conv.to_field( tbl, "indian_raid", o.indian_raid );
  conv.to_field( tbl, "woodcut14", o.woodcut14 );
  conv.to_field( tbl, "woodcut15", o.woodcut15 );
  conv.to_field( tbl, "woodcut16", o.woodcut16 );
  tbl["__key_order"] = cdr::list{
    "discovery_of_the_new_world",
    "building_a_colony",
    "meeting_the_natives",
    "the_aztec_empire",
    "the_inca_nation",
    "discovery_of_the_pacific_ocean",
    "entering_indian_village",
    "the_fountain_of_youth",
    "cargo_from_the_new_world",
    "meeting_fellow_europeans",
    "colony_burning",
    "colony_destroyed",
    "indian_raid",
    "woodcut14",
    "woodcut15",
    "woodcut16",
  };
  return tbl;
}

cdr::result<Event> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Event> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Event res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "discovery_of_the_new_world", discovery_of_the_new_world );
  CONV_FROM_FIELD( "building_a_colony", building_a_colony );
  CONV_FROM_FIELD( "meeting_the_natives", meeting_the_natives );
  CONV_FROM_FIELD( "the_aztec_empire", the_aztec_empire );
  CONV_FROM_FIELD( "the_inca_nation", the_inca_nation );
  CONV_FROM_FIELD( "discovery_of_the_pacific_ocean", discovery_of_the_pacific_ocean );
  CONV_FROM_FIELD( "entering_indian_village", entering_indian_village );
  CONV_FROM_FIELD( "the_fountain_of_youth", the_fountain_of_youth );
  CONV_FROM_FIELD( "cargo_from_the_new_world", cargo_from_the_new_world );
  CONV_FROM_FIELD( "meeting_fellow_europeans", meeting_fellow_europeans );
  CONV_FROM_FIELD( "colony_burning", colony_burning );
  CONV_FROM_FIELD( "colony_destroyed", colony_destroyed );
  CONV_FROM_FIELD( "indian_raid", indian_raid );
  CONV_FROM_FIELD( "woodcut14", woodcut14 );
  CONV_FROM_FIELD( "woodcut15", woodcut15 );
  CONV_FROM_FIELD( "woodcut16", woodcut16 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Duration const& o,
                         cdr::tag_t<Duration> ) {
  cdr::table tbl;
  conv.to_field( tbl, "dur_1", o.dur_1 );
  conv.to_field( tbl, "dur_2", o.dur_2 );
  tbl["__key_order"] = cdr::list{
    "dur_1",
    "dur_2",
  };
  return tbl;
}

cdr::result<Duration> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Duration> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Duration res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "dur_1", dur_1 );
  CONV_FROM_FIELD( "dur_2", dur_2 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Buildings const& o,
                         cdr::tag_t<Buildings> ) {
  cdr::table tbl;
  conv.to_field( tbl, "fortification", o.fortification );
  conv.to_field( tbl, "armory", o.armory );
  conv.to_field( tbl, "docks", o.docks );
  conv.to_field( tbl, "town_hall", o.town_hall );
  conv.to_field( tbl, "schoolhouse", o.schoolhouse );
  conv.to_field( tbl, "warehouse", o.warehouse );
  conv.to_field( tbl, "unused05a", o.unused05a );
  conv.to_field( tbl, "stables", o.stables );
  conv.to_field( tbl, "custom_house", o.custom_house );
  conv.to_field( tbl, "printing_press", o.printing_press );
  conv.to_field( tbl, "weavers_house", o.weavers_house );
  conv.to_field( tbl, "tobacconists_house", o.tobacconists_house );
  conv.to_field( tbl, "rum_distillers_house", o.rum_distillers_house );
  conv.to_field( tbl, "capitol (unused)", o.capitol_unused );
  conv.to_field( tbl, "fur_traders_house", o.fur_traders_house );
  conv.to_field( tbl, "carpenters_shop", o.carpenters_shop );
  conv.to_field( tbl, "church", o.church );
  conv.to_field( tbl, "blacksmiths_house", o.blacksmiths_house );
  conv.to_field( tbl, "unused05b", bits<6>{ o.unused05b } );
  tbl["__key_order"] = cdr::list{
    "fortification",
    "armory",
    "docks",
    "town_hall",
    "schoolhouse",
    "warehouse",
    "unused05a",
    "stables",
    "custom_house",
    "printing_press",
    "weavers_house",
    "tobacconists_house",
    "rum_distillers_house",
    "capitol (unused)",
    "fur_traders_house",
    "carpenters_shop",
    "church",
    "blacksmiths_house",
    "unused05b",
  };
  return tbl;
}

cdr::result<Buildings> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Buildings> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Buildings res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "fortification", fortification );
  CONV_FROM_FIELD( "armory", armory );
  CONV_FROM_FIELD( "docks", docks );
  CONV_FROM_FIELD( "town_hall", town_hall );
  CONV_FROM_FIELD( "schoolhouse", schoolhouse );
  CONV_FROM_FIELD( "warehouse", warehouse );
  CONV_FROM_FIELD( "unused05a", unused05a );
  CONV_FROM_FIELD( "stables", stables );
  CONV_FROM_FIELD( "custom_house", custom_house );
  CONV_FROM_FIELD( "printing_press", printing_press );
  CONV_FROM_FIELD( "weavers_house", weavers_house );
  CONV_FROM_FIELD( "tobacconists_house", tobacconists_house );
  CONV_FROM_FIELD( "rum_distillers_house", rum_distillers_house );
  CONV_FROM_FIELD( "capitol (unused)", capitol_unused );
  CONV_FROM_FIELD( "fur_traders_house", fur_traders_house );
  CONV_FROM_FIELD( "carpenters_shop", carpenters_shop );
  CONV_FROM_FIELD( "church", church );
  CONV_FROM_FIELD( "blacksmiths_house", blacksmiths_house );
  CONV_FROM_BITSTRING_FIELD( "unused05b", unused05b, 6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         CustomHouseFlags const& o,
                         cdr::tag_t<CustomHouseFlags> ) {
  cdr::table tbl;
  conv.to_field( tbl, "food", o.food );
  conv.to_field( tbl, "sugar", o.sugar );
  conv.to_field( tbl, "tobacco", o.tobacco );
  conv.to_field( tbl, "cotton", o.cotton );
  conv.to_field( tbl, "furs", o.furs );
  conv.to_field( tbl, "lumber", o.lumber );
  conv.to_field( tbl, "ore", o.ore );
  conv.to_field( tbl, "silver", o.silver );
  conv.to_field( tbl, "horses", o.horses );
  conv.to_field( tbl, "rum", o.rum );
  conv.to_field( tbl, "cigars", o.cigars );
  conv.to_field( tbl, "cloth", o.cloth );
  conv.to_field( tbl, "coats", o.coats );
  conv.to_field( tbl, "trade_goods", o.trade_goods );
  conv.to_field( tbl, "tools", o.tools );
  conv.to_field( tbl, "muskets", o.muskets );
  tbl["__key_order"] = cdr::list{
    "food",
    "sugar",
    "tobacco",
    "cotton",
    "furs",
    "lumber",
    "ore",
    "silver",
    "horses",
    "rum",
    "cigars",
    "cloth",
    "coats",
    "trade_goods",
    "tools",
    "muskets",
  };
  return tbl;
}

cdr::result<CustomHouseFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CustomHouseFlags> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  CustomHouseFlags res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "food", food );
  CONV_FROM_FIELD( "sugar", sugar );
  CONV_FROM_FIELD( "tobacco", tobacco );
  CONV_FROM_FIELD( "cotton", cotton );
  CONV_FROM_FIELD( "furs", furs );
  CONV_FROM_FIELD( "lumber", lumber );
  CONV_FROM_FIELD( "ore", ore );
  CONV_FROM_FIELD( "silver", silver );
  CONV_FROM_FIELD( "horses", horses );
  CONV_FROM_FIELD( "rum", rum );
  CONV_FROM_FIELD( "cigars", cigars );
  CONV_FROM_FIELD( "cloth", cloth );
  CONV_FROM_FIELD( "coats", coats );
  CONV_FROM_FIELD( "trade_goods", trade_goods );
  CONV_FROM_FIELD( "tools", tools );
  CONV_FROM_FIELD( "muskets", muskets );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         NationInfo const& o,
                         cdr::tag_t<NationInfo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "nation_id", o.nation_id );
  conv.to_field( tbl, "vis_to_english", o.vis_to_english );
  conv.to_field( tbl, "vis_to_french", o.vis_to_french );
  conv.to_field( tbl, "vis_to_spanish", o.vis_to_spanish );
  conv.to_field( tbl, "vis_to_dutch", o.vis_to_dutch );
  tbl["__key_order"] = cdr::list{
    "nation_id",
    "vis_to_english",
    "vis_to_french",
    "vis_to_spanish",
    "vis_to_dutch",
  };
  return tbl;
}

cdr::result<NationInfo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationInfo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  NationInfo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "nation_id", nation_id );
  CONV_FROM_FIELD( "vis_to_english", vis_to_english );
  CONV_FROM_FIELD( "vis_to_french", vis_to_french );
  CONV_FROM_FIELD( "vis_to_spanish", vis_to_spanish );
  CONV_FROM_FIELD( "vis_to_dutch", vis_to_dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Unknown15 const& o,
                         cdr::tag_t<Unknown15> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown15a", bits<7>{ o.unknown15a } );
  conv.to_field( tbl, "damaged", o.damaged );
  tbl["__key_order"] = cdr::list{
    "unknown15a",
    "damaged",
  };
  return tbl;
}

cdr::result<Unknown15> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown15> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Unknown15 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_BITSTRING_FIELD( "unknown15a", unknown15a, 7 );
  CONV_FROM_FIELD( "damaged", damaged );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         CargoItems const& o,
                         cdr::tag_t<CargoItems> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
  };
  return tbl;
}

cdr::result<CargoItems> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CargoItems> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  CargoItems res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         BoycottBitmap const& o,
                         cdr::tag_t<BoycottBitmap> ) {
  cdr::table tbl;
  conv.to_field( tbl, "food", o.food );
  conv.to_field( tbl, "sugar", o.sugar );
  conv.to_field( tbl, "tobacco", o.tobacco );
  conv.to_field( tbl, "cotton", o.cotton );
  conv.to_field( tbl, "furs", o.furs );
  conv.to_field( tbl, "lumber", o.lumber );
  conv.to_field( tbl, "ore", o.ore );
  conv.to_field( tbl, "silver", o.silver );
  conv.to_field( tbl, "horses", o.horses );
  conv.to_field( tbl, "rum", o.rum );
  conv.to_field( tbl, "cigars", o.cigars );
  conv.to_field( tbl, "cloth", o.cloth );
  conv.to_field( tbl, "coats", o.coats );
  conv.to_field( tbl, "trade_goods", o.trade_goods );
  conv.to_field( tbl, "tools", o.tools );
  conv.to_field( tbl, "muskets", o.muskets );
  tbl["__key_order"] = cdr::list{
    "food",
    "sugar",
    "tobacco",
    "cotton",
    "furs",
    "lumber",
    "ore",
    "silver",
    "horses",
    "rum",
    "cigars",
    "cloth",
    "coats",
    "trade_goods",
    "tools",
    "muskets",
  };
  return tbl;
}

cdr::result<BoycottBitmap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BoycottBitmap> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  BoycottBitmap res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "food", food );
  CONV_FROM_FIELD( "sugar", sugar );
  CONV_FROM_FIELD( "tobacco", tobacco );
  CONV_FROM_FIELD( "cotton", cotton );
  CONV_FROM_FIELD( "furs", furs );
  CONV_FROM_FIELD( "lumber", lumber );
  CONV_FROM_FIELD( "ore", ore );
  CONV_FROM_FIELD( "silver", silver );
  CONV_FROM_FIELD( "horses", horses );
  CONV_FROM_FIELD( "rum", rum );
  CONV_FROM_FIELD( "cigars", cigars );
  CONV_FROM_FIELD( "cloth", cloth );
  CONV_FROM_FIELD( "coats", coats );
  CONV_FROM_FIELD( "trade_goods", trade_goods );
  CONV_FROM_FIELD( "tools", tools );
  CONV_FROM_FIELD( "muskets", muskets );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         ALCS const& o,
                         cdr::tag_t<ALCS> ) {
  cdr::table tbl;
  conv.to_field( tbl, "artillery_near", o.artillery_near );
  conv.to_field( tbl, "learned", o.learned );
  conv.to_field( tbl, "capital", o.capital );
  conv.to_field( tbl, "scouted", o.scouted );
  conv.to_field( tbl, "unused09", bits<4>{ o.unused09 } );
  tbl["__key_order"] = cdr::list{
    "artillery_near",
    "learned",
    "capital",
    "scouted",
    "unused09",
  };
  return tbl;
}

cdr::result<ALCS> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ALCS> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ALCS res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "artillery_near", artillery_near );
  CONV_FROM_FIELD( "learned", learned );
  CONV_FROM_FIELD( "capital", capital );
  CONV_FROM_FIELD( "scouted", scouted );
  CONV_FROM_BITSTRING_FIELD( "unused09", unused09, 4 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         TILE const& o,
                         cdr::tag_t<TILE> ) {
  cdr::table tbl;
  conv.to_field( tbl, "tile", o.tile );
  conv.to_field( tbl, "hill_river", o.hill_river );
  tbl["__key_order"] = cdr::list{
    "tile",
    "hill_river",
  };
  return tbl;
}

cdr::result<TILE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TILE> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TILE res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "tile", tile );
  CONV_FROM_FIELD( "hill_river", hill_river );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         MASK const& o,
                         cdr::tag_t<MASK> ) {
  cdr::table tbl;
  conv.to_field( tbl, "has_unit", o.has_unit );
  conv.to_field( tbl, "has_city", o.has_city );
  conv.to_field( tbl, "suppress", o.suppress );
  conv.to_field( tbl, "road", o.road );
  conv.to_field( tbl, "purchased", o.purchased );
  conv.to_field( tbl, "pacific", o.pacific );
  conv.to_field( tbl, "plowed", o.plowed );
  conv.to_field( tbl, "unused", o.unused );
  tbl["__key_order"] = cdr::list{
    "has_unit",
    "has_city",
    "suppress",
    "road",
    "purchased",
    "pacific",
    "plowed",
    "unused",
  };
  return tbl;
}

cdr::result<MASK> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<MASK> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  MASK res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "has_unit", has_unit );
  CONV_FROM_FIELD( "has_city", has_city );
  CONV_FROM_FIELD( "suppress", suppress );
  CONV_FROM_FIELD( "road", road );
  CONV_FROM_FIELD( "purchased", purchased );
  CONV_FROM_FIELD( "pacific", pacific );
  CONV_FROM_FIELD( "plowed", plowed );
  CONV_FROM_FIELD( "unused", unused );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         PATH const& o,
                         cdr::tag_t<PATH> ) {
  cdr::table tbl;
  conv.to_field( tbl, "region_id", o.region_id );
  conv.to_field( tbl, "visitor_nation", o.visitor_nation );
  tbl["__key_order"] = cdr::list{
    "region_id",
    "visitor_nation",
  };
  return tbl;
}

cdr::result<PATH> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PATH> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  PATH res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "region_id", region_id );
  CONV_FROM_FIELD( "visitor_nation", visitor_nation );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         SEEN const& o,
                         cdr::tag_t<SEEN> ) {
  cdr::table tbl;
  conv.to_field( tbl, "score", o.score );
  conv.to_field( tbl, "vis2en", o.vis2en );
  conv.to_field( tbl, "vis2fr", o.vis2fr );
  conv.to_field( tbl, "vis2sp", o.vis2sp );
  conv.to_field( tbl, "vis2du", o.vis2du );
  tbl["__key_order"] = cdr::list{
    "score",
    "vis2en",
    "vis2fr",
    "vis2sp",
    "vis2du",
  };
  return tbl;
}

cdr::result<SEEN> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<SEEN> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  SEEN res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "score", score );
  CONV_FROM_FIELD( "vis2en", vis2en );
  CONV_FROM_FIELD( "vis2fr", vis2fr );
  CONV_FROM_FIELD( "vis2sp", vis2sp );
  CONV_FROM_FIELD( "vis2du", vis2du );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop1LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop1LoadsAndUnloadsCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unloads_count", o.unloads_count );
  conv.to_field( tbl, "loads_count", o.loads_count );
  tbl["__key_order"] = cdr::list{
    "unloads_count",
    "loads_count",
  };
  return tbl;
}

cdr::result<Stop1LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1LoadsAndUnloadsCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop1LoadsAndUnloadsCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unloads_count", unloads_count );
  CONV_FROM_FIELD( "loads_count", loads_count );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop1LoadsCargo const& o,
                         cdr::tag_t<Stop1LoadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop1LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1LoadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop1LoadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop1UnloadsCargo const& o,
                         cdr::tag_t<Stop1UnloadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop1UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1UnloadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop1UnloadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop2LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop2LoadsAndUnloadsCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unloads_count", o.unloads_count );
  conv.to_field( tbl, "loads_count", o.loads_count );
  tbl["__key_order"] = cdr::list{
    "unloads_count",
    "loads_count",
  };
  return tbl;
}

cdr::result<Stop2LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2LoadsAndUnloadsCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop2LoadsAndUnloadsCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unloads_count", unloads_count );
  CONV_FROM_FIELD( "loads_count", loads_count );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop2LoadsCargo const& o,
                         cdr::tag_t<Stop2LoadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop2LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2LoadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop2LoadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop2UnloadsCargo const& o,
                         cdr::tag_t<Stop2UnloadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop2UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2UnloadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop2UnloadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop3LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop3LoadsAndUnloadsCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unloads_count", o.unloads_count );
  conv.to_field( tbl, "loads_count", o.loads_count );
  tbl["__key_order"] = cdr::list{
    "unloads_count",
    "loads_count",
  };
  return tbl;
}

cdr::result<Stop3LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3LoadsAndUnloadsCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop3LoadsAndUnloadsCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unloads_count", unloads_count );
  CONV_FROM_FIELD( "loads_count", loads_count );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop3LoadsCargo const& o,
                         cdr::tag_t<Stop3LoadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop3LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3LoadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop3LoadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop3UnloadsCargo const& o,
                         cdr::tag_t<Stop3UnloadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop3UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3UnloadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop3UnloadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop4LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop4LoadsAndUnloadsCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unloads_count", o.unloads_count );
  conv.to_field( tbl, "loads_count", o.loads_count );
  tbl["__key_order"] = cdr::list{
    "unloads_count",
    "loads_count",
  };
  return tbl;
}

cdr::result<Stop4LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4LoadsAndUnloadsCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop4LoadsAndUnloadsCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unloads_count", unloads_count );
  CONV_FROM_FIELD( "loads_count", loads_count );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop4LoadsCargo const& o,
                         cdr::tag_t<Stop4LoadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop4LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4LoadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop4LoadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stop4UnloadsCargo const& o,
                         cdr::tag_t<Stop4UnloadsCargo> ) {
  cdr::table tbl;
  conv.to_field( tbl, "cargo_1", o.cargo_1 );
  conv.to_field( tbl, "cargo_2", o.cargo_2 );
  conv.to_field( tbl, "cargo_3", o.cargo_3 );
  conv.to_field( tbl, "cargo_4", o.cargo_4 );
  conv.to_field( tbl, "cargo_5", o.cargo_5 );
  conv.to_field( tbl, "cargo_6", o.cargo_6 );
  tbl["__key_order"] = cdr::list{
    "cargo_1",
    "cargo_2",
    "cargo_3",
    "cargo_4",
    "cargo_5",
    "cargo_6",
  };
  return tbl;
}

cdr::result<Stop4UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4UnloadsCargo> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stop4UnloadsCargo res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "cargo_1", cargo_1 );
  CONV_FROM_FIELD( "cargo_2", cargo_2 );
  CONV_FROM_FIELD( "cargo_3", cargo_3 );
  CONV_FROM_FIELD( "cargo_4", cargo_4 );
  CONV_FROM_FIELD( "cargo_5", cargo_5 );
  CONV_FROM_FIELD( "cargo_6", cargo_6 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         ExpeditionaryForce const& o,
                         cdr::tag_t<ExpeditionaryForce> ) {
  cdr::table tbl;
  conv.to_field( tbl, "regulars", o.regulars );
  conv.to_field( tbl, "dragoons", o.dragoons );
  conv.to_field( tbl, "man-o-wars", o.man_o_wars );
  conv.to_field( tbl, "artillery", o.artillery );
  tbl["__key_order"] = cdr::list{
    "regulars",
    "dragoons",
    "man-o-wars",
    "artillery",
  };
  return tbl;
}

cdr::result<ExpeditionaryForce> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ExpeditionaryForce> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ExpeditionaryForce res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "regulars", regulars );
  CONV_FROM_FIELD( "dragoons", dragoons );
  CONV_FROM_FIELD( "man-o-wars", man_o_wars );
  CONV_FROM_FIELD( "artillery", artillery );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         BackupForce const& o,
                         cdr::tag_t<BackupForce> ) {
  cdr::table tbl;
  conv.to_field( tbl, "regulars", o.regulars );
  conv.to_field( tbl, "dragoons", o.dragoons );
  conv.to_field( tbl, "man-o-wars", o.man_o_wars );
  conv.to_field( tbl, "artillery", o.artillery );
  tbl["__key_order"] = cdr::list{
    "regulars",
    "dragoons",
    "man-o-wars",
    "artillery",
  };
  return tbl;
}

cdr::result<BackupForce> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BackupForce> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  BackupForce res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "regulars", regulars );
  CONV_FROM_FIELD( "dragoons", dragoons );
  CONV_FROM_FIELD( "man-o-wars", man_o_wars );
  CONV_FROM_FIELD( "artillery", artillery );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         PriceGroupState const& o,
                         cdr::tag_t<PriceGroupState> ) {
  cdr::table tbl;
  conv.to_field( tbl, "food", o.food );
  conv.to_field( tbl, "sugar", o.sugar );
  conv.to_field( tbl, "tobacco", o.tobacco );
  conv.to_field( tbl, "cotton", o.cotton );
  conv.to_field( tbl, "furs", o.furs );
  conv.to_field( tbl, "lumber", o.lumber );
  conv.to_field( tbl, "ore", o.ore );
  conv.to_field( tbl, "silver", o.silver );
  conv.to_field( tbl, "horses", o.horses );
  conv.to_field( tbl, "rum", o.rum );
  conv.to_field( tbl, "cigars", o.cigars );
  conv.to_field( tbl, "cloth", o.cloth );
  conv.to_field( tbl, "coats", o.coats );
  conv.to_field( tbl, "trade_goods", o.trade_goods );
  conv.to_field( tbl, "tools", o.tools );
  conv.to_field( tbl, "muskets", o.muskets );
  tbl["__key_order"] = cdr::list{
    "food",
    "sugar",
    "tobacco",
    "cotton",
    "furs",
    "lumber",
    "ore",
    "silver",
    "horses",
    "rum",
    "cigars",
    "cloth",
    "coats",
    "trade_goods",
    "tools",
    "muskets",
  };
  return tbl;
}

cdr::result<PriceGroupState> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PriceGroupState> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  PriceGroupState res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "food", food );
  CONV_FROM_FIELD( "sugar", sugar );
  CONV_FROM_FIELD( "tobacco", tobacco );
  CONV_FROM_FIELD( "cotton", cotton );
  CONV_FROM_FIELD( "furs", furs );
  CONV_FROM_FIELD( "lumber", lumber );
  CONV_FROM_FIELD( "ore", ore );
  CONV_FROM_FIELD( "silver", silver );
  CONV_FROM_FIELD( "horses", horses );
  CONV_FROM_FIELD( "rum", rum );
  CONV_FROM_FIELD( "cigars", cigars );
  CONV_FROM_FIELD( "cloth", cloth );
  CONV_FROM_FIELD( "coats", coats );
  CONV_FROM_FIELD( "trade_goods", trade_goods );
  CONV_FROM_FIELD( "tools", tools );
  CONV_FROM_FIELD( "muskets", muskets );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         HEAD const& o,
                         cdr::tag_t<HEAD> ) {
  cdr::table tbl;
  conv.to_field( tbl, "colonize", o.colonize );
  conv.to_field( tbl, "unknown00", o.unknown00 );
  conv.to_field( tbl, "map_size_x", o.map_size_x );
  conv.to_field( tbl, "map_size_y", o.map_size_y );
  conv.to_field( tbl, "tut1", o.tut1 );
  conv.to_field( tbl, "unknown03", o.unknown03 );
  conv.to_field( tbl, "game_options", o.game_options );
  conv.to_field( tbl, "colony_report_options", o.colony_report_options );
  conv.to_field( tbl, "tut2", o.tut2 );
  conv.to_field( tbl, "tut3", o.tut3 );
  conv.to_field( tbl, "unknown39", o.unknown39 );
  conv.to_field( tbl, "year", o.year );
  conv.to_field( tbl, "season", o.season );
  conv.to_field( tbl, "turn", o.turn );
  conv.to_field( tbl, "tile_selection_mode", o.tile_selection_mode );
  conv.to_field( tbl, "unknown40", o.unknown40 );
  conv.to_field( tbl, "active_unit", o.active_unit );
  conv.to_field( tbl, "unknown41", o.unknown41 );
  conv.to_field( tbl, "tribe_count", o.tribe_count );
  conv.to_field( tbl, "unit_count", o.unit_count );
  conv.to_field( tbl, "colony_count", o.colony_count );
  conv.to_field( tbl, "trade_route_count", o.trade_route_count );
  conv.to_field( tbl, "unknown42", o.unknown42 );
  conv.to_field( tbl, "difficulty", o.difficulty );
  conv.to_field( tbl, "unknown43a", o.unknown43a );
  conv.to_field( tbl, "unknown43b", o.unknown43b );
  conv.to_field( tbl, "founding_father", o.founding_father );
  conv.to_field( tbl, "unknown44", o.unknown44 );
  conv.to_field( tbl, "nation_relation", o.nation_relation );
  conv.to_field( tbl, "unknown45", o.unknown45 );
  conv.to_field( tbl, "expeditionary_force", o.expeditionary_force );
  conv.to_field( tbl, "backup_force", o.backup_force );
  conv.to_field( tbl, "price_group_state", o.price_group_state );
  conv.to_field( tbl, "event", o.event );
  conv.to_field( tbl, "unknown05", o.unknown05 );
  tbl["__key_order"] = cdr::list{
    "colonize",
    "unknown00",
    "map_size_x",
    "map_size_y",
    "tut1",
    "unknown03",
    "game_options",
    "colony_report_options",
    "tut2",
    "tut3",
    "unknown39",
    "year",
    "season",
    "turn",
    "tile_selection_mode",
    "unknown40",
    "active_unit",
    "unknown41",
    "tribe_count",
    "unit_count",
    "colony_count",
    "trade_route_count",
    "unknown42",
    "difficulty",
    "unknown43a",
    "unknown43b",
    "founding_father",
    "unknown44",
    "nation_relation",
    "unknown45",
    "expeditionary_force",
    "backup_force",
    "price_group_state",
    "event",
    "unknown05",
  };
  return tbl;
}

cdr::result<HEAD> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<HEAD> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  HEAD res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "colonize", colonize );
  CONV_FROM_FIELD( "unknown00", unknown00 );
  CONV_FROM_FIELD( "map_size_x", map_size_x );
  CONV_FROM_FIELD( "map_size_y", map_size_y );
  CONV_FROM_FIELD( "tut1", tut1 );
  CONV_FROM_FIELD( "unknown03", unknown03 );
  CONV_FROM_FIELD( "game_options", game_options );
  CONV_FROM_FIELD( "colony_report_options", colony_report_options );
  CONV_FROM_FIELD( "tut2", tut2 );
  CONV_FROM_FIELD( "tut3", tut3 );
  CONV_FROM_FIELD( "unknown39", unknown39 );
  CONV_FROM_FIELD( "year", year );
  CONV_FROM_FIELD( "season", season );
  CONV_FROM_FIELD( "turn", turn );
  CONV_FROM_FIELD( "tile_selection_mode", tile_selection_mode );
  CONV_FROM_FIELD( "unknown40", unknown40 );
  CONV_FROM_FIELD( "active_unit", active_unit );
  CONV_FROM_FIELD( "unknown41", unknown41 );
  CONV_FROM_FIELD( "tribe_count", tribe_count );
  CONV_FROM_FIELD( "unit_count", unit_count );
  CONV_FROM_FIELD( "colony_count", colony_count );
  CONV_FROM_FIELD( "trade_route_count", trade_route_count );
  CONV_FROM_FIELD( "unknown42", unknown42 );
  CONV_FROM_FIELD( "difficulty", difficulty );
  CONV_FROM_FIELD( "unknown43a", unknown43a );
  CONV_FROM_FIELD( "unknown43b", unknown43b );
  CONV_FROM_FIELD( "founding_father", founding_father );
  CONV_FROM_FIELD( "unknown44", unknown44 );
  CONV_FROM_FIELD( "nation_relation", nation_relation );
  CONV_FROM_FIELD( "unknown45", unknown45 );
  CONV_FROM_FIELD( "expeditionary_force", expeditionary_force );
  CONV_FROM_FIELD( "backup_force", backup_force );
  CONV_FROM_FIELD( "price_group_state", price_group_state );
  CONV_FROM_FIELD( "event", event );
  CONV_FROM_FIELD( "unknown05", unknown05 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         PLAYER const& o,
                         cdr::tag_t<PLAYER> ) {
  cdr::table tbl;
  conv.to_field( tbl, "name", o.name );
  conv.to_field( tbl, "country_name", o.country_name );
  conv.to_field( tbl, "unknown06", o.unknown06 );
  conv.to_field( tbl, "control", o.control );
  conv.to_field( tbl, "founded_colonies", o.founded_colonies );
  conv.to_field( tbl, "diplomacy", o.diplomacy );
  tbl["__key_order"] = cdr::list{
    "name",
    "country_name",
    "unknown06",
    "control",
    "founded_colonies",
    "diplomacy",
  };
  return tbl;
}

cdr::result<PLAYER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PLAYER> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  PLAYER res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "name", name );
  CONV_FROM_FIELD( "country_name", country_name );
  CONV_FROM_FIELD( "unknown06", unknown06 );
  CONV_FROM_FIELD( "control", control );
  CONV_FROM_FIELD( "founded_colonies", founded_colonies );
  CONV_FROM_FIELD( "diplomacy", diplomacy );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Tiles const& o,
                         cdr::tag_t<Tiles> ) {
  cdr::table tbl;
  conv.to_field( tbl, "tile_N", o.tile_n );
  conv.to_field( tbl, "tile_E", o.tile_e );
  conv.to_field( tbl, "tile_S", o.tile_s );
  conv.to_field( tbl, "tile_W", o.tile_w );
  conv.to_field( tbl, "tile_NW", o.tile_nw );
  conv.to_field( tbl, "tile_NE", o.tile_ne );
  conv.to_field( tbl, "tile_SE", o.tile_se );
  conv.to_field( tbl, "tile_SW", o.tile_sw );
  tbl["__key_order"] = cdr::list{
    "tile_N",
    "tile_E",
    "tile_S",
    "tile_W",
    "tile_NW",
    "tile_NE",
    "tile_SE",
    "tile_SW",
  };
  return tbl;
}

cdr::result<Tiles> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Tiles> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Tiles res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "tile_N", tile_n );
  CONV_FROM_FIELD( "tile_E", tile_e );
  CONV_FROM_FIELD( "tile_S", tile_s );
  CONV_FROM_FIELD( "tile_W", tile_w );
  CONV_FROM_FIELD( "tile_NW", tile_nw );
  CONV_FROM_FIELD( "tile_NE", tile_ne );
  CONV_FROM_FIELD( "tile_SE", tile_se );
  CONV_FROM_FIELD( "tile_SW", tile_sw );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Stock const& o,
                         cdr::tag_t<Stock> ) {
  cdr::table tbl;
  conv.to_field( tbl, "food", o.food );
  conv.to_field( tbl, "sugar", o.sugar );
  conv.to_field( tbl, "tobacco", o.tobacco );
  conv.to_field( tbl, "cotton", o.cotton );
  conv.to_field( tbl, "furs", o.furs );
  conv.to_field( tbl, "lumber", o.lumber );
  conv.to_field( tbl, "ore", o.ore );
  conv.to_field( tbl, "silver", o.silver );
  conv.to_field( tbl, "horses", o.horses );
  conv.to_field( tbl, "rum", o.rum );
  conv.to_field( tbl, "cigars", o.cigars );
  conv.to_field( tbl, "cloth", o.cloth );
  conv.to_field( tbl, "coats", o.coats );
  conv.to_field( tbl, "trade_goods", o.trade_goods );
  conv.to_field( tbl, "tools", o.tools );
  conv.to_field( tbl, "muskets", o.muskets );
  tbl["__key_order"] = cdr::list{
    "food",
    "sugar",
    "tobacco",
    "cotton",
    "furs",
    "lumber",
    "ore",
    "silver",
    "horses",
    "rum",
    "cigars",
    "cloth",
    "coats",
    "trade_goods",
    "tools",
    "muskets",
  };
  return tbl;
}

cdr::result<Stock> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stock> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Stock res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "food", food );
  CONV_FROM_FIELD( "sugar", sugar );
  CONV_FROM_FIELD( "tobacco", tobacco );
  CONV_FROM_FIELD( "cotton", cotton );
  CONV_FROM_FIELD( "furs", furs );
  CONV_FROM_FIELD( "lumber", lumber );
  CONV_FROM_FIELD( "ore", ore );
  CONV_FROM_FIELD( "silver", silver );
  CONV_FROM_FIELD( "horses", horses );
  CONV_FROM_FIELD( "rum", rum );
  CONV_FROM_FIELD( "cigars", cigars );
  CONV_FROM_FIELD( "cloth", cloth );
  CONV_FROM_FIELD( "coats", coats );
  CONV_FROM_FIELD( "trade_goods", trade_goods );
  CONV_FROM_FIELD( "tools", tools );
  CONV_FROM_FIELD( "muskets", muskets );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         PopulationOnMap const& o,
                         cdr::tag_t<PopulationOnMap> ) {
  cdr::table tbl;
  conv.to_field( tbl, "for english", o.for_english );
  conv.to_field( tbl, "for french", o.for_french );
  conv.to_field( tbl, "for spanish", o.for_spanish );
  conv.to_field( tbl, "for dutch", o.for_dutch );
  tbl["__key_order"] = cdr::list{
    "for english",
    "for french",
    "for spanish",
    "for dutch",
  };
  return tbl;
}

cdr::result<PopulationOnMap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PopulationOnMap> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  PopulationOnMap res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "for english", for_english );
  CONV_FROM_FIELD( "for french", for_french );
  CONV_FROM_FIELD( "for spanish", for_spanish );
  CONV_FROM_FIELD( "for dutch", for_dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         FortificationOnMap const& o,
                         cdr::tag_t<FortificationOnMap> ) {
  cdr::table tbl;
  conv.to_field( tbl, "for english", o.for_english );
  conv.to_field( tbl, "for french", o.for_french );
  conv.to_field( tbl, "for spanish", o.for_spanish );
  conv.to_field( tbl, "for dutch", o.for_dutch );
  tbl["__key_order"] = cdr::list{
    "for english",
    "for french",
    "for spanish",
    "for dutch",
  };
  return tbl;
}

cdr::result<FortificationOnMap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<FortificationOnMap> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  FortificationOnMap res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "for english", for_english );
  CONV_FROM_FIELD( "for french", for_french );
  CONV_FROM_FIELD( "for spanish", for_spanish );
  CONV_FROM_FIELD( "for dutch", for_dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         COLONY const& o,
                         cdr::tag_t<COLONY> ) {
  cdr::table tbl;
  conv.to_field( tbl, "x, y", o.x_y );
  conv.to_field( tbl, "name", o.name );
  conv.to_field( tbl, "nation_id", o.nation_id );
  conv.to_field( tbl, "unknown08", o.unknown08 );
  conv.to_field( tbl, "population", o.population );
  conv.to_field( tbl, "occupation", o.occupation );
  conv.to_field( tbl, "profession", o.profession );
  conv.to_field( tbl, "duration", o.duration );
  conv.to_field( tbl, "tiles", o.tiles );
  conv.to_field( tbl, "unknown10", o.unknown10 );
  conv.to_field( tbl, "buildings", o.buildings );
  conv.to_field( tbl, "custom_house_flags", o.custom_house_flags );
  conv.to_field( tbl, "unknown11", o.unknown11 );
  conv.to_field( tbl, "hammers", o.hammers );
  conv.to_field( tbl, "building_in_production", o.building_in_production );
  conv.to_field( tbl, "warehouse_level", o.warehouse_level );
  conv.to_field( tbl, "unknown12", o.unknown12 );
  conv.to_field( tbl, "stock", o.stock );
  conv.to_field( tbl, "population_on_map", o.population_on_map );
  conv.to_field( tbl, "fortification_on_map", o.fortification_on_map );
  conv.to_field( tbl, "rebel_dividend", o.rebel_dividend );
  conv.to_field( tbl, "rebel_divisor", o.rebel_divisor );
  tbl["__key_order"] = cdr::list{
    "x, y",
    "name",
    "nation_id",
    "unknown08",
    "population",
    "occupation",
    "profession",
    "duration",
    "tiles",
    "unknown10",
    "buildings",
    "custom_house_flags",
    "unknown11",
    "hammers",
    "building_in_production",
    "warehouse_level",
    "unknown12",
    "stock",
    "population_on_map",
    "fortification_on_map",
    "rebel_dividend",
    "rebel_divisor",
  };
  return tbl;
}

cdr::result<COLONY> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<COLONY> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  COLONY res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "x, y", x_y );
  CONV_FROM_FIELD( "name", name );
  CONV_FROM_FIELD( "nation_id", nation_id );
  CONV_FROM_FIELD( "unknown08", unknown08 );
  CONV_FROM_FIELD( "population", population );
  CONV_FROM_FIELD( "occupation", occupation );
  CONV_FROM_FIELD( "profession", profession );
  CONV_FROM_FIELD( "duration", duration );
  CONV_FROM_FIELD( "tiles", tiles );
  CONV_FROM_FIELD( "unknown10", unknown10 );
  CONV_FROM_FIELD( "buildings", buildings );
  CONV_FROM_FIELD( "custom_house_flags", custom_house_flags );
  CONV_FROM_FIELD( "unknown11", unknown11 );
  CONV_FROM_FIELD( "hammers", hammers );
  CONV_FROM_FIELD( "building_in_production", building_in_production );
  CONV_FROM_FIELD( "warehouse_level", warehouse_level );
  CONV_FROM_FIELD( "unknown12", unknown12 );
  CONV_FROM_FIELD( "stock", stock );
  CONV_FROM_FIELD( "population_on_map", population_on_map );
  CONV_FROM_FIELD( "fortification_on_map", fortification_on_map );
  CONV_FROM_FIELD( "rebel_dividend", rebel_dividend );
  CONV_FROM_FIELD( "rebel_divisor", rebel_divisor );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         TransportChain const& o,
                         cdr::tag_t<TransportChain> ) {
  cdr::table tbl;
  conv.to_field( tbl, "next_unit_idx", o.next_unit_idx );
  conv.to_field( tbl, "prev_unit_idx", o.prev_unit_idx );
  tbl["__key_order"] = cdr::list{
    "next_unit_idx",
    "prev_unit_idx",
  };
  return tbl;
}

cdr::result<TransportChain> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TransportChain> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TransportChain res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "next_unit_idx", next_unit_idx );
  CONV_FROM_FIELD( "prev_unit_idx", prev_unit_idx );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         UNIT const& o,
                         cdr::tag_t<UNIT> ) {
  cdr::table tbl;
  conv.to_field( tbl, "x, y", o.x_y );
  conv.to_field( tbl, "type", o.type );
  conv.to_field( tbl, "nation_info", o.nation_info );
  conv.to_field( tbl, "unknown15", o.unknown15 );
  conv.to_field( tbl, "moves", o.moves );
  conv.to_field( tbl, "origin_settlement", o.origin_settlement );
  conv.to_field( tbl, "unknown16b", o.unknown16b );
  conv.to_field( tbl, "orders", o.orders );
  conv.to_field( tbl, "goto_x", o.goto_x );
  conv.to_field( tbl, "goto_y", o.goto_y );
  conv.to_field( tbl, "unknown18", o.unknown18 );
  conv.to_field( tbl, "holds_occupied", o.holds_occupied );
  conv.to_field( tbl, "cargo_items", o.cargo_items );
  conv.to_field( tbl, "cargo_hold", o.cargo_hold );
  conv.to_field( tbl, "turns_worked", o.turns_worked );
  conv.to_field( tbl, "profession_or_treasure_amount", o.profession_or_treasure_amount );
  conv.to_field( tbl, "transport_chain", o.transport_chain );
  tbl["__key_order"] = cdr::list{
    "x, y",
    "type",
    "nation_info",
    "unknown15",
    "moves",
    "origin_settlement",
    "unknown16b",
    "orders",
    "goto_x",
    "goto_y",
    "unknown18",
    "holds_occupied",
    "cargo_items",
    "cargo_hold",
    "turns_worked",
    "profession_or_treasure_amount",
    "transport_chain",
  };
  return tbl;
}

cdr::result<UNIT> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UNIT> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UNIT res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "x, y", x_y );
  CONV_FROM_FIELD( "type", type );
  CONV_FROM_FIELD( "nation_info", nation_info );
  CONV_FROM_FIELD( "unknown15", unknown15 );
  CONV_FROM_FIELD( "moves", moves );
  CONV_FROM_FIELD( "origin_settlement", origin_settlement );
  CONV_FROM_FIELD( "unknown16b", unknown16b );
  CONV_FROM_FIELD( "orders", orders );
  CONV_FROM_FIELD( "goto_x", goto_x );
  CONV_FROM_FIELD( "goto_y", goto_y );
  CONV_FROM_FIELD( "unknown18", unknown18 );
  CONV_FROM_FIELD( "holds_occupied", holds_occupied );
  CONV_FROM_FIELD( "cargo_items", cargo_items );
  CONV_FROM_FIELD( "cargo_hold", cargo_hold );
  CONV_FROM_FIELD( "turns_worked", turns_worked );
  CONV_FROM_FIELD( "profession_or_treasure_amount", profession_or_treasure_amount );
  CONV_FROM_FIELD( "transport_chain", transport_chain );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         RelationByIndian const& o,
                         cdr::tag_t<RelationByIndian> ) {
  cdr::table tbl;
  conv.to_field( tbl, "Inca", o.inca );
  conv.to_field( tbl, "Aztec", o.aztec );
  conv.to_field( tbl, "Awarak", o.awarak );
  conv.to_field( tbl, "Iroquois", o.iroquois );
  conv.to_field( tbl, "Cherokee", o.cherokee );
  conv.to_field( tbl, "Apache", o.apache );
  conv.to_field( tbl, "Sioux", o.sioux );
  conv.to_field( tbl, "Tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "Inca",
    "Aztec",
    "Awarak",
    "Iroquois",
    "Cherokee",
    "Apache",
    "Sioux",
    "Tupi",
  };
  return tbl;
}

cdr::result<RelationByIndian> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByIndian> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  RelationByIndian res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "Inca", inca );
  CONV_FROM_FIELD( "Aztec", aztec );
  CONV_FROM_FIELD( "Awarak", awarak );
  CONV_FROM_FIELD( "Iroquois", iroquois );
  CONV_FROM_FIELD( "Cherokee", cherokee );
  CONV_FROM_FIELD( "Apache", apache );
  CONV_FROM_FIELD( "Sioux", sioux );
  CONV_FROM_FIELD( "Tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Trade const& o,
                         cdr::tag_t<Trade> ) {
  cdr::table tbl;
  conv.to_field( tbl, "eu_prc", o.eu_prc );
  conv.to_field( tbl, "nr", o.nr );
  conv.to_field( tbl, "gold", o.gold );
  conv.to_field( tbl, "tons", o.tons );
  conv.to_field( tbl, "tons2", o.tons2 );
  tbl["__key_order"] = cdr::list{
    "eu_prc",
    "nr",
    "gold",
    "tons",
    "tons2",
  };
  return tbl;
}

cdr::result<Trade> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Trade> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Trade res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "eu_prc", eu_prc );
  CONV_FROM_FIELD( "nr", nr );
  CONV_FROM_FIELD( "gold", gold );
  CONV_FROM_FIELD( "tons", tons );
  CONV_FROM_FIELD( "tons2", tons2 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         NATION const& o,
                         cdr::tag_t<NATION> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown19", o.unknown19 );
  conv.to_field( tbl, "tax_rate", o.tax_rate );
  conv.to_field( tbl, "recruit", o.recruit );
  conv.to_field( tbl, "unused07", o.unused07 );
  conv.to_field( tbl, "recruit_count", o.recruit_count );
  conv.to_field( tbl, "founding_fathers", o.founding_fathers );
  conv.to_field( tbl, "unknown21", o.unknown21 );
  conv.to_field( tbl, "liberty_bells_total", o.liberty_bells_total );
  conv.to_field( tbl, "liberty_bells_last_turn", o.liberty_bells_last_turn );
  conv.to_field( tbl, "unknown22", o.unknown22 );
  conv.to_field( tbl, "next_founding_father", o.next_founding_father );
  conv.to_field( tbl, "founding_father_count", o.founding_father_count );
  conv.to_field( tbl, "prob_founding_father_count_end", o.prob_founding_father_count_end );
  conv.to_field( tbl, "villages_burned", o.villages_burned );
  conv.to_field( tbl, "unknown23", o.unknown23 );
  conv.to_field( tbl, "artillery_bought_count", o.artillery_bought_count );
  conv.to_field( tbl, "boycott_bitmap", o.boycott_bitmap );
  conv.to_field( tbl, "royal_money", o.royal_money );
  conv.to_field( tbl, "unknown24b", o.unknown24b );
  conv.to_field( tbl, "gold", o.gold );
  conv.to_field( tbl, "current_crosses", o.current_crosses );
  conv.to_field( tbl, "needed_crosses", o.needed_crosses );
  conv.to_field( tbl, "point_return_from_europe", o.point_return_from_europe );
  conv.to_field( tbl, "unknown25b", o.unknown25b );
  conv.to_field( tbl, "relation_by_indian", o.relation_by_indian );
  conv.to_field( tbl, "unknown26a", o.unknown26a );
  conv.to_field( tbl, "unknown26b", o.unknown26b );
  conv.to_field( tbl, "unknown26c", o.unknown26c );
  conv.to_field( tbl, "trade", o.trade );
  tbl["__key_order"] = cdr::list{
    "unknown19",
    "tax_rate",
    "recruit",
    "unused07",
    "recruit_count",
    "founding_fathers",
    "unknown21",
    "liberty_bells_total",
    "liberty_bells_last_turn",
    "unknown22",
    "next_founding_father",
    "founding_father_count",
    "prob_founding_father_count_end",
    "villages_burned",
    "unknown23",
    "artillery_bought_count",
    "boycott_bitmap",
    "royal_money",
    "unknown24b",
    "gold",
    "current_crosses",
    "needed_crosses",
    "point_return_from_europe",
    "unknown25b",
    "relation_by_indian",
    "unknown26a",
    "unknown26b",
    "unknown26c",
    "trade",
  };
  return tbl;
}

cdr::result<NATION> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NATION> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  NATION res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unknown19", unknown19 );
  CONV_FROM_FIELD( "tax_rate", tax_rate );
  CONV_FROM_FIELD( "recruit", recruit );
  CONV_FROM_FIELD( "unused07", unused07 );
  CONV_FROM_FIELD( "recruit_count", recruit_count );
  CONV_FROM_FIELD( "founding_fathers", founding_fathers );
  CONV_FROM_FIELD( "unknown21", unknown21 );
  CONV_FROM_FIELD( "liberty_bells_total", liberty_bells_total );
  CONV_FROM_FIELD( "liberty_bells_last_turn", liberty_bells_last_turn );
  CONV_FROM_FIELD( "unknown22", unknown22 );
  CONV_FROM_FIELD( "next_founding_father", next_founding_father );
  CONV_FROM_FIELD( "founding_father_count", founding_father_count );
  CONV_FROM_FIELD( "prob_founding_father_count_end", prob_founding_father_count_end );
  CONV_FROM_FIELD( "villages_burned", villages_burned );
  CONV_FROM_FIELD( "unknown23", unknown23 );
  CONV_FROM_FIELD( "artillery_bought_count", artillery_bought_count );
  CONV_FROM_FIELD( "boycott_bitmap", boycott_bitmap );
  CONV_FROM_FIELD( "royal_money", royal_money );
  CONV_FROM_FIELD( "unknown24b", unknown24b );
  CONV_FROM_FIELD( "gold", gold );
  CONV_FROM_FIELD( "current_crosses", current_crosses );
  CONV_FROM_FIELD( "needed_crosses", needed_crosses );
  CONV_FROM_FIELD( "point_return_from_europe", point_return_from_europe );
  CONV_FROM_FIELD( "unknown25b", unknown25b );
  CONV_FROM_FIELD( "relation_by_indian", relation_by_indian );
  CONV_FROM_FIELD( "unknown26a", unknown26a );
  CONV_FROM_FIELD( "unknown26b", unknown26b );
  CONV_FROM_FIELD( "unknown26c", unknown26c );
  CONV_FROM_FIELD( "trade", trade );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         Alarm const& o,
                         cdr::tag_t<Alarm> ) {
  cdr::table tbl;
  conv.to_field( tbl, "friction", o.friction );
  conv.to_field( tbl, "attacks", o.attacks );
  tbl["__key_order"] = cdr::list{
    "friction",
    "attacks",
  };
  return tbl;
}

cdr::result<Alarm> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Alarm> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Alarm res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "friction", friction );
  CONV_FROM_FIELD( "attacks", attacks );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         TRIBE const& o,
                         cdr::tag_t<TRIBE> ) {
  cdr::table tbl;
  conv.to_field( tbl, "x, y", o.x_y );
  conv.to_field( tbl, "nation_id", o.nation_id );
  conv.to_field( tbl, "ALCS", o.alcs );
  conv.to_field( tbl, "population", o.population );
  conv.to_field( tbl, "mission", o.mission );
  conv.to_field( tbl, "unknown28", o.unknown28 );
  conv.to_field( tbl, "last_bought", o.last_bought );
  conv.to_field( tbl, "last_sold", o.last_sold );
  conv.to_field( tbl, "alarm", o.alarm );
  tbl["__key_order"] = cdr::list{
    "x, y",
    "nation_id",
    "ALCS",
    "population",
    "mission",
    "unknown28",
    "last_bought",
    "last_sold",
    "alarm",
  };
  return tbl;
}

cdr::result<TRIBE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TRIBE> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TRIBE res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "x, y", x_y );
  CONV_FROM_FIELD( "nation_id", nation_id );
  CONV_FROM_FIELD( "ALCS", alcs );
  CONV_FROM_FIELD( "population", population );
  CONV_FROM_FIELD( "mission", mission );
  CONV_FROM_FIELD( "unknown28", unknown28 );
  CONV_FROM_FIELD( "last_bought", last_bought );
  CONV_FROM_FIELD( "last_sold", last_sold );
  CONV_FROM_FIELD( "alarm", alarm );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         RelationByNations const& o,
                         cdr::tag_t<RelationByNations> ) {
  cdr::table tbl;
  conv.to_field( tbl, "England", o.england );
  conv.to_field( tbl, "France", o.france );
  conv.to_field( tbl, "Spain", o.spain );
  conv.to_field( tbl, "Netherlands", o.netherlands );
  tbl["__key_order"] = cdr::list{
    "England",
    "France",
    "Spain",
    "Netherlands",
  };
  return tbl;
}

cdr::result<RelationByNations> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByNations> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  RelationByNations res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "England", england );
  CONV_FROM_FIELD( "France", france );
  CONV_FROM_FIELD( "Spain", spain );
  CONV_FROM_FIELD( "Netherlands", netherlands );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         INDIAN const& o,
                         cdr::tag_t<INDIAN> ) {
  cdr::table tbl;
  conv.to_field( tbl, "capitol (x, y)", o.capitol_x_y );
  conv.to_field( tbl, "tech", o.tech );
  conv.to_field( tbl, "unknown31a", o.unknown31a );
  conv.to_field( tbl, "muskets", o.muskets );
  conv.to_field( tbl, "horse_herds", o.horse_herds );
  conv.to_field( tbl, "unknown31b", o.unknown31b );
  conv.to_field( tbl, "tons", o.tons );
  conv.to_field( tbl, "unknown32", o.unknown32 );
  conv.to_field( tbl, "relation_by_nations", o.relation_by_nations );
  conv.to_field( tbl, "unknown33", o.unknown33 );
  conv.to_field( tbl, "alarm_by_player", o.alarm_by_player );
  tbl["__key_order"] = cdr::list{
    "capitol (x, y)",
    "tech",
    "unknown31a",
    "muskets",
    "horse_herds",
    "unknown31b",
    "tons",
    "unknown32",
    "relation_by_nations",
    "unknown33",
    "alarm_by_player",
  };
  return tbl;
}

cdr::result<INDIAN> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<INDIAN> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  INDIAN res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "capitol (x, y)", capitol_x_y );
  CONV_FROM_FIELD( "tech", tech );
  CONV_FROM_FIELD( "unknown31a", unknown31a );
  CONV_FROM_FIELD( "muskets", muskets );
  CONV_FROM_FIELD( "horse_herds", horse_herds );
  CONV_FROM_FIELD( "unknown31b", unknown31b );
  CONV_FROM_FIELD( "tons", tons );
  CONV_FROM_FIELD( "unknown32", unknown32 );
  CONV_FROM_FIELD( "relation_by_nations", relation_by_nations );
  CONV_FROM_FIELD( "unknown33", unknown33 );
  CONV_FROM_FIELD( "alarm_by_player", alarm_by_player );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         STUFF const& o,
                         cdr::tag_t<STUFF> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown34", o.unknown34 );
  conv.to_field( tbl, "counter_decreasing_on_new_colony", o.counter_decreasing_on_new_colony );
  conv.to_field( tbl, "unknown35", o.unknown35 );
  conv.to_field( tbl, "counter_increasing_on_new_colony", o.counter_increasing_on_new_colony );
  conv.to_field( tbl, "unknown36", o.unknown36 );
  conv.to_field( tbl, "x", o.x );
  conv.to_field( tbl, "y", o.y );
  conv.to_field( tbl, "zoom_level", o.zoom_level );
  conv.to_field( tbl, "unknown37", o.unknown37 );
  conv.to_field( tbl, "viewport_x", o.viewport_x );
  conv.to_field( tbl, "viewport_y", o.viewport_y );
  tbl["__key_order"] = cdr::list{
    "unknown34",
    "counter_decreasing_on_new_colony",
    "unknown35",
    "counter_increasing_on_new_colony",
    "unknown36",
    "x",
    "y",
    "zoom_level",
    "unknown37",
    "viewport_x",
    "viewport_y",
  };
  return tbl;
}

cdr::result<STUFF> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<STUFF> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  STUFF res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unknown34", unknown34 );
  CONV_FROM_FIELD( "counter_decreasing_on_new_colony", counter_decreasing_on_new_colony );
  CONV_FROM_FIELD( "unknown35", unknown35 );
  CONV_FROM_FIELD( "counter_increasing_on_new_colony", counter_increasing_on_new_colony );
  CONV_FROM_FIELD( "unknown36", unknown36 );
  CONV_FROM_FIELD( "x", x );
  CONV_FROM_FIELD( "y", y );
  CONV_FROM_FIELD( "zoom_level", zoom_level );
  CONV_FROM_FIELD( "unknown37", unknown37 );
  CONV_FROM_FIELD( "viewport_x", viewport_x );
  CONV_FROM_FIELD( "viewport_y", viewport_y );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
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

cdr::value to_canonical( cdr::converter& conv,
                         TRADEROUTE const& o,
                         cdr::tag_t<TRADEROUTE> ) {
  cdr::table tbl;
  conv.to_field( tbl, "name", o.name );
  conv.to_field( tbl, "land_or_sea", o.land_or_sea );
  conv.to_field( tbl, "stops_count", o.stops_count );
  conv.to_field( tbl, "stop_1_colony_index", o.stop_1_colony_index );
  conv.to_field( tbl, "stop_1_loads_and_unloads_count", o.stop_1_loads_and_unloads_count );
  conv.to_field( tbl, "stop_1_loads_cargo", o.stop_1_loads_cargo );
  conv.to_field( tbl, "stop_1_unloads_cargo", o.stop_1_unloads_cargo );
  conv.to_field( tbl, "unknown47", o.unknown47 );
  conv.to_field( tbl, "stop_2_colony_index", o.stop_2_colony_index );
  conv.to_field( tbl, "stop_2_loads_and_unloads_count", o.stop_2_loads_and_unloads_count );
  conv.to_field( tbl, "stop_2_loads_cargo", o.stop_2_loads_cargo );
  conv.to_field( tbl, "stop_2_unloads_cargo", o.stop_2_unloads_cargo );
  conv.to_field( tbl, "unknown48", o.unknown48 );
  conv.to_field( tbl, "stop_3_colony_index", o.stop_3_colony_index );
  conv.to_field( tbl, "stop_3_loads_and_unloads_count", o.stop_3_loads_and_unloads_count );
  conv.to_field( tbl, "stop_3_loads_cargo", o.stop_3_loads_cargo );
  conv.to_field( tbl, "stop_3_unloads_cargo", o.stop_3_unloads_cargo );
  conv.to_field( tbl, "unknown49", o.unknown49 );
  conv.to_field( tbl, "stop_4_colony_index", o.stop_4_colony_index );
  conv.to_field( tbl, "stop_4_loads_and_unloads_count", o.stop_4_loads_and_unloads_count );
  conv.to_field( tbl, "stop_4_loads_cargo", o.stop_4_loads_cargo );
  conv.to_field( tbl, "stop_4_unloads_cargo", o.stop_4_unloads_cargo );
  conv.to_field( tbl, "unknown50", o.unknown50 );
  tbl["__key_order"] = cdr::list{
    "name",
    "land_or_sea",
    "stops_count",
    "stop_1_colony_index",
    "stop_1_loads_and_unloads_count",
    "stop_1_loads_cargo",
    "stop_1_unloads_cargo",
    "unknown47",
    "stop_2_colony_index",
    "stop_2_loads_and_unloads_count",
    "stop_2_loads_cargo",
    "stop_2_unloads_cargo",
    "unknown48",
    "stop_3_colony_index",
    "stop_3_loads_and_unloads_count",
    "stop_3_loads_cargo",
    "stop_3_unloads_cargo",
    "unknown49",
    "stop_4_colony_index",
    "stop_4_loads_and_unloads_count",
    "stop_4_loads_cargo",
    "stop_4_unloads_cargo",
    "unknown50",
  };
  return tbl;
}

cdr::result<TRADEROUTE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TRADEROUTE> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TRADEROUTE res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "name", name );
  CONV_FROM_FIELD( "land_or_sea", land_or_sea );
  CONV_FROM_FIELD( "stops_count", stops_count );
  CONV_FROM_FIELD( "stop_1_colony_index", stop_1_colony_index );
  CONV_FROM_FIELD( "stop_1_loads_and_unloads_count", stop_1_loads_and_unloads_count );
  CONV_FROM_FIELD( "stop_1_loads_cargo", stop_1_loads_cargo );
  CONV_FROM_FIELD( "stop_1_unloads_cargo", stop_1_unloads_cargo );
  CONV_FROM_FIELD( "unknown47", unknown47 );
  CONV_FROM_FIELD( "stop_2_colony_index", stop_2_colony_index );
  CONV_FROM_FIELD( "stop_2_loads_and_unloads_count", stop_2_loads_and_unloads_count );
  CONV_FROM_FIELD( "stop_2_loads_cargo", stop_2_loads_cargo );
  CONV_FROM_FIELD( "stop_2_unloads_cargo", stop_2_unloads_cargo );
  CONV_FROM_FIELD( "unknown48", unknown48 );
  CONV_FROM_FIELD( "stop_3_colony_index", stop_3_colony_index );
  CONV_FROM_FIELD( "stop_3_loads_and_unloads_count", stop_3_loads_and_unloads_count );
  CONV_FROM_FIELD( "stop_3_loads_cargo", stop_3_loads_cargo );
  CONV_FROM_FIELD( "stop_3_unloads_cargo", stop_3_unloads_cargo );
  CONV_FROM_FIELD( "unknown49", unknown49 );
  CONV_FROM_FIELD( "stop_4_colony_index", stop_4_colony_index );
  CONV_FROM_FIELD( "stop_4_loads_and_unloads_count", stop_4_loads_and_unloads_count );
  CONV_FROM_FIELD( "stop_4_loads_cargo", stop_4_loads_cargo );
  CONV_FROM_FIELD( "stop_4_unloads_cargo", stop_4_unloads_cargo );
  CONV_FROM_FIELD( "unknown50", unknown50 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** ColonySAV
*****************************************************************/
// NOTE: binary conversion manually implemented.

cdr::value to_canonical( cdr::converter& conv,
                         ColonySAV const& o,
                         cdr::tag_t<ColonySAV> ) {
  cdr::table tbl;
  conv.to_field( tbl, "HEAD", o.head );
  conv.to_field( tbl, "PLAYER", o.player );
  conv.to_field( tbl, "OTHER", o.other );
  conv.to_field( tbl, "COLONY", o.colony );
  conv.to_field( tbl, "UNIT", o.unit );
  conv.to_field( tbl, "NATION", o.nation );
  conv.to_field( tbl, "TRIBE", o.tribe );
  conv.to_field( tbl, "INDIAN", o.indian );
  conv.to_field( tbl, "STUFF", o.stuff );
  conv.to_field( tbl, "TILE", o.tile );
  conv.to_field( tbl, "MASK", o.mask );
  conv.to_field( tbl, "PATH", o.path );
  conv.to_field( tbl, "SEEN", o.seen );
  conv.to_field( tbl, "unknown_map38a", o.unknown_map38a );
  conv.to_field( tbl, "unknown_map38b", o.unknown_map38b );
  conv.to_field( tbl, "unknown39a", o.unknown39a );
  conv.to_field( tbl, "unknown39b", o.unknown39b );
  conv.to_field( tbl, "prime_resource_seed", o.prime_resource_seed );
  conv.to_field( tbl, "unknown39d", o.unknown39d );
  conv.to_field( tbl, "TRADE_ROUTE", o.trade_route );
  tbl["__key_order"] = cdr::list{
    "HEAD",
    "PLAYER",
    "OTHER",
    "COLONY",
    "UNIT",
    "NATION",
    "TRIBE",
    "INDIAN",
    "STUFF",
    "TILE",
    "MASK",
    "PATH",
    "SEEN",
    "unknown_map38a",
    "unknown_map38b",
    "unknown39a",
    "unknown39b",
    "prime_resource_seed",
    "unknown39d",
    "TRADE_ROUTE",
  };
  return tbl;
}

cdr::result<ColonySAV> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonySAV> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ColonySAV res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "HEAD", head );
  CONV_FROM_FIELD( "PLAYER", player );
  CONV_FROM_FIELD( "OTHER", other );
  CONV_FROM_FIELD( "COLONY", colony );
  CONV_FROM_FIELD( "UNIT", unit );
  CONV_FROM_FIELD( "NATION", nation );
  CONV_FROM_FIELD( "TRIBE", tribe );
  CONV_FROM_FIELD( "INDIAN", indian );
  CONV_FROM_FIELD( "STUFF", stuff );
  CONV_FROM_FIELD( "TILE", tile );
  CONV_FROM_FIELD( "MASK", mask );
  CONV_FROM_FIELD( "PATH", path );
  CONV_FROM_FIELD( "SEEN", seen );
  CONV_FROM_FIELD( "unknown_map38a", unknown_map38a );
  CONV_FROM_FIELD( "unknown_map38b", unknown_map38b );
  CONV_FROM_FIELD( "unknown39a", unknown39a );
  CONV_FROM_FIELD( "unknown39b", unknown39b );
  CONV_FROM_FIELD( "prime_resource_seed", prime_resource_seed );
  CONV_FROM_FIELD( "unknown39d", unknown39d );
  CONV_FROM_FIELD( "TRADE_ROUTE", trade_route );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

}  // namespace sav
