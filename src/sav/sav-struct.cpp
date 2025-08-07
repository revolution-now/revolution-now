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
#include "base/to-str-ext-std.hpp"

// C++ standard libary
#include <map>

/****************************************************************
** Macros.
*****************************************************************/
#define BAD_ENUM_STR_VALUE( typename, str_value )             \
  conv.err( "unrecognized value for enum " typename ": '{}'", \
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
void to_str( cargo_4bit_type const& o, std::string& out, base::tag<cargo_4bit_type> ) {
  switch( o ) {
    case cargo_4bit_type::food: out += "food"; return;
    case cargo_4bit_type::sugar: out += "sugar"; return;
    case cargo_4bit_type::tobacco: out += "tobacco"; return;
    case cargo_4bit_type::cotton: out += "cotton"; return;
    case cargo_4bit_type::furs: out += "furs"; return;
    case cargo_4bit_type::lumber: out += "lumber"; return;
    case cargo_4bit_type::ore: out += "ore"; return;
    case cargo_4bit_type::silver: out += "silver"; return;
    case cargo_4bit_type::horses: out += "horses"; return;
    case cargo_4bit_type::rum: out += "rum"; return;
    case cargo_4bit_type::cigars: out += "cigars"; return;
    case cargo_4bit_type::cloth: out += "cloth"; return;
    case cargo_4bit_type::coats: out += "coats"; return;
    case cargo_4bit_type::goods: out += "goods"; return;
    case cargo_4bit_type::tools: out += "tools"; return;
    case cargo_4bit_type::muskets: out += "muskets"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
void to_str( control_type const& o, std::string& out, base::tag<control_type> ) {
  switch( o ) {
    case control_type::player: out += "PLAYER"; return;
    case control_type::ai: out += "AI"; return;
    case control_type::withdrawn: out += "WITHDRAWN"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         control_type const& o,
                         cdr::tag_t<control_type> ) {
  switch( o ) {
    case control_type::player: return "PLAYER";
    case control_type::ai: return "AI";
    case control_type::withdrawn: return "WITHDRAWN";
  }
  return cdr::null;
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
void to_str( difficulty_type const& o, std::string& out, base::tag<difficulty_type> ) {
  switch( o ) {
    case difficulty_type::discoverer: out += "Discoverer"; return;
    case difficulty_type::explorer: out += "Explorer"; return;
    case difficulty_type::conquistador: out += "Conquistador"; return;
    case difficulty_type::governor: out += "Governor"; return;
    case difficulty_type::viceroy: out += "Viceroy"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
** end_of_turn_sign_type
*****************************************************************/
void to_str( end_of_turn_sign_type const& o, std::string& out, base::tag<end_of_turn_sign_type> ) {
  switch( o ) {
    case end_of_turn_sign_type::not_shown: out += "Not shown"; return;
    case end_of_turn_sign_type::flashing: out += "Flashing"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         end_of_turn_sign_type const& o,
                         cdr::tag_t<end_of_turn_sign_type> ) {
  switch( o ) {
    case end_of_turn_sign_type::not_shown: return "Not shown";
    case end_of_turn_sign_type::flashing: return "Flashing";
  }
  return cdr::null;
}

cdr::result<end_of_turn_sign_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<end_of_turn_sign_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, end_of_turn_sign_type> const m{
    { "Not shown", end_of_turn_sign_type::not_shown },
    { "Flashing", end_of_turn_sign_type::flashing },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "end_of_turn_sign_type", str );
}

/****************************************************************
** fortification_level_type
*****************************************************************/
void to_str( fortification_level_type const& o, std::string& out, base::tag<fortification_level_type> ) {
  switch( o ) {
    case fortification_level_type::none: out += "none"; return;
    case fortification_level_type::stockade: out += "stockade"; return;
    case fortification_level_type::fort: out += "fort"; return;
    case fortification_level_type::fortress: out += "fortress"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         fortification_level_type const& o,
                         cdr::tag_t<fortification_level_type> ) {
  switch( o ) {
    case fortification_level_type::none: return "none";
    case fortification_level_type::stockade: return "stockade";
    case fortification_level_type::fort: return "fort";
    case fortification_level_type::fortress: return "fortress";
  }
  return cdr::null;
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
void to_str( has_city_1bit_type const& o, std::string& out, base::tag<has_city_1bit_type> ) {
  switch( o ) {
    case has_city_1bit_type::empty: out += " "; return;
    case has_city_1bit_type::c: out += "C"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         has_city_1bit_type const& o,
                         cdr::tag_t<has_city_1bit_type> ) {
  switch( o ) {
    case has_city_1bit_type::empty: return " ";
    case has_city_1bit_type::c: return "C";
  }
  return cdr::null;
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
void to_str( has_unit_1bit_type const& o, std::string& out, base::tag<has_unit_1bit_type> ) {
  switch( o ) {
    case has_unit_1bit_type::empty: out += " "; return;
    case has_unit_1bit_type::u: out += "U"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         has_unit_1bit_type const& o,
                         cdr::tag_t<has_unit_1bit_type> ) {
  switch( o ) {
    case has_unit_1bit_type::empty: return " ";
    case has_unit_1bit_type::u: return "U";
  }
  return cdr::null;
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
void to_str( hills_river_3bit_type const& o, std::string& out, base::tag<hills_river_3bit_type> ) {
  switch( o ) {
    case hills_river_3bit_type::empty: out += "  "; return;
    case hills_river_3bit_type::c: out += "^ "; return;
    case hills_river_3bit_type::t: out += "~ "; return;
    case hills_river_3bit_type::tc: out += "~^"; return;
    case hills_river_3bit_type::qq: out += "??"; return;
    case hills_river_3bit_type::cc: out += "^^"; return;
    case hills_river_3bit_type::tt: out += "~~"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
void to_str( level_2bit_type const& o, std::string& out, base::tag<level_2bit_type> ) {
  switch( o ) {
    case level_2bit_type::_0: out += "0"; return;
    case level_2bit_type::_1: out += "1"; return;
    case level_2bit_type::_2: out += "2"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         level_2bit_type const& o,
                         cdr::tag_t<level_2bit_type> ) {
  switch( o ) {
    case level_2bit_type::_0: return "0";
    case level_2bit_type::_1: return "1";
    case level_2bit_type::_2: return "2";
  }
  return cdr::null;
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
void to_str( level_3bit_type const& o, std::string& out, base::tag<level_3bit_type> ) {
  switch( o ) {
    case level_3bit_type::_0: out += "0"; return;
    case level_3bit_type::_1: out += "1"; return;
    case level_3bit_type::_2: out += "2"; return;
    case level_3bit_type::_3: out += "3"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         level_3bit_type const& o,
                         cdr::tag_t<level_3bit_type> ) {
  switch( o ) {
    case level_3bit_type::_0: return "0";
    case level_3bit_type::_1: return "1";
    case level_3bit_type::_2: return "2";
    case level_3bit_type::_3: return "3";
  }
  return cdr::null;
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
** nation_2byte_type
*****************************************************************/
void to_str( nation_2byte_type const& o, std::string& out, base::tag<nation_2byte_type> ) {
  switch( o ) {
    case nation_2byte_type::england: out += "England"; return;
    case nation_2byte_type::france: out += "France"; return;
    case nation_2byte_type::spain: out += "Spain"; return;
    case nation_2byte_type::netherlands: out += "Netherlands"; return;
    case nation_2byte_type::inca: out += "Inca"; return;
    case nation_2byte_type::aztec: out += "Aztec"; return;
    case nation_2byte_type::arawak: out += "Arawak"; return;
    case nation_2byte_type::iroquois: out += "Iroquois"; return;
    case nation_2byte_type::cherokee: out += "Cherokee"; return;
    case nation_2byte_type::apache: out += "Apache"; return;
    case nation_2byte_type::sioux: out += "Sioux"; return;
    case nation_2byte_type::tupi: out += "Tupi"; return;
    case nation_2byte_type::none: out += "None"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         nation_2byte_type const& o,
                         cdr::tag_t<nation_2byte_type> ) {
  switch( o ) {
    case nation_2byte_type::england: return "England";
    case nation_2byte_type::france: return "France";
    case nation_2byte_type::spain: return "Spain";
    case nation_2byte_type::netherlands: return "Netherlands";
    case nation_2byte_type::inca: return "Inca";
    case nation_2byte_type::aztec: return "Aztec";
    case nation_2byte_type::arawak: return "Arawak";
    case nation_2byte_type::iroquois: return "Iroquois";
    case nation_2byte_type::cherokee: return "Cherokee";
    case nation_2byte_type::apache: return "Apache";
    case nation_2byte_type::sioux: return "Sioux";
    case nation_2byte_type::tupi: return "Tupi";
    case nation_2byte_type::none: return "None";
  }
  return cdr::null;
}

cdr::result<nation_2byte_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_2byte_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, nation_2byte_type> const m{
    { "England", nation_2byte_type::england },
    { "France", nation_2byte_type::france },
    { "Spain", nation_2byte_type::spain },
    { "Netherlands", nation_2byte_type::netherlands },
    { "Inca", nation_2byte_type::inca },
    { "Aztec", nation_2byte_type::aztec },
    { "Arawak", nation_2byte_type::arawak },
    { "Iroquois", nation_2byte_type::iroquois },
    { "Cherokee", nation_2byte_type::cherokee },
    { "Apache", nation_2byte_type::apache },
    { "Sioux", nation_2byte_type::sioux },
    { "Tupi", nation_2byte_type::tupi },
    { "None", nation_2byte_type::none },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "nation_2byte_type", str );
}

/****************************************************************
** nation_4bit_short_type
*****************************************************************/
void to_str( nation_4bit_short_type const& o, std::string& out, base::tag<nation_4bit_short_type> ) {
  switch( o ) {
    case nation_4bit_short_type::en: out += "EN"; return;
    case nation_4bit_short_type::fr: out += "FR"; return;
    case nation_4bit_short_type::sp: out += "SP"; return;
    case nation_4bit_short_type::nl: out += "NL"; return;
    case nation_4bit_short_type::in: out += "in"; return;
    case nation_4bit_short_type::az: out += "az"; return;
    case nation_4bit_short_type::aw: out += "aw"; return;
    case nation_4bit_short_type::ir: out += "ir"; return;
    case nation_4bit_short_type::ch: out += "ch"; return;
    case nation_4bit_short_type::ap: out += "ap"; return;
    case nation_4bit_short_type::si: out += "si"; return;
    case nation_4bit_short_type::tu: out += "tu"; return;
    case nation_4bit_short_type::empty: out += "  "; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
void to_str( nation_4bit_type const& o, std::string& out, base::tag<nation_4bit_type> ) {
  switch( o ) {
    case nation_4bit_type::england: out += "England"; return;
    case nation_4bit_type::france: out += "France"; return;
    case nation_4bit_type::spain: out += "Spain"; return;
    case nation_4bit_type::netherlands: out += "Netherlands"; return;
    case nation_4bit_type::inca: out += "Inca"; return;
    case nation_4bit_type::aztec: out += "Aztec"; return;
    case nation_4bit_type::arawak: out += "Arawak"; return;
    case nation_4bit_type::iroquois: out += "Iroquois"; return;
    case nation_4bit_type::cherokee: out += "Cherokee"; return;
    case nation_4bit_type::apache: out += "Apache"; return;
    case nation_4bit_type::sioux: out += "Sioux"; return;
    case nation_4bit_type::tupi: out += "Tupi"; return;
    case nation_4bit_type::none: out += "None"; return;
  }
  out += "<unrecognized>";
}

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
    case nation_4bit_type::arawak: return "Arawak";
    case nation_4bit_type::iroquois: return "Iroquois";
    case nation_4bit_type::cherokee: return "Cherokee";
    case nation_4bit_type::apache: return "Apache";
    case nation_4bit_type::sioux: return "Sioux";
    case nation_4bit_type::tupi: return "Tupi";
    case nation_4bit_type::none: return "None";
  }
  return cdr::null;
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
    { "Arawak", nation_4bit_type::arawak },
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
void to_str( nation_type const& o, std::string& out, base::tag<nation_type> ) {
  switch( o ) {
    case nation_type::england: out += "England"; return;
    case nation_type::france: out += "France"; return;
    case nation_type::spain: out += "Spain"; return;
    case nation_type::netherlands: out += "Netherlands"; return;
    case nation_type::inca: out += "Inca"; return;
    case nation_type::aztec: out += "Aztec"; return;
    case nation_type::arawak: out += "Arawak"; return;
    case nation_type::iroquois: out += "Iroquois"; return;
    case nation_type::cherokee: out += "Cherokee"; return;
    case nation_type::apache: out += "Apache"; return;
    case nation_type::sioux: out += "Sioux"; return;
    case nation_type::tupi: out += "Tupi"; return;
    case nation_type::none: out += "None"; return;
  }
  out += "<unrecognized>";
}

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
    case nation_type::arawak: return "Arawak";
    case nation_type::iroquois: return "Iroquois";
    case nation_type::cherokee: return "Cherokee";
    case nation_type::apache: return "Apache";
    case nation_type::sioux: return "Sioux";
    case nation_type::tupi: return "Tupi";
    case nation_type::none: return "None";
  }
  return cdr::null;
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
    { "Arawak", nation_type::arawak },
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
void to_str( occupation_type const& o, std::string& out, base::tag<occupation_type> ) {
  switch( o ) {
    case occupation_type::farmer: out += "Farmer"; return;
    case occupation_type::sugar_planter: out += "Sugar planter"; return;
    case occupation_type::tobacco_planter: out += "Tobacco planter"; return;
    case occupation_type::cotton_planter: out += "Cotton planter"; return;
    case occupation_type::fur_trapper: out += "Fur trapper"; return;
    case occupation_type::lumberjack: out += "Lumberjack"; return;
    case occupation_type::ore_miner: out += "Ore miner"; return;
    case occupation_type::silver_miner: out += "Silver miner"; return;
    case occupation_type::fisherman: out += "Fisherman"; return;
    case occupation_type::distiller: out += "Distiller"; return;
    case occupation_type::tobacconist: out += "Tobacconist"; return;
    case occupation_type::weaver: out += "Weaver"; return;
    case occupation_type::fur_trader: out += "Fur trader"; return;
    case occupation_type::carpenter: out += "Carpenter"; return;
    case occupation_type::blacksmith: out += "Blacksmith"; return;
    case occupation_type::gunsmith: out += "Gunsmith"; return;
    case occupation_type::preacher: out += "Preacher"; return;
    case occupation_type::statesman: out += "Statesman"; return;
    case occupation_type::teacher: out += "Teacher"; return;
    case occupation_type::qqqqqqqqqq: out += "??????????"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
void to_str( orders_type const& o, std::string& out, base::tag<orders_type> ) {
  switch( o ) {
    case orders_type::none: out += "none"; return;
    case orders_type::sentry: out += "sentry"; return;
    case orders_type::trading: out += "trading"; return;
    case orders_type::g0to: out += "goto"; return;
    case orders_type::fortify: out += "fortify"; return;
    case orders_type::fortified: out += "fortified"; return;
    case orders_type::plow: out += "plow"; return;
    case orders_type::road: out += "road"; return;
    case orders_type::unknowna: out += "unknowna"; return;
    case orders_type::unknownb: out += "unknownb"; return;
    case orders_type::unknownc: out += "unknownc"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         orders_type const& o,
                         cdr::tag_t<orders_type> ) {
  switch( o ) {
    case orders_type::none: return "none";
    case orders_type::sentry: return "sentry";
    case orders_type::trading: return "trading";
    case orders_type::g0to: return "goto";
    case orders_type::fortify: return "fortify";
    case orders_type::fortified: return "fortified";
    case orders_type::plow: return "plow";
    case orders_type::road: return "road";
    case orders_type::unknowna: return "unknowna";
    case orders_type::unknownb: return "unknownb";
    case orders_type::unknownc: return "unknownc";
  }
  return cdr::null;
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
    { "fortify", orders_type::fortify },
    { "fortified", orders_type::fortified },
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
void to_str( pacific_1bit_type const& o, std::string& out, base::tag<pacific_1bit_type> ) {
  switch( o ) {
    case pacific_1bit_type::empty: out += " "; return;
    case pacific_1bit_type::t: out += "~"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         pacific_1bit_type const& o,
                         cdr::tag_t<pacific_1bit_type> ) {
  switch( o ) {
    case pacific_1bit_type::empty: return " ";
    case pacific_1bit_type::t: return "~";
  }
  return cdr::null;
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
void to_str( plowed_1bit_type const& o, std::string& out, base::tag<plowed_1bit_type> ) {
  switch( o ) {
    case plowed_1bit_type::empty: out += " "; return;
    case plowed_1bit_type::h: out += "#"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         plowed_1bit_type const& o,
                         cdr::tag_t<plowed_1bit_type> ) {
  switch( o ) {
    case plowed_1bit_type::empty: return " ";
    case plowed_1bit_type::h: return "#";
  }
  return cdr::null;
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
void to_str( profession_type const& o, std::string& out, base::tag<profession_type> ) {
  switch( o ) {
    case profession_type::expert_farmer: out += "Expert farmer"; return;
    case profession_type::master_sugar_planter: out += "Master sugar planter"; return;
    case profession_type::master_tobacco_planter: out += "Master tobacco planter"; return;
    case profession_type::master_cotton_planter: out += "Master cotton planter"; return;
    case profession_type::expert_fur_trapper: out += "Expert fur trapper"; return;
    case profession_type::expert_lumberjack: out += "Expert lumberjack"; return;
    case profession_type::expert_ore_miner: out += "Expert ore miner"; return;
    case profession_type::expert_silver_miner: out += "Expert silver miner"; return;
    case profession_type::expert_fisherman: out += "Expert fisherman"; return;
    case profession_type::master_distiller: out += "Master distiller"; return;
    case profession_type::master_tobacconist: out += "Master tobacconist"; return;
    case profession_type::master_weaver: out += "Master weaver"; return;
    case profession_type::master_fur_trader: out += "Master fur trader"; return;
    case profession_type::master_carpenter: out += "Master carpenter"; return;
    case profession_type::master_blacksmith: out += "Master blacksmith"; return;
    case profession_type::master_gunsmith: out += "Master gunsmith"; return;
    case profession_type::firebrand_preacher: out += "Firebrand preacher"; return;
    case profession_type::elder_statesman: out += "Elder statesman"; return;
    case profession_type::expert_teacher: out += "Expert teacher"; return;
    case profession_type::a_free_colonist: out += "*(Free colonist)"; return;
    case profession_type::hardy_pioneer: out += "Hardy pioneer"; return;
    case profession_type::veteran_soldier: out += "Veteran soldier"; return;
    case profession_type::seasoned_scout: out += "Seasoned scout"; return;
    case profession_type::veteran_dragoon: out += "Veteran dragoon"; return;
    case profession_type::jesuit_missionary: out += "Jesuit missionary"; return;
    case profession_type::indentured_servant: out += "Indentured servant"; return;
    case profession_type::petty_criminal: out += "Petty criminal"; return;
    case profession_type::indian_convert: out += "Indian convert"; return;
    case profession_type::free_colonist: out += "Free colonist"; return;
  }
  out += "<unrecognized>";
}

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
    case profession_type::expert_teacher: return "Expert teacher";
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
  return cdr::null;
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
    { "Expert teacher", profession_type::expert_teacher },
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
void to_str( purchased_1bit_type const& o, std::string& out, base::tag<purchased_1bit_type> ) {
  switch( o ) {
    case purchased_1bit_type::empty: out += " "; return;
    case purchased_1bit_type::a: out += "*"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         purchased_1bit_type const& o,
                         cdr::tag_t<purchased_1bit_type> ) {
  switch( o ) {
    case purchased_1bit_type::empty: return " ";
    case purchased_1bit_type::a: return "*";
  }
  return cdr::null;
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
void to_str( region_id_4bit_type const& o, std::string& out, base::tag<region_id_4bit_type> ) {
  switch( o ) {
    case region_id_4bit_type::_0: out += " 0"; return;
    case region_id_4bit_type::_1: out += " 1"; return;
    case region_id_4bit_type::_2: out += " 2"; return;
    case region_id_4bit_type::_3: out += " 3"; return;
    case region_id_4bit_type::_4: out += " 4"; return;
    case region_id_4bit_type::_5: out += " 5"; return;
    case region_id_4bit_type::_6: out += " 6"; return;
    case region_id_4bit_type::_7: out += " 7"; return;
    case region_id_4bit_type::_8: out += " 8"; return;
    case region_id_4bit_type::_9: out += " 9"; return;
    case region_id_4bit_type::_10: out += "10"; return;
    case region_id_4bit_type::_11: out += "11"; return;
    case region_id_4bit_type::_12: out += "12"; return;
    case region_id_4bit_type::_13: out += "13"; return;
    case region_id_4bit_type::_14: out += "14"; return;
    case region_id_4bit_type::_15: out += "15"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
** relation_3bit_type
*****************************************************************/
void to_str( relation_3bit_type const& o, std::string& out, base::tag<relation_3bit_type> ) {
  switch( o ) {
    case relation_3bit_type::self_vanished_not_met: out += "self/vanished/not met"; return;
    case relation_3bit_type::war: out += "war"; return;
    case relation_3bit_type::post_granted_independence: out += "post_granted_independence"; return;
    case relation_3bit_type::peace: out += "peace"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         relation_3bit_type const& o,
                         cdr::tag_t<relation_3bit_type> ) {
  switch( o ) {
    case relation_3bit_type::self_vanished_not_met: return "self/vanished/not met";
    case relation_3bit_type::war: return "war";
    case relation_3bit_type::post_granted_independence: return "post_granted_independence";
    case relation_3bit_type::peace: return "peace";
  }
  return cdr::null;
}

cdr::result<relation_3bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<relation_3bit_type> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, relation_3bit_type> const m{
    { "self/vanished/not met", relation_3bit_type::self_vanished_not_met },
    { "war", relation_3bit_type::war },
    { "post_granted_independence", relation_3bit_type::post_granted_independence },
    { "peace", relation_3bit_type::peace },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "relation_3bit_type", str );
}

/****************************************************************
** road_1bit_type
*****************************************************************/
void to_str( road_1bit_type const& o, std::string& out, base::tag<road_1bit_type> ) {
  switch( o ) {
    case road_1bit_type::empty: out += " "; return;
    case road_1bit_type::e: out += "="; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         road_1bit_type const& o,
                         cdr::tag_t<road_1bit_type> ) {
  switch( o ) {
    case road_1bit_type::empty: return " ";
    case road_1bit_type::e: return "=";
  }
  return cdr::null;
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
void to_str( season_type const& o, std::string& out, base::tag<season_type> ) {
  switch( o ) {
    case season_type::spring: out += "spring"; return;
    case season_type::autumn: out += "autumn"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         season_type const& o,
                         cdr::tag_t<season_type> ) {
  switch( o ) {
    case season_type::spring: return "spring";
    case season_type::autumn: return "autumn";
  }
  return cdr::null;
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
void to_str( suppress_1bit_type const& o, std::string& out, base::tag<suppress_1bit_type> ) {
  switch( o ) {
    case suppress_1bit_type::empty: out += " "; return;
    case suppress_1bit_type::_: out += "_"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         suppress_1bit_type const& o,
                         cdr::tag_t<suppress_1bit_type> ) {
  switch( o ) {
    case suppress_1bit_type::empty: return " ";
    case suppress_1bit_type::_: return "_";
  }
  return cdr::null;
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
void to_str( tech_type const& o, std::string& out, base::tag<tech_type> ) {
  switch( o ) {
    case tech_type::semi_nomadic: out += "Semi-Nomadic"; return;
    case tech_type::agrarian: out += "Agrarian"; return;
    case tech_type::advanced: out += "Advanced"; return;
    case tech_type::civilized: out += "Civilized"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         tech_type const& o,
                         cdr::tag_t<tech_type> ) {
  switch( o ) {
    case tech_type::semi_nomadic: return "Semi-Nomadic";
    case tech_type::agrarian: return "Agrarian";
    case tech_type::advanced: return "Advanced";
    case tech_type::civilized: return "Civilized";
  }
  return cdr::null;
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
void to_str( terrain_5bit_type const& o, std::string& out, base::tag<terrain_5bit_type> ) {
  switch( o ) {
    case terrain_5bit_type::tu: out += "tu "; return;
    case terrain_5bit_type::de: out += "de "; return;
    case terrain_5bit_type::pl: out += "pl "; return;
    case terrain_5bit_type::pr: out += "pr "; return;
    case terrain_5bit_type::gr: out += "gr "; return;
    case terrain_5bit_type::sa: out += "sa "; return;
    case terrain_5bit_type::mr: out += "mr "; return;
    case terrain_5bit_type::sw: out += "sw "; return;
    case terrain_5bit_type::tuf: out += "tuF"; return;
    case terrain_5bit_type::def: out += "deF"; return;
    case terrain_5bit_type::plf: out += "plF"; return;
    case terrain_5bit_type::prf: out += "prF"; return;
    case terrain_5bit_type::grf: out += "grF"; return;
    case terrain_5bit_type::saf: out += "saF"; return;
    case terrain_5bit_type::mrf: out += "mrF"; return;
    case terrain_5bit_type::swf: out += "swF"; return;
    case terrain_5bit_type::tuw: out += "tuW"; return;
    case terrain_5bit_type::dew: out += "deW"; return;
    case terrain_5bit_type::plw: out += "plW"; return;
    case terrain_5bit_type::prw: out += "prW"; return;
    case terrain_5bit_type::grw: out += "grW"; return;
    case terrain_5bit_type::saw: out += "saW"; return;
    case terrain_5bit_type::mrw: out += "mrW"; return;
    case terrain_5bit_type::sww: out += "swW"; return;
    case terrain_5bit_type::arc: out += "arc"; return;
    case terrain_5bit_type::ttt: out += "~~~"; return;
    case terrain_5bit_type::tnt: out += "~:~"; return;
  }
  out += "<unrecognized>";
}

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
    case terrain_5bit_type::mr: return "mr ";
    case terrain_5bit_type::sw: return "sw ";
    case terrain_5bit_type::tuf: return "tuF";
    case terrain_5bit_type::def: return "deF";
    case terrain_5bit_type::plf: return "plF";
    case terrain_5bit_type::prf: return "prF";
    case terrain_5bit_type::grf: return "grF";
    case terrain_5bit_type::saf: return "saF";
    case terrain_5bit_type::mrf: return "mrF";
    case terrain_5bit_type::swf: return "swF";
    case terrain_5bit_type::tuw: return "tuW";
    case terrain_5bit_type::dew: return "deW";
    case terrain_5bit_type::plw: return "plW";
    case terrain_5bit_type::prw: return "prW";
    case terrain_5bit_type::grw: return "grW";
    case terrain_5bit_type::saw: return "saW";
    case terrain_5bit_type::mrw: return "mrW";
    case terrain_5bit_type::sww: return "swW";
    case terrain_5bit_type::arc: return "arc";
    case terrain_5bit_type::ttt: return "~~~";
    case terrain_5bit_type::tnt: return "~:~";
  }
  return cdr::null;
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
    { "mr ", terrain_5bit_type::mr },
    { "sw ", terrain_5bit_type::sw },
    { "tuF", terrain_5bit_type::tuf },
    { "deF", terrain_5bit_type::def },
    { "plF", terrain_5bit_type::plf },
    { "prF", terrain_5bit_type::prf },
    { "grF", terrain_5bit_type::grf },
    { "saF", terrain_5bit_type::saf },
    { "mrF", terrain_5bit_type::mrf },
    { "swF", terrain_5bit_type::swf },
    { "tuW", terrain_5bit_type::tuw },
    { "deW", terrain_5bit_type::dew },
    { "plW", terrain_5bit_type::plw },
    { "prW", terrain_5bit_type::prw },
    { "grW", terrain_5bit_type::grw },
    { "saW", terrain_5bit_type::saw },
    { "mrW", terrain_5bit_type::mrw },
    { "swW", terrain_5bit_type::sww },
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
void to_str( trade_route_type const& o, std::string& out, base::tag<trade_route_type> ) {
  switch( o ) {
    case trade_route_type::land: out += "land"; return;
    case trade_route_type::sea: out += "sea"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         trade_route_type const& o,
                         cdr::tag_t<trade_route_type> ) {
  switch( o ) {
    case trade_route_type::land: return "land";
    case trade_route_type::sea: return "sea";
  }
  return cdr::null;
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
void to_str( unit_type const& o, std::string& out, base::tag<unit_type> ) {
  switch( o ) {
    case unit_type::colonist: out += "Colonist"; return;
    case unit_type::soldier: out += "Soldier"; return;
    case unit_type::pioneer: out += "Pioneer"; return;
    case unit_type::missionary: out += "Missionary"; return;
    case unit_type::dragoon: out += "Dragoon"; return;
    case unit_type::scout: out += "Scout"; return;
    case unit_type::tory_regular: out += "Tory regular"; return;
    case unit_type::continental_cavalry: out += "Continental cavalry"; return;
    case unit_type::tory_cavalry: out += "Tory cavalry"; return;
    case unit_type::continental_army: out += "Continental army"; return;
    case unit_type::treasure: out += "Treasure"; return;
    case unit_type::artillery: out += "Artillery"; return;
    case unit_type::wagon_train: out += "Wagon train"; return;
    case unit_type::caravel: out += "Caravel"; return;
    case unit_type::merchantman: out += "Merchantman"; return;
    case unit_type::galleon: out += "Galleon"; return;
    case unit_type::privateer: out += "Privateer"; return;
    case unit_type::frigate: out += "Frigate"; return;
    case unit_type::man_o_war: out += "Man-O-War"; return;
    case unit_type::brave: out += "Brave"; return;
    case unit_type::armed_brave: out += "Armed brave"; return;
    case unit_type::mounted_brave: out += "Mounted brave"; return;
    case unit_type::mounted_warrior: out += "Mounted warrior"; return;
  }
  out += "<unrecognized>";
}

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
  return cdr::null;
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
void to_str( visible_to_dutch_1bit_type const& o, std::string& out, base::tag<visible_to_dutch_1bit_type> ) {
  switch( o ) {
    case visible_to_dutch_1bit_type::empty: out += " "; return;
    case visible_to_dutch_1bit_type::d: out += "D"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         visible_to_dutch_1bit_type const& o,
                         cdr::tag_t<visible_to_dutch_1bit_type> ) {
  switch( o ) {
    case visible_to_dutch_1bit_type::empty: return " ";
    case visible_to_dutch_1bit_type::d: return "D";
  }
  return cdr::null;
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
void to_str( visible_to_english_1bit_type const& o, std::string& out, base::tag<visible_to_english_1bit_type> ) {
  switch( o ) {
    case visible_to_english_1bit_type::empty: out += " "; return;
    case visible_to_english_1bit_type::e: out += "E"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         visible_to_english_1bit_type const& o,
                         cdr::tag_t<visible_to_english_1bit_type> ) {
  switch( o ) {
    case visible_to_english_1bit_type::empty: return " ";
    case visible_to_english_1bit_type::e: return "E";
  }
  return cdr::null;
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
void to_str( visible_to_french_1bit_type const& o, std::string& out, base::tag<visible_to_french_1bit_type> ) {
  switch( o ) {
    case visible_to_french_1bit_type::empty: out += " "; return;
    case visible_to_french_1bit_type::f: out += "F"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         visible_to_french_1bit_type const& o,
                         cdr::tag_t<visible_to_french_1bit_type> ) {
  switch( o ) {
    case visible_to_french_1bit_type::empty: return " ";
    case visible_to_french_1bit_type::f: return "F";
  }
  return cdr::null;
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
void to_str( visible_to_spanish_1bit_type const& o, std::string& out, base::tag<visible_to_spanish_1bit_type> ) {
  switch( o ) {
    case visible_to_spanish_1bit_type::empty: out += " "; return;
    case visible_to_spanish_1bit_type::s: out += "S"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         visible_to_spanish_1bit_type const& o,
                         cdr::tag_t<visible_to_spanish_1bit_type> ) {
  switch( o ) {
    case visible_to_spanish_1bit_type::empty: return " ";
    case visible_to_spanish_1bit_type::s: return "S";
  }
  return cdr::null;
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
** yes_no_byte
*****************************************************************/
void to_str( yes_no_byte const& o, std::string& out, base::tag<yes_no_byte> ) {
  switch( o ) {
    case yes_no_byte::no: out += "no"; return;
    case yes_no_byte::yes: out += "yes"; return;
  }
  out += "<unrecognized>";
}

cdr::value to_canonical( cdr::converter&,
                         yes_no_byte const& o,
                         cdr::tag_t<yes_no_byte> ) {
  switch( o ) {
    case yes_no_byte::no: return "no";
    case yes_no_byte::yes: return "yes";
  }
  return cdr::null;
}

cdr::result<yes_no_byte> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<yes_no_byte> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  static std::map<std::string, yes_no_byte> const m{
    { "no", yes_no_byte::no },
    { "yes", yes_no_byte::yes },
  };
  if( auto it = m.find( str ); it != m.end() )
    return it->second;
  else
    return BAD_ENUM_STR_VALUE( "yes_no_byte", str );
}

/****************************************************************
** TutorialHelp
*****************************************************************/
void to_str( TutorialHelp const& o, std::string& out, base::tag<TutorialHelp> ) {
  out += "TutorialHelp{";
  out += "hint_pioneer="; base::to_str( o.hint_pioneer, out ); out += ',';
  out += "hint_soldier="; base::to_str( o.hint_soldier, out ); out += ',';
  out += "unknown02="; base::to_str( o.unknown02, out ); out += ',';
  out += "hint_new_colonist_in_colony="; base::to_str( o.hint_new_colonist_in_colony, out ); out += ',';
  out += "hint_food_deficit="; base::to_str( o.hint_food_deficit, out ); out += ',';
  out += "hint_harbor="; base::to_str( o.hint_harbor, out ); out += ',';
  out += "unknown06="; base::to_str( o.unknown06, out ); out += ',';
  out += "hint_native_convert="; base::to_str( o.hint_native_convert, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TutorialHelp& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.hint_pioneer = (bits & 0b1); bits >>= 1;
  o.hint_soldier = (bits & 0b1); bits >>= 1;
  o.unknown02 = (bits & 0b1); bits >>= 1;
  o.hint_new_colonist_in_colony = (bits & 0b1); bits >>= 1;
  o.hint_food_deficit = (bits & 0b1); bits >>= 1;
  o.hint_harbor = (bits & 0b1); bits >>= 1;
  o.unknown06 = (bits & 0b1); bits >>= 1;
  o.hint_native_convert = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, TutorialHelp const& o ) {
  uint8_t bits = 0;
  bits |= (o.hint_native_convert & 0b1); bits <<= 1;
  bits |= (o.unknown06 & 0b1); bits <<= 1;
  bits |= (o.hint_harbor & 0b1); bits <<= 1;
  bits |= (o.hint_food_deficit & 0b1); bits <<= 1;
  bits |= (o.hint_new_colonist_in_colony & 0b1); bits <<= 1;
  bits |= (o.unknown02 & 0b1); bits <<= 1;
  bits |= (o.hint_soldier & 0b1); bits <<= 1;
  bits |= (o.hint_pioneer & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         TutorialHelp const& o,
                         cdr::tag_t<TutorialHelp> ) {
  cdr::table tbl;
  conv.to_field( tbl, "hint_pioneer", o.hint_pioneer );
  conv.to_field( tbl, "hint_soldier", o.hint_soldier );
  conv.to_field( tbl, "unknown02", o.unknown02 );
  conv.to_field( tbl, "hint_new_colonist_in_colony", o.hint_new_colonist_in_colony );
  conv.to_field( tbl, "hint_food_deficit", o.hint_food_deficit );
  conv.to_field( tbl, "hint_harbor", o.hint_harbor );
  conv.to_field( tbl, "unknown06", o.unknown06 );
  conv.to_field( tbl, "hint_native_convert", o.hint_native_convert );
  tbl["__key_order"] = cdr::list{
    "hint_pioneer",
    "hint_soldier",
    "unknown02",
    "hint_new_colonist_in_colony",
    "hint_food_deficit",
    "hint_harbor",
    "unknown06",
    "hint_native_convert",
  };
  return tbl;
}

cdr::result<TutorialHelp> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TutorialHelp> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TutorialHelp res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "hint_pioneer", hint_pioneer );
  CONV_FROM_FIELD( "hint_soldier", hint_soldier );
  CONV_FROM_FIELD( "unknown02", unknown02 );
  CONV_FROM_FIELD( "hint_new_colonist_in_colony", hint_new_colonist_in_colony );
  CONV_FROM_FIELD( "hint_food_deficit", hint_food_deficit );
  CONV_FROM_FIELD( "hint_harbor", hint_harbor );
  CONV_FROM_FIELD( "unknown06", unknown06 );
  CONV_FROM_FIELD( "hint_native_convert", hint_native_convert );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** GameFlags1
*****************************************************************/
void to_str( GameFlags1 const& o, std::string& out, base::tag<GameFlags1> ) {
  out += "GameFlags1{";
  out += "independence_declared="; base::to_str( o.independence_declared, out ); out += ',';
  out += "deploy_intervention_force="; base::to_str( o.deploy_intervention_force, out ); out += ',';
  out += "independence_war_intro="; base::to_str( o.independence_war_intro, out ); out += ',';
  out += "won_independence="; base::to_str( o.won_independence, out ); out += ',';
  out += "score_sequence_done="; base::to_str( o.score_sequence_done, out ); out += ',';
  out += "ref_will_forfeight="; base::to_str( o.ref_will_forfeight, out ); out += ',';
  out += "ref_captured_colony="; base::to_str( o.ref_captured_colony, out ); out += ',';
  out += "tutorial_hints="; base::to_str( o.tutorial_hints, out ); out += ',';
  out += "disable_water_color_cycling="; base::to_str( o.disable_water_color_cycling, out ); out += ',';
  out += "combat_analysis="; base::to_str( o.combat_analysis, out ); out += ',';
  out += "autosave="; base::to_str( o.autosave, out ); out += ',';
  out += "end_of_turn="; base::to_str( o.end_of_turn, out ); out += ',';
  out += "fast_piece_slide="; base::to_str( o.fast_piece_slide, out ); out += ',';
  out += "cheats_enabled="; base::to_str( o.cheats_enabled, out ); out += ',';
  out += "show_foreign_moves="; base::to_str( o.show_foreign_moves, out ); out += ',';
  out += "show_indian_moves="; base::to_str( o.show_indian_moves, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, GameFlags1& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.independence_declared = (bits & 0b1); bits >>= 1;
  o.deploy_intervention_force = (bits & 0b1); bits >>= 1;
  o.independence_war_intro = (bits & 0b1); bits >>= 1;
  o.won_independence = (bits & 0b1); bits >>= 1;
  o.score_sequence_done = (bits & 0b1); bits >>= 1;
  o.ref_will_forfeight = (bits & 0b1); bits >>= 1;
  o.ref_captured_colony = (bits & 0b1); bits >>= 1;
  o.tutorial_hints = (bits & 0b1); bits >>= 1;
  o.disable_water_color_cycling = (bits & 0b1); bits >>= 1;
  o.combat_analysis = (bits & 0b1); bits >>= 1;
  o.autosave = (bits & 0b1); bits >>= 1;
  o.end_of_turn = (bits & 0b1); bits >>= 1;
  o.fast_piece_slide = (bits & 0b1); bits >>= 1;
  o.cheats_enabled = (bits & 0b1); bits >>= 1;
  o.show_foreign_moves = (bits & 0b1); bits >>= 1;
  o.show_indian_moves = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, GameFlags1 const& o ) {
  uint16_t bits = 0;
  bits |= (o.show_indian_moves & 0b1); bits <<= 1;
  bits |= (o.show_foreign_moves & 0b1); bits <<= 1;
  bits |= (o.cheats_enabled & 0b1); bits <<= 1;
  bits |= (o.fast_piece_slide & 0b1); bits <<= 1;
  bits |= (o.end_of_turn & 0b1); bits <<= 1;
  bits |= (o.autosave & 0b1); bits <<= 1;
  bits |= (o.combat_analysis & 0b1); bits <<= 1;
  bits |= (o.disable_water_color_cycling & 0b1); bits <<= 1;
  bits |= (o.tutorial_hints & 0b1); bits <<= 1;
  bits |= (o.ref_captured_colony & 0b1); bits <<= 1;
  bits |= (o.ref_will_forfeight & 0b1); bits <<= 1;
  bits |= (o.score_sequence_done & 0b1); bits <<= 1;
  bits |= (o.won_independence & 0b1); bits <<= 1;
  bits |= (o.independence_war_intro & 0b1); bits <<= 1;
  bits |= (o.deploy_intervention_force & 0b1); bits <<= 1;
  bits |= (o.independence_declared & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         GameFlags1 const& o,
                         cdr::tag_t<GameFlags1> ) {
  cdr::table tbl;
  conv.to_field( tbl, "independence_declared", o.independence_declared );
  conv.to_field( tbl, "deploy_intervention_force", o.deploy_intervention_force );
  conv.to_field( tbl, "independence_war_intro", o.independence_war_intro );
  conv.to_field( tbl, "won_independence", o.won_independence );
  conv.to_field( tbl, "score_sequence_done", o.score_sequence_done );
  conv.to_field( tbl, "ref_will_forfeight", o.ref_will_forfeight );
  conv.to_field( tbl, "ref_captured_colony", o.ref_captured_colony );
  conv.to_field( tbl, "tutorial_hints", o.tutorial_hints );
  conv.to_field( tbl, "disable_water_color_cycling", o.disable_water_color_cycling );
  conv.to_field( tbl, "combat_analysis", o.combat_analysis );
  conv.to_field( tbl, "autosave", o.autosave );
  conv.to_field( tbl, "end_of_turn", o.end_of_turn );
  conv.to_field( tbl, "fast_piece_slide", o.fast_piece_slide );
  conv.to_field( tbl, "cheats_enabled", o.cheats_enabled );
  conv.to_field( tbl, "show_foreign_moves", o.show_foreign_moves );
  conv.to_field( tbl, "show_indian_moves", o.show_indian_moves );
  tbl["__key_order"] = cdr::list{
    "independence_declared",
    "deploy_intervention_force",
    "independence_war_intro",
    "won_independence",
    "score_sequence_done",
    "ref_will_forfeight",
    "ref_captured_colony",
    "tutorial_hints",
    "disable_water_color_cycling",
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

cdr::result<GameFlags1> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<GameFlags1> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  GameFlags1 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "independence_declared", independence_declared );
  CONV_FROM_FIELD( "deploy_intervention_force", deploy_intervention_force );
  CONV_FROM_FIELD( "independence_war_intro", independence_war_intro );
  CONV_FROM_FIELD( "won_independence", won_independence );
  CONV_FROM_FIELD( "score_sequence_done", score_sequence_done );
  CONV_FROM_FIELD( "ref_will_forfeight", ref_will_forfeight );
  CONV_FROM_FIELD( "ref_captured_colony", ref_captured_colony );
  CONV_FROM_FIELD( "tutorial_hints", tutorial_hints );
  CONV_FROM_FIELD( "disable_water_color_cycling", disable_water_color_cycling );
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
** ColonyReportOptionsToDisable
*****************************************************************/
void to_str( ColonyReportOptionsToDisable const& o, std::string& out, base::tag<ColonyReportOptionsToDisable> ) {
  out += "ColonyReportOptionsToDisable{";
  out += "labels_on_cargo_and_terrain="; base::to_str( o.labels_on_cargo_and_terrain, out ); out += ',';
  out += "labels_on_buildings="; base::to_str( o.labels_on_buildings, out ); out += ',';
  out += "report_new_cargos_available="; base::to_str( o.report_new_cargos_available, out ); out += ',';
  out += "report_inefficient_government="; base::to_str( o.report_inefficient_government, out ); out += ',';
  out += "report_tools_needed_for_production="; base::to_str( o.report_tools_needed_for_production, out ); out += ',';
  out += "report_raw_materials_shortages="; base::to_str( o.report_raw_materials_shortages, out ); out += ',';
  out += "report_food_shortages="; base::to_str( o.report_food_shortages, out ); out += ',';
  out += "report_when_colonists_trained="; base::to_str( o.report_when_colonists_trained, out ); out += ',';
  out += "report_sons_of_liberty_membership="; base::to_str( o.report_sons_of_liberty_membership, out ); out += ',';
  out += "report_rebel_majorities="; base::to_str( o.report_rebel_majorities, out ); out += ',';
  out += "unused03="; base::to_str( bits<6>{ o.unused03 }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ColonyReportOptionsToDisable& o ) {
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

bool write_binary( base::IBinaryIO& b, ColonyReportOptionsToDisable const& o ) {
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
                         ColonyReportOptionsToDisable const& o,
                         cdr::tag_t<ColonyReportOptionsToDisable> ) {
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

cdr::result<ColonyReportOptionsToDisable> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonyReportOptionsToDisable> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ColonyReportOptionsToDisable res = {};
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
** GameFlags2
*****************************************************************/
void to_str( GameFlags2 const& o, std::string& out, base::tag<GameFlags2> ) {
  out += "GameFlags2{";
  out += "how_to_win="; base::to_str( o.how_to_win, out ); out += ',';
  out += "background_music="; base::to_str( o.background_music, out ); out += ',';
  out += "event_music="; base::to_str( o.event_music, out ); out += ',';
  out += "sound_effects="; base::to_str( o.sound_effects, out ); out += ',';
  out += "hint_how_to_move_ship="; base::to_str( o.hint_how_to_move_ship, out ); out += ',';
  out += "unknown_hint01="; base::to_str( o.unknown_hint01, out ); out += ',';
  out += "hint_lumber_abundance="; base::to_str( o.hint_lumber_abundance, out ); out += ',';
  out += "hint_colony_view="; base::to_str( o.hint_colony_view, out ); out += ',';
  out += "hint_dock_units_waiting="; base::to_str( o.hint_dock_units_waiting, out ); out += ',';
  out += "hint_full_cargo="; base::to_str( o.hint_full_cargo, out ); out += ',';
  out += "hint_build_stockade="; base::to_str( o.hint_build_stockade, out ); out += ',';
  out += "hint_free_colonist="; base::to_str( o.hint_free_colonist, out ); out += ',';
  out += "unknown_hint08="; base::to_str( o.unknown_hint08, out ); out += ',';
  out += "unknown_hint09="; base::to_str( o.unknown_hint09, out ); out += ',';
  out += "hint_ship_valuable="; base::to_str( o.hint_ship_valuable, out ); out += ',';
  out += "hint_ship_in_colony="; base::to_str( o.hint_ship_in_colony, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, GameFlags2& o ) {
  uint16_t bits = 0;
  if( !b.read_bytes<2>( bits ) ) return false;
  o.how_to_win = (bits & 0b1); bits >>= 1;
  o.background_music = (bits & 0b1); bits >>= 1;
  o.event_music = (bits & 0b1); bits >>= 1;
  o.sound_effects = (bits & 0b1); bits >>= 1;
  o.hint_how_to_move_ship = (bits & 0b1); bits >>= 1;
  o.unknown_hint01 = (bits & 0b1); bits >>= 1;
  o.hint_lumber_abundance = (bits & 0b1); bits >>= 1;
  o.hint_colony_view = (bits & 0b1); bits >>= 1;
  o.hint_dock_units_waiting = (bits & 0b1); bits >>= 1;
  o.hint_full_cargo = (bits & 0b1); bits >>= 1;
  o.hint_build_stockade = (bits & 0b1); bits >>= 1;
  o.hint_free_colonist = (bits & 0b1); bits >>= 1;
  o.unknown_hint08 = (bits & 0b1); bits >>= 1;
  o.unknown_hint09 = (bits & 0b1); bits >>= 1;
  o.hint_ship_valuable = (bits & 0b1); bits >>= 1;
  o.hint_ship_in_colony = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, GameFlags2 const& o ) {
  uint16_t bits = 0;
  bits |= (o.hint_ship_in_colony & 0b1); bits <<= 1;
  bits |= (o.hint_ship_valuable & 0b1); bits <<= 1;
  bits |= (o.unknown_hint09 & 0b1); bits <<= 1;
  bits |= (o.unknown_hint08 & 0b1); bits <<= 1;
  bits |= (o.hint_free_colonist & 0b1); bits <<= 1;
  bits |= (o.hint_build_stockade & 0b1); bits <<= 1;
  bits |= (o.hint_full_cargo & 0b1); bits <<= 1;
  bits |= (o.hint_dock_units_waiting & 0b1); bits <<= 1;
  bits |= (o.hint_colony_view & 0b1); bits <<= 1;
  bits |= (o.hint_lumber_abundance & 0b1); bits <<= 1;
  bits |= (o.unknown_hint01 & 0b1); bits <<= 1;
  bits |= (o.hint_how_to_move_ship & 0b1); bits <<= 1;
  bits |= (o.sound_effects & 0b1); bits <<= 1;
  bits |= (o.event_music & 0b1); bits <<= 1;
  bits |= (o.background_music & 0b1); bits <<= 1;
  bits |= (o.how_to_win & 0b1); bits <<= 0;
  return b.write_bytes<2>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         GameFlags2 const& o,
                         cdr::tag_t<GameFlags2> ) {
  cdr::table tbl;
  conv.to_field( tbl, "how_to_win", o.how_to_win );
  conv.to_field( tbl, "background_music", o.background_music );
  conv.to_field( tbl, "event_music", o.event_music );
  conv.to_field( tbl, "sound_effects", o.sound_effects );
  conv.to_field( tbl, "hint_how_to_move_ship", o.hint_how_to_move_ship );
  conv.to_field( tbl, "unknown_hint01", o.unknown_hint01 );
  conv.to_field( tbl, "hint_lumber_abundance", o.hint_lumber_abundance );
  conv.to_field( tbl, "hint_colony_view", o.hint_colony_view );
  conv.to_field( tbl, "hint_dock_units_waiting", o.hint_dock_units_waiting );
  conv.to_field( tbl, "hint_full_cargo", o.hint_full_cargo );
  conv.to_field( tbl, "hint_build_stockade", o.hint_build_stockade );
  conv.to_field( tbl, "hint_free_colonist", o.hint_free_colonist );
  conv.to_field( tbl, "unknown_hint08", o.unknown_hint08 );
  conv.to_field( tbl, "unknown_hint09", o.unknown_hint09 );
  conv.to_field( tbl, "hint_ship_valuable", o.hint_ship_valuable );
  conv.to_field( tbl, "hint_ship_in_colony", o.hint_ship_in_colony );
  tbl["__key_order"] = cdr::list{
    "how_to_win",
    "background_music",
    "event_music",
    "sound_effects",
    "hint_how_to_move_ship",
    "unknown_hint01",
    "hint_lumber_abundance",
    "hint_colony_view",
    "hint_dock_units_waiting",
    "hint_full_cargo",
    "hint_build_stockade",
    "hint_free_colonist",
    "unknown_hint08",
    "unknown_hint09",
    "hint_ship_valuable",
    "hint_ship_in_colony",
  };
  return tbl;
}

cdr::result<GameFlags2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<GameFlags2> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  GameFlags2 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "how_to_win", how_to_win );
  CONV_FROM_FIELD( "background_music", background_music );
  CONV_FROM_FIELD( "event_music", event_music );
  CONV_FROM_FIELD( "sound_effects", sound_effects );
  CONV_FROM_FIELD( "hint_how_to_move_ship", hint_how_to_move_ship );
  CONV_FROM_FIELD( "unknown_hint01", unknown_hint01 );
  CONV_FROM_FIELD( "hint_lumber_abundance", hint_lumber_abundance );
  CONV_FROM_FIELD( "hint_colony_view", hint_colony_view );
  CONV_FROM_FIELD( "hint_dock_units_waiting", hint_dock_units_waiting );
  CONV_FROM_FIELD( "hint_full_cargo", hint_full_cargo );
  CONV_FROM_FIELD( "hint_build_stockade", hint_build_stockade );
  CONV_FROM_FIELD( "hint_free_colonist", hint_free_colonist );
  CONV_FROM_FIELD( "unknown_hint08", unknown_hint08 );
  CONV_FROM_FIELD( "unknown_hint09", unknown_hint09 );
  CONV_FROM_FIELD( "hint_ship_valuable", hint_ship_valuable );
  CONV_FROM_FIELD( "hint_ship_in_colony", hint_ship_in_colony );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Event
*****************************************************************/
void to_str( Event const& o, std::string& out, base::tag<Event> ) {
  out += "Event{";
  out += "discovery_of_the_new_world="; base::to_str( o.discovery_of_the_new_world, out ); out += ',';
  out += "building_a_colony="; base::to_str( o.building_a_colony, out ); out += ',';
  out += "meeting_the_natives="; base::to_str( o.meeting_the_natives, out ); out += ',';
  out += "the_aztec_empire="; base::to_str( o.the_aztec_empire, out ); out += ',';
  out += "the_inca_nation="; base::to_str( o.the_inca_nation, out ); out += ',';
  out += "discovery_of_the_pacific_ocean="; base::to_str( o.discovery_of_the_pacific_ocean, out ); out += ',';
  out += "entering_indian_village="; base::to_str( o.entering_indian_village, out ); out += ',';
  out += "the_fountain_of_youth="; base::to_str( o.the_fountain_of_youth, out ); out += ',';
  out += "cargo_from_the_new_world="; base::to_str( o.cargo_from_the_new_world, out ); out += ',';
  out += "meeting_fellow_europeans="; base::to_str( o.meeting_fellow_europeans, out ); out += ',';
  out += "colony_burning="; base::to_str( o.colony_burning, out ); out += ',';
  out += "colony_destroyed="; base::to_str( o.colony_destroyed, out ); out += ',';
  out += "indian_raid="; base::to_str( o.indian_raid, out ); out += ',';
  out += "woodcut14="; base::to_str( o.woodcut14, out ); out += ',';
  out += "woodcut15="; base::to_str( o.woodcut15, out ); out += ',';
  out += "woodcut16="; base::to_str( o.woodcut16, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Event& o ) {
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

bool write_binary( base::IBinaryIO& b, Event const& o ) {
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
** PlayerFlags
*****************************************************************/
void to_str( PlayerFlags const& o, std::string& out, base::tag<PlayerFlags> ) {
  out += "PlayerFlags{";
  out += "unknown06a="; base::to_str( bits<7>{ o.unknown06a }, out ); out += ',';
  out += "named_new_world="; base::to_str( o.named_new_world, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PlayerFlags& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unknown06a = (bits & 0b1111111); bits >>= 7;
  o.named_new_world = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, PlayerFlags const& o ) {
  uint8_t bits = 0;
  bits |= (o.named_new_world & 0b1); bits <<= 7;
  bits |= (o.unknown06a & 0b1111111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         PlayerFlags const& o,
                         cdr::tag_t<PlayerFlags> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown06a", bits<7>{ o.unknown06a } );
  conv.to_field( tbl, "named_new_world", o.named_new_world );
  tbl["__key_order"] = cdr::list{
    "unknown06a",
    "named_new_world",
  };
  return tbl;
}

cdr::result<PlayerFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PlayerFlags> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  PlayerFlags res = {};
  std::set<std::string> used_keys;
  CONV_FROM_BITSTRING_FIELD( "unknown06a", unknown06a, 7 );
  CONV_FROM_FIELD( "named_new_world", named_new_world );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** ColonyFlags
*****************************************************************/
void to_str( ColonyFlags const& o, std::string& out, base::tag<ColonyFlags> ) {
  out += "ColonyFlags{";
  out += "tory_uprising="; base::to_str( o.tory_uprising, out ); out += ',';
  out += "level2_sol_bonus="; base::to_str( o.level2_sol_bonus, out ); out += ',';
  out += "level1_sol_bonus="; base::to_str( o.level1_sol_bonus, out ); out += ',';
  out += "inefficient_govt_notified="; base::to_str( o.inefficient_govt_notified, out ); out += ',';
  out += "unknown04="; base::to_str( o.unknown04, out ); out += ',';
  out += "unknown05="; base::to_str( o.unknown05, out ); out += ',';
  out += "port_colony="; base::to_str( o.port_colony, out ); out += ',';
  out += "construction_complete_blinking="; base::to_str( o.construction_complete_blinking, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ColonyFlags& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.tory_uprising = (bits & 0b1); bits >>= 1;
  o.level2_sol_bonus = (bits & 0b1); bits >>= 1;
  o.level1_sol_bonus = (bits & 0b1); bits >>= 1;
  o.inefficient_govt_notified = (bits & 0b1); bits >>= 1;
  o.unknown04 = (bits & 0b1); bits >>= 1;
  o.unknown05 = (bits & 0b1); bits >>= 1;
  o.port_colony = (bits & 0b1); bits >>= 1;
  o.construction_complete_blinking = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, ColonyFlags const& o ) {
  uint8_t bits = 0;
  bits |= (o.construction_complete_blinking & 0b1); bits <<= 1;
  bits |= (o.port_colony & 0b1); bits <<= 1;
  bits |= (o.unknown05 & 0b1); bits <<= 1;
  bits |= (o.unknown04 & 0b1); bits <<= 1;
  bits |= (o.inefficient_govt_notified & 0b1); bits <<= 1;
  bits |= (o.level1_sol_bonus & 0b1); bits <<= 1;
  bits |= (o.level2_sol_bonus & 0b1); bits <<= 1;
  bits |= (o.tory_uprising & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         ColonyFlags const& o,
                         cdr::tag_t<ColonyFlags> ) {
  cdr::table tbl;
  conv.to_field( tbl, "tory_uprising", o.tory_uprising );
  conv.to_field( tbl, "level2_sol_bonus", o.level2_sol_bonus );
  conv.to_field( tbl, "level1_sol_bonus", o.level1_sol_bonus );
  conv.to_field( tbl, "inefficient_govt_notified", o.inefficient_govt_notified );
  conv.to_field( tbl, "unknown04", o.unknown04 );
  conv.to_field( tbl, "unknown05", o.unknown05 );
  conv.to_field( tbl, "port_colony", o.port_colony );
  conv.to_field( tbl, "construction_complete_blinking", o.construction_complete_blinking );
  tbl["__key_order"] = cdr::list{
    "tory_uprising",
    "level2_sol_bonus",
    "level1_sol_bonus",
    "inefficient_govt_notified",
    "unknown04",
    "unknown05",
    "port_colony",
    "construction_complete_blinking",
  };
  return tbl;
}

cdr::result<ColonyFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonyFlags> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ColonyFlags res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "tory_uprising", tory_uprising );
  CONV_FROM_FIELD( "level2_sol_bonus", level2_sol_bonus );
  CONV_FROM_FIELD( "level1_sol_bonus", level1_sol_bonus );
  CONV_FROM_FIELD( "inefficient_govt_notified", inefficient_govt_notified );
  CONV_FROM_FIELD( "unknown04", unknown04 );
  CONV_FROM_FIELD( "unknown05", unknown05 );
  CONV_FROM_FIELD( "port_colony", port_colony );
  CONV_FROM_FIELD( "construction_complete_blinking", construction_complete_blinking );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Duration
*****************************************************************/
void to_str( Duration const& o, std::string& out, base::tag<Duration> ) {
  out += "Duration{";
  out += "dur_1="; base::to_str( o.dur_1, out ); out += ',';
  out += "dur_2="; base::to_str( o.dur_2, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Duration& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.dur_1 = (bits & 0b1111); bits >>= 4;
  o.dur_2 = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, Duration const& o ) {
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
void to_str( Buildings const& o, std::string& out, base::tag<Buildings> ) {
  out += "Buildings{";
  out += "fortification="; base::to_str( o.fortification, out ); out += ',';
  out += "armory="; base::to_str( o.armory, out ); out += ',';
  out += "docks="; base::to_str( o.docks, out ); out += ',';
  out += "town_hall="; base::to_str( o.town_hall, out ); out += ',';
  out += "schoolhouse="; base::to_str( o.schoolhouse, out ); out += ',';
  out += "warehouse="; base::to_str( o.warehouse, out ); out += ',';
  out += "unused05a="; base::to_str( o.unused05a, out ); out += ',';
  out += "stables="; base::to_str( o.stables, out ); out += ',';
  out += "custom_house="; base::to_str( o.custom_house, out ); out += ',';
  out += "printing_press="; base::to_str( o.printing_press, out ); out += ',';
  out += "weavers_house="; base::to_str( o.weavers_house, out ); out += ',';
  out += "tobacconists_house="; base::to_str( o.tobacconists_house, out ); out += ',';
  out += "rum_distillers_house="; base::to_str( o.rum_distillers_house, out ); out += ',';
  out += "capitol_unused="; base::to_str( o.capitol_unused, out ); out += ',';
  out += "fur_traders_house="; base::to_str( o.fur_traders_house, out ); out += ',';
  out += "carpenters_shop="; base::to_str( o.carpenters_shop, out ); out += ',';
  out += "church="; base::to_str( o.church, out ); out += ',';
  out += "blacksmiths_house="; base::to_str( o.blacksmiths_house, out ); out += ',';
  out += "unused05b="; base::to_str( bits<6>{ o.unused05b }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Buildings& o ) {
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

bool write_binary( base::IBinaryIO& b, Buildings const& o ) {
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
void to_str( CustomHouseFlags const& o, std::string& out, base::tag<CustomHouseFlags> ) {
  out += "CustomHouseFlags{";
  out += "food="; base::to_str( o.food, out ); out += ',';
  out += "sugar="; base::to_str( o.sugar, out ); out += ',';
  out += "tobacco="; base::to_str( o.tobacco, out ); out += ',';
  out += "cotton="; base::to_str( o.cotton, out ); out += ',';
  out += "furs="; base::to_str( o.furs, out ); out += ',';
  out += "lumber="; base::to_str( o.lumber, out ); out += ',';
  out += "ore="; base::to_str( o.ore, out ); out += ',';
  out += "silver="; base::to_str( o.silver, out ); out += ',';
  out += "horses="; base::to_str( o.horses, out ); out += ',';
  out += "rum="; base::to_str( o.rum, out ); out += ',';
  out += "cigars="; base::to_str( o.cigars, out ); out += ',';
  out += "cloth="; base::to_str( o.cloth, out ); out += ',';
  out += "coats="; base::to_str( o.coats, out ); out += ',';
  out += "trade_goods="; base::to_str( o.trade_goods, out ); out += ',';
  out += "tools="; base::to_str( o.tools, out ); out += ',';
  out += "muskets="; base::to_str( o.muskets, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CustomHouseFlags& o ) {
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

bool write_binary( base::IBinaryIO& b, CustomHouseFlags const& o ) {
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
void to_str( NationInfo const& o, std::string& out, base::tag<NationInfo> ) {
  out += "NationInfo{";
  out += "nation_id="; base::to_str( o.nation_id, out ); out += ',';
  out += "vis_to_english="; base::to_str( o.vis_to_english, out ); out += ',';
  out += "vis_to_french="; base::to_str( o.vis_to_french, out ); out += ',';
  out += "vis_to_spanish="; base::to_str( o.vis_to_spanish, out ); out += ',';
  out += "vis_to_dutch="; base::to_str( o.vis_to_dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationInfo& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.nation_id = static_cast<nation_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.vis_to_english = (bits & 0b1); bits >>= 1;
  o.vis_to_french = (bits & 0b1); bits >>= 1;
  o.vis_to_spanish = (bits & 0b1); bits >>= 1;
  o.vis_to_dutch = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, NationInfo const& o ) {
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
void to_str( Unknown15 const& o, std::string& out, base::tag<Unknown15> ) {
  out += "Unknown15{";
  out += "unknown15a="; base::to_str( bits<7>{ o.unknown15a }, out ); out += ',';
  out += "damaged="; base::to_str( o.damaged, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown15& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unknown15a = (bits & 0b1111111); bits >>= 7;
  o.damaged = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, Unknown15 const& o ) {
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
void to_str( CargoItems const& o, std::string& out, base::tag<CargoItems> ) {
  out += "CargoItems{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CargoItems& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.cargo_1 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.cargo_2 = static_cast<cargo_4bit_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, CargoItems const& o ) {
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
** NationFlags
*****************************************************************/
void to_str( NationFlags const& o, std::string& out, base::tag<NationFlags> ) {
  out += "NationFlags{";
  out += "unknown19a="; base::to_str( bits<2>{ o.unknown19a }, out ); out += ',';
  out += "granted_independence="; base::to_str( o.granted_independence, out ); out += ',';
  out += "promoted_continental_units="; base::to_str( o.promoted_continental_units, out ); out += ',';
  out += "unknown19b="; base::to_str( bits<2>{ o.unknown19b }, out ); out += ',';
  out += "immigration_started="; base::to_str( o.immigration_started, out ); out += ',';
  out += "unknown19c="; base::to_str( bits<1>{ o.unknown19c }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationFlags& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unknown19a = (bits & 0b11); bits >>= 2;
  o.granted_independence = (bits & 0b1); bits >>= 1;
  o.promoted_continental_units = (bits & 0b1); bits >>= 1;
  o.unknown19b = (bits & 0b11); bits >>= 2;
  o.immigration_started = (bits & 0b1); bits >>= 1;
  o.unknown19c = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, NationFlags const& o ) {
  uint8_t bits = 0;
  bits |= (o.unknown19c & 0b1); bits <<= 1;
  bits |= (o.immigration_started & 0b1); bits <<= 2;
  bits |= (o.unknown19b & 0b11); bits <<= 1;
  bits |= (o.promoted_continental_units & 0b1); bits <<= 1;
  bits |= (o.granted_independence & 0b1); bits <<= 2;
  bits |= (o.unknown19a & 0b11); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         NationFlags const& o,
                         cdr::tag_t<NationFlags> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown19a", bits<2>{ o.unknown19a } );
  conv.to_field( tbl, "granted_independence", o.granted_independence );
  conv.to_field( tbl, "promoted_continental_units", o.promoted_continental_units );
  conv.to_field( tbl, "unknown19b", bits<2>{ o.unknown19b } );
  conv.to_field( tbl, "immigration_started", o.immigration_started );
  conv.to_field( tbl, "unknown19c", bits<1>{ o.unknown19c } );
  tbl["__key_order"] = cdr::list{
    "unknown19a",
    "granted_independence",
    "promoted_continental_units",
    "unknown19b",
    "immigration_started",
    "unknown19c",
  };
  return tbl;
}

cdr::result<NationFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationFlags> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  NationFlags res = {};
  std::set<std::string> used_keys;
  CONV_FROM_BITSTRING_FIELD( "unknown19a", unknown19a, 2 );
  CONV_FROM_FIELD( "granted_independence", granted_independence );
  CONV_FROM_FIELD( "promoted_continental_units", promoted_continental_units );
  CONV_FROM_BITSTRING_FIELD( "unknown19b", unknown19b, 2 );
  CONV_FROM_FIELD( "immigration_started", immigration_started );
  CONV_FROM_BITSTRING_FIELD( "unknown19c", unknown19c, 1 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** FoundingFathers
*****************************************************************/
void to_str( FoundingFathers const& o, std::string& out, base::tag<FoundingFathers> ) {
  out += "FoundingFathers{";
  out += "adam_smith="; base::to_str( o.adam_smith, out ); out += ',';
  out += "jakob_fugger="; base::to_str( o.jakob_fugger, out ); out += ',';
  out += "peter_minuit="; base::to_str( o.peter_minuit, out ); out += ',';
  out += "peter_stuyvesant="; base::to_str( o.peter_stuyvesant, out ); out += ',';
  out += "jan_de_witt="; base::to_str( o.jan_de_witt, out ); out += ',';
  out += "ferdinand_magellan="; base::to_str( o.ferdinand_magellan, out ); out += ',';
  out += "francisco_coronado="; base::to_str( o.francisco_coronado, out ); out += ',';
  out += "hernando_de_soto="; base::to_str( o.hernando_de_soto, out ); out += ',';
  out += "henry_hudson="; base::to_str( o.henry_hudson, out ); out += ',';
  out += "sieur_de_la_salle="; base::to_str( o.sieur_de_la_salle, out ); out += ',';
  out += "hernan_cortes="; base::to_str( o.hernan_cortes, out ); out += ',';
  out += "george_washington="; base::to_str( o.george_washington, out ); out += ',';
  out += "paul_revere="; base::to_str( o.paul_revere, out ); out += ',';
  out += "francis_drake="; base::to_str( o.francis_drake, out ); out += ',';
  out += "john_paul_jones="; base::to_str( o.john_paul_jones, out ); out += ',';
  out += "thomas_jefferson="; base::to_str( o.thomas_jefferson, out ); out += ',';
  out += "pocahontas="; base::to_str( o.pocahontas, out ); out += ',';
  out += "thomas_paine="; base::to_str( o.thomas_paine, out ); out += ',';
  out += "simon_bolivar="; base::to_str( o.simon_bolivar, out ); out += ',';
  out += "benjamin_franklin="; base::to_str( o.benjamin_franklin, out ); out += ',';
  out += "william_brewster="; base::to_str( o.william_brewster, out ); out += ',';
  out += "william_penn="; base::to_str( o.william_penn, out ); out += ',';
  out += "jean_de_brebeuf="; base::to_str( o.jean_de_brebeuf, out ); out += ',';
  out += "juan_de_sepulveda="; base::to_str( o.juan_de_sepulveda, out ); out += ',';
  out += "bartolme_de_las_casas="; base::to_str( o.bartolme_de_las_casas, out ); out += ',';
  out += "unknown00="; base::to_str( bits<7>{ o.unknown00 }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, FoundingFathers& o ) {
  uint32_t bits = 0;
  if( !b.read_bytes<4>( bits ) ) return false;
  o.adam_smith = (bits & 0b1); bits >>= 1;
  o.jakob_fugger = (bits & 0b1); bits >>= 1;
  o.peter_minuit = (bits & 0b1); bits >>= 1;
  o.peter_stuyvesant = (bits & 0b1); bits >>= 1;
  o.jan_de_witt = (bits & 0b1); bits >>= 1;
  o.ferdinand_magellan = (bits & 0b1); bits >>= 1;
  o.francisco_coronado = (bits & 0b1); bits >>= 1;
  o.hernando_de_soto = (bits & 0b1); bits >>= 1;
  o.henry_hudson = (bits & 0b1); bits >>= 1;
  o.sieur_de_la_salle = (bits & 0b1); bits >>= 1;
  o.hernan_cortes = (bits & 0b1); bits >>= 1;
  o.george_washington = (bits & 0b1); bits >>= 1;
  o.paul_revere = (bits & 0b1); bits >>= 1;
  o.francis_drake = (bits & 0b1); bits >>= 1;
  o.john_paul_jones = (bits & 0b1); bits >>= 1;
  o.thomas_jefferson = (bits & 0b1); bits >>= 1;
  o.pocahontas = (bits & 0b1); bits >>= 1;
  o.thomas_paine = (bits & 0b1); bits >>= 1;
  o.simon_bolivar = (bits & 0b1); bits >>= 1;
  o.benjamin_franklin = (bits & 0b1); bits >>= 1;
  o.william_brewster = (bits & 0b1); bits >>= 1;
  o.william_penn = (bits & 0b1); bits >>= 1;
  o.jean_de_brebeuf = (bits & 0b1); bits >>= 1;
  o.juan_de_sepulveda = (bits & 0b1); bits >>= 1;
  o.bartolme_de_las_casas = (bits & 0b1); bits >>= 1;
  o.unknown00 = (bits & 0b1111111); bits >>= 7;
  return true;
}

bool write_binary( base::IBinaryIO& b, FoundingFathers const& o ) {
  uint32_t bits = 0;
  bits |= (o.unknown00 & 0b1111111); bits <<= 1;
  bits |= (o.bartolme_de_las_casas & 0b1); bits <<= 1;
  bits |= (o.juan_de_sepulveda & 0b1); bits <<= 1;
  bits |= (o.jean_de_brebeuf & 0b1); bits <<= 1;
  bits |= (o.william_penn & 0b1); bits <<= 1;
  bits |= (o.william_brewster & 0b1); bits <<= 1;
  bits |= (o.benjamin_franklin & 0b1); bits <<= 1;
  bits |= (o.simon_bolivar & 0b1); bits <<= 1;
  bits |= (o.thomas_paine & 0b1); bits <<= 1;
  bits |= (o.pocahontas & 0b1); bits <<= 1;
  bits |= (o.thomas_jefferson & 0b1); bits <<= 1;
  bits |= (o.john_paul_jones & 0b1); bits <<= 1;
  bits |= (o.francis_drake & 0b1); bits <<= 1;
  bits |= (o.paul_revere & 0b1); bits <<= 1;
  bits |= (o.george_washington & 0b1); bits <<= 1;
  bits |= (o.hernan_cortes & 0b1); bits <<= 1;
  bits |= (o.sieur_de_la_salle & 0b1); bits <<= 1;
  bits |= (o.henry_hudson & 0b1); bits <<= 1;
  bits |= (o.hernando_de_soto & 0b1); bits <<= 1;
  bits |= (o.francisco_coronado & 0b1); bits <<= 1;
  bits |= (o.ferdinand_magellan & 0b1); bits <<= 1;
  bits |= (o.jan_de_witt & 0b1); bits <<= 1;
  bits |= (o.peter_stuyvesant & 0b1); bits <<= 1;
  bits |= (o.peter_minuit & 0b1); bits <<= 1;
  bits |= (o.jakob_fugger & 0b1); bits <<= 1;
  bits |= (o.adam_smith & 0b1); bits <<= 0;
  return b.write_bytes<4>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         FoundingFathers const& o,
                         cdr::tag_t<FoundingFathers> ) {
  cdr::table tbl;
  conv.to_field( tbl, "adam_smith", o.adam_smith );
  conv.to_field( tbl, "jakob_fugger", o.jakob_fugger );
  conv.to_field( tbl, "peter_minuit", o.peter_minuit );
  conv.to_field( tbl, "peter_stuyvesant", o.peter_stuyvesant );
  conv.to_field( tbl, "jan_de_witt", o.jan_de_witt );
  conv.to_field( tbl, "ferdinand_magellan", o.ferdinand_magellan );
  conv.to_field( tbl, "francisco_coronado", o.francisco_coronado );
  conv.to_field( tbl, "hernando_de_soto", o.hernando_de_soto );
  conv.to_field( tbl, "henry_hudson", o.henry_hudson );
  conv.to_field( tbl, "sieur_de_la_salle", o.sieur_de_la_salle );
  conv.to_field( tbl, "hernan_cortes", o.hernan_cortes );
  conv.to_field( tbl, "george_washington", o.george_washington );
  conv.to_field( tbl, "paul_revere", o.paul_revere );
  conv.to_field( tbl, "francis_drake", o.francis_drake );
  conv.to_field( tbl, "john_paul_jones", o.john_paul_jones );
  conv.to_field( tbl, "thomas_jefferson", o.thomas_jefferson );
  conv.to_field( tbl, "pocahontas", o.pocahontas );
  conv.to_field( tbl, "thomas_paine", o.thomas_paine );
  conv.to_field( tbl, "simon_bolivar", o.simon_bolivar );
  conv.to_field( tbl, "benjamin_franklin", o.benjamin_franklin );
  conv.to_field( tbl, "william_brewster", o.william_brewster );
  conv.to_field( tbl, "william_penn", o.william_penn );
  conv.to_field( tbl, "jean_de_brebeuf", o.jean_de_brebeuf );
  conv.to_field( tbl, "juan_de_sepulveda", o.juan_de_sepulveda );
  conv.to_field( tbl, "bartolme_de_las_casas", o.bartolme_de_las_casas );
  conv.to_field( tbl, "unknown00", bits<7>{ o.unknown00 } );
  tbl["__key_order"] = cdr::list{
    "adam_smith",
    "jakob_fugger",
    "peter_minuit",
    "peter_stuyvesant",
    "jan_de_witt",
    "ferdinand_magellan",
    "francisco_coronado",
    "hernando_de_soto",
    "henry_hudson",
    "sieur_de_la_salle",
    "hernan_cortes",
    "george_washington",
    "paul_revere",
    "francis_drake",
    "john_paul_jones",
    "thomas_jefferson",
    "pocahontas",
    "thomas_paine",
    "simon_bolivar",
    "benjamin_franklin",
    "william_brewster",
    "william_penn",
    "jean_de_brebeuf",
    "juan_de_sepulveda",
    "bartolme_de_las_casas",
    "unknown00",
  };
  return tbl;
}

cdr::result<FoundingFathers> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<FoundingFathers> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  FoundingFathers res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "adam_smith", adam_smith );
  CONV_FROM_FIELD( "jakob_fugger", jakob_fugger );
  CONV_FROM_FIELD( "peter_minuit", peter_minuit );
  CONV_FROM_FIELD( "peter_stuyvesant", peter_stuyvesant );
  CONV_FROM_FIELD( "jan_de_witt", jan_de_witt );
  CONV_FROM_FIELD( "ferdinand_magellan", ferdinand_magellan );
  CONV_FROM_FIELD( "francisco_coronado", francisco_coronado );
  CONV_FROM_FIELD( "hernando_de_soto", hernando_de_soto );
  CONV_FROM_FIELD( "henry_hudson", henry_hudson );
  CONV_FROM_FIELD( "sieur_de_la_salle", sieur_de_la_salle );
  CONV_FROM_FIELD( "hernan_cortes", hernan_cortes );
  CONV_FROM_FIELD( "george_washington", george_washington );
  CONV_FROM_FIELD( "paul_revere", paul_revere );
  CONV_FROM_FIELD( "francis_drake", francis_drake );
  CONV_FROM_FIELD( "john_paul_jones", john_paul_jones );
  CONV_FROM_FIELD( "thomas_jefferson", thomas_jefferson );
  CONV_FROM_FIELD( "pocahontas", pocahontas );
  CONV_FROM_FIELD( "thomas_paine", thomas_paine );
  CONV_FROM_FIELD( "simon_bolivar", simon_bolivar );
  CONV_FROM_FIELD( "benjamin_franklin", benjamin_franklin );
  CONV_FROM_FIELD( "william_brewster", william_brewster );
  CONV_FROM_FIELD( "william_penn", william_penn );
  CONV_FROM_FIELD( "jean_de_brebeuf", jean_de_brebeuf );
  CONV_FROM_FIELD( "juan_de_sepulveda", juan_de_sepulveda );
  CONV_FROM_FIELD( "bartolme_de_las_casas", bartolme_de_las_casas );
  CONV_FROM_BITSTRING_FIELD( "unknown00", unknown00, 7 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** BoycottBitmap
*****************************************************************/
void to_str( BoycottBitmap const& o, std::string& out, base::tag<BoycottBitmap> ) {
  out += "BoycottBitmap{";
  out += "food="; base::to_str( o.food, out ); out += ',';
  out += "sugar="; base::to_str( o.sugar, out ); out += ',';
  out += "tobacco="; base::to_str( o.tobacco, out ); out += ',';
  out += "cotton="; base::to_str( o.cotton, out ); out += ',';
  out += "furs="; base::to_str( o.furs, out ); out += ',';
  out += "lumber="; base::to_str( o.lumber, out ); out += ',';
  out += "ore="; base::to_str( o.ore, out ); out += ',';
  out += "silver="; base::to_str( o.silver, out ); out += ',';
  out += "horses="; base::to_str( o.horses, out ); out += ',';
  out += "rum="; base::to_str( o.rum, out ); out += ',';
  out += "cigars="; base::to_str( o.cigars, out ); out += ',';
  out += "cloth="; base::to_str( o.cloth, out ); out += ',';
  out += "coats="; base::to_str( o.coats, out ); out += ',';
  out += "trade_goods="; base::to_str( o.trade_goods, out ); out += ',';
  out += "tools="; base::to_str( o.tools, out ); out += ',';
  out += "muskets="; base::to_str( o.muskets, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BoycottBitmap& o ) {
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

bool write_binary( base::IBinaryIO& b, BoycottBitmap const& o ) {
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
** RelationByNations
*****************************************************************/
void to_str( RelationByNations const& o, std::string& out, base::tag<RelationByNations> ) {
  out += "RelationByNations{";
  out += "attitudeq="; base::to_str( o.attitudeq, out ); out += ',';
  out += "status="; base::to_str( o.status, out ); out += ',';
  out += "irritated_by_piracy="; base::to_str( o.irritated_by_piracy, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByNations& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.attitudeq = (bits & 0b1111); bits >>= 4;
  o.status = static_cast<relation_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.irritated_by_piracy = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, RelationByNations const& o ) {
  uint8_t bits = 0;
  bits |= (o.irritated_by_piracy & 0b1); bits <<= 3;
  bits |= (static_cast<uint8_t>( o.status ) & 0b111); bits <<= 4;
  bits |= (o.attitudeq & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         RelationByNations const& o,
                         cdr::tag_t<RelationByNations> ) {
  cdr::table tbl;
  conv.to_field( tbl, "attitude?", o.attitudeq );
  conv.to_field( tbl, "status", o.status );
  conv.to_field( tbl, "irritated_by_piracy", o.irritated_by_piracy );
  tbl["__key_order"] = cdr::list{
    "attitude?",
    "status",
    "irritated_by_piracy",
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
  CONV_FROM_FIELD( "attitude?", attitudeq );
  CONV_FROM_FIELD( "status", status );
  CONV_FROM_FIELD( "irritated_by_piracy", irritated_by_piracy );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** RelationByIndian
*****************************************************************/
void to_str( RelationByIndian const& o, std::string& out, base::tag<RelationByIndian> ) {
  out += "RelationByIndian{";
  out += "attitudeq="; base::to_str( o.attitudeq, out ); out += ',';
  out += "status="; base::to_str( o.status, out ); out += ',';
  out += "unused="; base::to_str( o.unused, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByIndian& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.attitudeq = (bits & 0b1111); bits >>= 4;
  o.status = static_cast<relation_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.unused = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, RelationByIndian const& o ) {
  uint8_t bits = 0;
  bits |= (o.unused & 0b1); bits <<= 3;
  bits |= (static_cast<uint8_t>( o.status ) & 0b111); bits <<= 4;
  bits |= (o.attitudeq & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         RelationByIndian const& o,
                         cdr::tag_t<RelationByIndian> ) {
  cdr::table tbl;
  conv.to_field( tbl, "attitude?", o.attitudeq );
  conv.to_field( tbl, "status", o.status );
  conv.to_field( tbl, "unused", o.unused );
  tbl["__key_order"] = cdr::list{
    "attitude?",
    "status",
    "unused",
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
  CONV_FROM_FIELD( "attitude?", attitudeq );
  CONV_FROM_FIELD( "status", status );
  CONV_FROM_FIELD( "unused", unused );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** BLCS
*****************************************************************/
void to_str( BLCS const& o, std::string& out, base::tag<BLCS> ) {
  out += "BLCS{";
  out += "brave_missing="; base::to_str( o.brave_missing, out ); out += ',';
  out += "learned="; base::to_str( o.learned, out ); out += ',';
  out += "capital="; base::to_str( o.capital, out ); out += ',';
  out += "scouted="; base::to_str( o.scouted, out ); out += ',';
  out += "unused09="; base::to_str( bits<4>{ o.unused09 }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BLCS& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.brave_missing = (bits & 0b1); bits >>= 1;
  o.learned = (bits & 0b1); bits >>= 1;
  o.capital = (bits & 0b1); bits >>= 1;
  o.scouted = (bits & 0b1); bits >>= 1;
  o.unused09 = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, BLCS const& o ) {
  uint8_t bits = 0;
  bits |= (o.unused09 & 0b1111); bits <<= 1;
  bits |= (o.scouted & 0b1); bits <<= 1;
  bits |= (o.capital & 0b1); bits <<= 1;
  bits |= (o.learned & 0b1); bits <<= 1;
  bits |= (o.brave_missing & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         BLCS const& o,
                         cdr::tag_t<BLCS> ) {
  cdr::table tbl;
  conv.to_field( tbl, "brave_missing", o.brave_missing );
  conv.to_field( tbl, "learned", o.learned );
  conv.to_field( tbl, "capital", o.capital );
  conv.to_field( tbl, "scouted", o.scouted );
  conv.to_field( tbl, "unused09", bits<4>{ o.unused09 } );
  tbl["__key_order"] = cdr::list{
    "brave_missing",
    "learned",
    "capital",
    "scouted",
    "unused09",
  };
  return tbl;
}

cdr::result<BLCS> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BLCS> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  BLCS res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "brave_missing", brave_missing );
  CONV_FROM_FIELD( "learned", learned );
  CONV_FROM_FIELD( "capital", capital );
  CONV_FROM_FIELD( "scouted", scouted );
  CONV_FROM_BITSTRING_FIELD( "unused09", unused09, 4 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Mission
*****************************************************************/
void to_str( Mission const& o, std::string& out, base::tag<Mission> ) {
  out += "Mission{";
  out += "nation_id="; base::to_str( o.nation_id, out ); out += ',';
  out += "expert="; base::to_str( o.expert, out ); out += ',';
  out += "unknown="; base::to_str( bits<3>{ o.unknown }, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Mission& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.nation_id = static_cast<nation_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.expert = (bits & 0b1); bits >>= 1;
  o.unknown = (bits & 0b111); bits >>= 3;
  return true;
}

bool write_binary( base::IBinaryIO& b, Mission const& o ) {
  uint8_t bits = 0;
  bits |= (o.unknown & 0b111); bits <<= 1;
  bits |= (o.expert & 0b1); bits <<= 4;
  bits |= (static_cast<uint8_t>( o.nation_id ) & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         Mission const& o,
                         cdr::tag_t<Mission> ) {
  cdr::table tbl;
  conv.to_field( tbl, "nation_id", o.nation_id );
  conv.to_field( tbl, "expert", o.expert );
  conv.to_field( tbl, "unknown", bits<3>{ o.unknown } );
  tbl["__key_order"] = cdr::list{
    "nation_id",
    "expert",
    "unknown",
  };
  return tbl;
}

cdr::result<Mission> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Mission> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Mission res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "nation_id", nation_id );
  CONV_FROM_FIELD( "expert", expert );
  CONV_FROM_BITSTRING_FIELD( "unknown", unknown, 3 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TribeFlags
*****************************************************************/
void to_str( TribeFlags const& o, std::string& out, base::tag<TribeFlags> ) {
  out += "TribeFlags{";
  out += "unknown01="; base::to_str( bits<5>{ o.unknown01 }, out ); out += ',';
  out += "joined_ref="; base::to_str( o.joined_ref, out ); out += ',';
  out += "unknown02="; base::to_str( bits<1>{ o.unknown02 }, out ); out += ',';
  out += "extinct="; base::to_str( o.extinct, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TribeFlags& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unknown01 = (bits & 0b11111); bits >>= 5;
  o.joined_ref = (bits & 0b1); bits >>= 1;
  o.unknown02 = (bits & 0b1); bits >>= 1;
  o.extinct = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, TribeFlags const& o ) {
  uint8_t bits = 0;
  bits |= (o.extinct & 0b1); bits <<= 1;
  bits |= (o.unknown02 & 0b1); bits <<= 1;
  bits |= (o.joined_ref & 0b1); bits <<= 5;
  bits |= (o.unknown01 & 0b11111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         TribeFlags const& o,
                         cdr::tag_t<TribeFlags> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown01", bits<5>{ o.unknown01 } );
  conv.to_field( tbl, "joined_ref", o.joined_ref );
  conv.to_field( tbl, "unknown02", bits<1>{ o.unknown02 } );
  conv.to_field( tbl, "extinct", o.extinct );
  tbl["__key_order"] = cdr::list{
    "unknown01",
    "joined_ref",
    "unknown02",
    "extinct",
  };
  return tbl;
}

cdr::result<TribeFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TribeFlags> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TribeFlags res = {};
  std::set<std::string> used_keys;
  CONV_FROM_BITSTRING_FIELD( "unknown01", unknown01, 5 );
  CONV_FROM_FIELD( "joined_ref", joined_ref );
  CONV_FROM_BITSTRING_FIELD( "unknown02", unknown02, 1 );
  CONV_FROM_FIELD( "extinct", extinct );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** RelationByNations2
*****************************************************************/
void to_str( RelationByNations2 const& o, std::string& out, base::tag<RelationByNations2> ) {
  out += "RelationByNations2{";
  out += "attitudeq="; base::to_str( o.attitudeq, out ); out += ',';
  out += "status="; base::to_str( o.status, out ); out += ',';
  out += "unused="; base::to_str( o.unused, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByNations2& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.attitudeq = (bits & 0b1111); bits >>= 4;
  o.status = static_cast<relation_3bit_type>( bits & 0b111 ); bits >>= 3;
  o.unused = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, RelationByNations2 const& o ) {
  uint8_t bits = 0;
  bits |= (o.unused & 0b1); bits <<= 3;
  bits |= (static_cast<uint8_t>( o.status ) & 0b111); bits <<= 4;
  bits |= (o.attitudeq & 0b1111); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         RelationByNations2 const& o,
                         cdr::tag_t<RelationByNations2> ) {
  cdr::table tbl;
  conv.to_field( tbl, "attitude?", o.attitudeq );
  conv.to_field( tbl, "status", o.status );
  conv.to_field( tbl, "unused", o.unused );
  tbl["__key_order"] = cdr::list{
    "attitude?",
    "status",
    "unused",
  };
  return tbl;
}

cdr::result<RelationByNations2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByNations2> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  RelationByNations2 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "attitude?", attitudeq );
  CONV_FROM_FIELD( "status", status );
  CONV_FROM_FIELD( "unused", unused );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TILE
*****************************************************************/
void to_str( TILE const& o, std::string& out, base::tag<TILE> ) {
  out += "TILE{";
  out += "tile="; base::to_str( o.tile, out ); out += ',';
  out += "hill_river="; base::to_str( o.hill_river, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TILE& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.tile = static_cast<terrain_5bit_type>( bits & 0b11111 ); bits >>= 5;
  o.hill_river = static_cast<hills_river_3bit_type>( bits & 0b111 ); bits >>= 3;
  return true;
}

bool write_binary( base::IBinaryIO& b, TILE const& o ) {
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
void to_str( MASK const& o, std::string& out, base::tag<MASK> ) {
  out += "MASK{";
  out += "has_unit="; base::to_str( o.has_unit, out ); out += ',';
  out += "has_city="; base::to_str( o.has_city, out ); out += ',';
  out += "suppress="; base::to_str( o.suppress, out ); out += ',';
  out += "road="; base::to_str( o.road, out ); out += ',';
  out += "purchased="; base::to_str( o.purchased, out ); out += ',';
  out += "pacific="; base::to_str( o.pacific, out ); out += ',';
  out += "plowed="; base::to_str( o.plowed, out ); out += ',';
  out += "unused="; base::to_str( o.unused, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, MASK& o ) {
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

bool write_binary( base::IBinaryIO& b, MASK const& o ) {
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
void to_str( PATH const& o, std::string& out, base::tag<PATH> ) {
  out += "PATH{";
  out += "region_id="; base::to_str( o.region_id, out ); out += ',';
  out += "visitor_nation="; base::to_str( o.visitor_nation, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PATH& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.region_id = static_cast<region_id_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.visitor_nation = static_cast<nation_4bit_short_type>( bits & 0b1111 ); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, PATH const& o ) {
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
void to_str( SEEN const& o, std::string& out, base::tag<SEEN> ) {
  out += "SEEN{";
  out += "score="; base::to_str( o.score, out ); out += ',';
  out += "vis2en="; base::to_str( o.vis2en, out ); out += ',';
  out += "vis2fr="; base::to_str( o.vis2fr, out ); out += ',';
  out += "vis2sp="; base::to_str( o.vis2sp, out ); out += ',';
  out += "vis2du="; base::to_str( o.vis2du, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, SEEN& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.score = static_cast<region_id_4bit_type>( bits & 0b1111 ); bits >>= 4;
  o.vis2en = static_cast<visible_to_english_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2fr = static_cast<visible_to_french_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2sp = static_cast<visible_to_spanish_1bit_type>( bits & 0b1 ); bits >>= 1;
  o.vis2du = static_cast<visible_to_dutch_1bit_type>( bits & 0b1 ); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, SEEN const& o ) {
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
** SeaLaneConnectivity
*****************************************************************/
void to_str( SeaLaneConnectivity const& o, std::string& out, base::tag<SeaLaneConnectivity> ) {
  out += "SeaLaneConnectivity{";
  out += "north="; base::to_str( o.north, out ); out += ',';
  out += "neast="; base::to_str( o.neast, out ); out += ',';
  out += "east="; base::to_str( o.east, out ); out += ',';
  out += "seast="; base::to_str( o.seast, out ); out += ',';
  out += "south="; base::to_str( o.south, out ); out += ',';
  out += "swest="; base::to_str( o.swest, out ); out += ',';
  out += "west="; base::to_str( o.west, out ); out += ',';
  out += "nwest="; base::to_str( o.nwest, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, SeaLaneConnectivity& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.north = (bits & 0b1); bits >>= 1;
  o.neast = (bits & 0b1); bits >>= 1;
  o.east = (bits & 0b1); bits >>= 1;
  o.seast = (bits & 0b1); bits >>= 1;
  o.south = (bits & 0b1); bits >>= 1;
  o.swest = (bits & 0b1); bits >>= 1;
  o.west = (bits & 0b1); bits >>= 1;
  o.nwest = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, SeaLaneConnectivity const& o ) {
  uint8_t bits = 0;
  bits |= (o.nwest & 0b1); bits <<= 1;
  bits |= (o.west & 0b1); bits <<= 1;
  bits |= (o.swest & 0b1); bits <<= 1;
  bits |= (o.south & 0b1); bits <<= 1;
  bits |= (o.seast & 0b1); bits <<= 1;
  bits |= (o.east & 0b1); bits <<= 1;
  bits |= (o.neast & 0b1); bits <<= 1;
  bits |= (o.north & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         SeaLaneConnectivity const& o,
                         cdr::tag_t<SeaLaneConnectivity> ) {
  cdr::table tbl;
  conv.to_field( tbl, "north", o.north );
  conv.to_field( tbl, "neast", o.neast );
  conv.to_field( tbl, "east", o.east );
  conv.to_field( tbl, "seast", o.seast );
  conv.to_field( tbl, "south", o.south );
  conv.to_field( tbl, "swest", o.swest );
  conv.to_field( tbl, "west", o.west );
  conv.to_field( tbl, "nwest", o.nwest );
  tbl["__key_order"] = cdr::list{
    "north",
    "neast",
    "east",
    "seast",
    "south",
    "swest",
    "west",
    "nwest",
  };
  return tbl;
}

cdr::result<SeaLaneConnectivity> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<SeaLaneConnectivity> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  SeaLaneConnectivity res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "north", north );
  CONV_FROM_FIELD( "neast", neast );
  CONV_FROM_FIELD( "east", east );
  CONV_FROM_FIELD( "seast", seast );
  CONV_FROM_FIELD( "south", south );
  CONV_FROM_FIELD( "swest", swest );
  CONV_FROM_FIELD( "west", west );
  CONV_FROM_FIELD( "nwest", nwest );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** LandConnectivity
*****************************************************************/
void to_str( LandConnectivity const& o, std::string& out, base::tag<LandConnectivity> ) {
  out += "LandConnectivity{";
  out += "north="; base::to_str( o.north, out ); out += ',';
  out += "neast="; base::to_str( o.neast, out ); out += ',';
  out += "east="; base::to_str( o.east, out ); out += ',';
  out += "seast="; base::to_str( o.seast, out ); out += ',';
  out += "south="; base::to_str( o.south, out ); out += ',';
  out += "swest="; base::to_str( o.swest, out ); out += ',';
  out += "west="; base::to_str( o.west, out ); out += ',';
  out += "nwest="; base::to_str( o.nwest, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, LandConnectivity& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.north = (bits & 0b1); bits >>= 1;
  o.neast = (bits & 0b1); bits >>= 1;
  o.east = (bits & 0b1); bits >>= 1;
  o.seast = (bits & 0b1); bits >>= 1;
  o.south = (bits & 0b1); bits >>= 1;
  o.swest = (bits & 0b1); bits >>= 1;
  o.west = (bits & 0b1); bits >>= 1;
  o.nwest = (bits & 0b1); bits >>= 1;
  return true;
}

bool write_binary( base::IBinaryIO& b, LandConnectivity const& o ) {
  uint8_t bits = 0;
  bits |= (o.nwest & 0b1); bits <<= 1;
  bits |= (o.west & 0b1); bits <<= 1;
  bits |= (o.swest & 0b1); bits <<= 1;
  bits |= (o.south & 0b1); bits <<= 1;
  bits |= (o.seast & 0b1); bits <<= 1;
  bits |= (o.east & 0b1); bits <<= 1;
  bits |= (o.neast & 0b1); bits <<= 1;
  bits |= (o.north & 0b1); bits <<= 0;
  return b.write_bytes<1>( bits );
}

cdr::value to_canonical( cdr::converter& conv,
                         LandConnectivity const& o,
                         cdr::tag_t<LandConnectivity> ) {
  cdr::table tbl;
  conv.to_field( tbl, "north", o.north );
  conv.to_field( tbl, "neast", o.neast );
  conv.to_field( tbl, "east", o.east );
  conv.to_field( tbl, "seast", o.seast );
  conv.to_field( tbl, "south", o.south );
  conv.to_field( tbl, "swest", o.swest );
  conv.to_field( tbl, "west", o.west );
  conv.to_field( tbl, "nwest", o.nwest );
  tbl["__key_order"] = cdr::list{
    "north",
    "neast",
    "east",
    "seast",
    "south",
    "swest",
    "west",
    "nwest",
  };
  return tbl;
}

cdr::result<LandConnectivity> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<LandConnectivity> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  LandConnectivity res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "north", north );
  CONV_FROM_FIELD( "neast", neast );
  CONV_FROM_FIELD( "east", east );
  CONV_FROM_FIELD( "seast", seast );
  CONV_FROM_FIELD( "south", south );
  CONV_FROM_FIELD( "swest", swest );
  CONV_FROM_FIELD( "west", west );
  CONV_FROM_FIELD( "nwest", nwest );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
void to_str( Stop1LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop1LoadsAndUnloadsCount> ) {
  out += "Stop1LoadsAndUnloadsCount{";
  out += "unloads_count="; base::to_str( o.unloads_count, out ); out += ',';
  out += "loads_count="; base::to_str( o.loads_count, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, Stop1LoadsAndUnloadsCount const& o ) {
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
void to_str( Stop1LoadsCargo const& o, std::string& out, base::tag<Stop1LoadsCargo> ) {
  out += "Stop1LoadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1LoadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop1LoadsCargo const& o ) {
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
void to_str( Stop1UnloadsCargo const& o, std::string& out, base::tag<Stop1UnloadsCargo> ) {
  out += "Stop1UnloadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1UnloadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop1UnloadsCargo const& o ) {
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
void to_str( Stop2LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop2LoadsAndUnloadsCount> ) {
  out += "Stop2LoadsAndUnloadsCount{";
  out += "unloads_count="; base::to_str( o.unloads_count, out ); out += ',';
  out += "loads_count="; base::to_str( o.loads_count, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, Stop2LoadsAndUnloadsCount const& o ) {
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
void to_str( Stop2LoadsCargo const& o, std::string& out, base::tag<Stop2LoadsCargo> ) {
  out += "Stop2LoadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2LoadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop2LoadsCargo const& o ) {
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
void to_str( Stop2UnloadsCargo const& o, std::string& out, base::tag<Stop2UnloadsCargo> ) {
  out += "Stop2UnloadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2UnloadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop2UnloadsCargo const& o ) {
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
void to_str( Stop3LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop3LoadsAndUnloadsCount> ) {
  out += "Stop3LoadsAndUnloadsCount{";
  out += "unloads_count="; base::to_str( o.unloads_count, out ); out += ',';
  out += "loads_count="; base::to_str( o.loads_count, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, Stop3LoadsAndUnloadsCount const& o ) {
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
void to_str( Stop3LoadsCargo const& o, std::string& out, base::tag<Stop3LoadsCargo> ) {
  out += "Stop3LoadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3LoadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop3LoadsCargo const& o ) {
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
void to_str( Stop3UnloadsCargo const& o, std::string& out, base::tag<Stop3UnloadsCargo> ) {
  out += "Stop3UnloadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3UnloadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop3UnloadsCargo const& o ) {
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
void to_str( Stop4LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop4LoadsAndUnloadsCount> ) {
  out += "Stop4LoadsAndUnloadsCount{";
  out += "unloads_count="; base::to_str( o.unloads_count, out ); out += ',';
  out += "loads_count="; base::to_str( o.loads_count, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4LoadsAndUnloadsCount& o ) {
  uint8_t bits = 0;
  if( !b.read_bytes<1>( bits ) ) return false;
  o.unloads_count = (bits & 0b1111); bits >>= 4;
  o.loads_count = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::IBinaryIO& b, Stop4LoadsAndUnloadsCount const& o ) {
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
void to_str( Stop4LoadsCargo const& o, std::string& out, base::tag<Stop4LoadsCargo> ) {
  out += "Stop4LoadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4LoadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop4LoadsCargo const& o ) {
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
void to_str( Stop4UnloadsCargo const& o, std::string& out, base::tag<Stop4UnloadsCargo> ) {
  out += "Stop4UnloadsCargo{";
  out += "cargo_1="; base::to_str( o.cargo_1, out ); out += ',';
  out += "cargo_2="; base::to_str( o.cargo_2, out ); out += ',';
  out += "cargo_3="; base::to_str( o.cargo_3, out ); out += ',';
  out += "cargo_4="; base::to_str( o.cargo_4, out ); out += ',';
  out += "cargo_5="; base::to_str( o.cargo_5, out ); out += ',';
  out += "cargo_6="; base::to_str( o.cargo_6, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4UnloadsCargo& o ) {
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

bool write_binary( base::IBinaryIO& b, Stop4UnloadsCargo const& o ) {
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
void to_str( ExpeditionaryForce const& o, std::string& out, base::tag<ExpeditionaryForce> ) {
  out += "ExpeditionaryForce{";
  out += "regulars="; base::to_str( o.regulars, out ); out += ',';
  out += "dragoons="; base::to_str( o.dragoons, out ); out += ',';
  out += "man_o_wars="; base::to_str( o.man_o_wars, out ); out += ',';
  out += "artillery="; base::to_str( o.artillery, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ExpeditionaryForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::IBinaryIO& b, ExpeditionaryForce const& o ) {
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
void to_str( BackupForce const& o, std::string& out, base::tag<BackupForce> ) {
  out += "BackupForce{";
  out += "regulars="; base::to_str( o.regulars, out ); out += ',';
  out += "dragoons="; base::to_str( o.dragoons, out ); out += ',';
  out += "man_o_wars="; base::to_str( o.man_o_wars, out ); out += ',';
  out += "artillery="; base::to_str( o.artillery, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BackupForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::IBinaryIO& b, BackupForce const& o ) {
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
void to_str( PriceGroupState const& o, std::string& out, base::tag<PriceGroupState> ) {
  out += "PriceGroupState{";
  out += "unused1="; base::to_str( o.unused1, out ); out += ',';
  out += "rum="; base::to_str( o.rum, out ); out += ',';
  out += "cigars="; base::to_str( o.cigars, out ); out += ',';
  out += "cloth="; base::to_str( o.cloth, out ); out += ',';
  out += "coats="; base::to_str( o.coats, out ); out += ',';
  out += "unused2="; base::to_str( o.unused2, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PriceGroupState& o ) {
  return true
    && read_binary( b, o.unused1 )
    && read_binary( b, o.rum )
    && read_binary( b, o.cigars )
    && read_binary( b, o.cloth )
    && read_binary( b, o.coats )
    && read_binary( b, o.unused2 )
    ;
}

bool write_binary( base::IBinaryIO& b, PriceGroupState const& o ) {
  return true
    && write_binary( b, o.unused1 )
    && write_binary( b, o.rum )
    && write_binary( b, o.cigars )
    && write_binary( b, o.cloth )
    && write_binary( b, o.coats )
    && write_binary( b, o.unused2 )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         PriceGroupState const& o,
                         cdr::tag_t<PriceGroupState> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unused1", o.unused1 );
  conv.to_field( tbl, "rum", o.rum );
  conv.to_field( tbl, "cigars", o.cigars );
  conv.to_field( tbl, "cloth", o.cloth );
  conv.to_field( tbl, "coats", o.coats );
  conv.to_field( tbl, "unused2", o.unused2 );
  tbl["__key_order"] = cdr::list{
    "unused1",
    "rum",
    "cigars",
    "cloth",
    "coats",
    "unused2",
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
  CONV_FROM_FIELD( "unused1", unused1 );
  CONV_FROM_FIELD( "rum", rum );
  CONV_FROM_FIELD( "cigars", cigars );
  CONV_FROM_FIELD( "cloth", cloth );
  CONV_FROM_FIELD( "coats", coats );
  CONV_FROM_FIELD( "unused2", unused2 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** HEADER
*****************************************************************/
void to_str( HEADER const& o, std::string& out, base::tag<HEADER> ) {
  out += "HEADER{";
  out += "colonize="; base::to_str( o.colonize, out ); out += ',';
  out += "unknown00="; base::to_str( o.unknown00, out ); out += ',';
  out += "map_size_x="; base::to_str( o.map_size_x, out ); out += ',';
  out += "map_size_y="; base::to_str( o.map_size_y, out ); out += ',';
  out += "tutorial_help="; base::to_str( o.tutorial_help, out ); out += ',';
  out += "unknown03="; base::to_str( o.unknown03, out ); out += ',';
  out += "game_flags_1="; base::to_str( o.game_flags_1, out ); out += ',';
  out += "colony_report_options_to_disable="; base::to_str( o.colony_report_options_to_disable, out ); out += ',';
  out += "game_flags_2="; base::to_str( o.game_flags_2, out ); out += ',';
  out += "unknown39="; base::to_str( o.unknown39, out ); out += ',';
  out += "year="; base::to_str( o.year, out ); out += ',';
  out += "season="; base::to_str( o.season, out ); out += ',';
  out += "turn="; base::to_str( o.turn, out ); out += ',';
  out += "tile_selection_mode="; base::to_str( o.tile_selection_mode, out ); out += ',';
  out += "unknown40="; base::to_str( o.unknown40, out ); out += ',';
  out += "active_unit="; base::to_str( o.active_unit, out ); out += ',';
  out += "nation_turn="; base::to_str( o.nation_turn, out ); out += ',';
  out += "curr_nation_map_view="; base::to_str( o.curr_nation_map_view, out ); out += ',';
  out += "human_player="; base::to_str( o.human_player, out ); out += ',';
  out += "dwelling_count="; base::to_str( o.dwelling_count, out ); out += ',';
  out += "unit_count="; base::to_str( o.unit_count, out ); out += ',';
  out += "colony_count="; base::to_str( o.colony_count, out ); out += ',';
  out += "trade_route_count="; base::to_str( o.trade_route_count, out ); out += ',';
  out += "show_entire_map="; base::to_str( o.show_entire_map, out ); out += ',';
  out += "fixed_nation_map_view="; base::to_str( o.fixed_nation_map_view, out ); out += ',';
  out += "difficulty="; base::to_str( o.difficulty, out ); out += ',';
  out += "unknown43a="; base::to_str( o.unknown43a, out ); out += ',';
  out += "unknown43b="; base::to_str( o.unknown43b, out ); out += ',';
  out += "founding_father="; base::to_str( o.founding_father, out ); out += ',';
  out += "unknown44aa="; base::to_str( o.unknown44aa, out ); out += ',';
  out += "manual_save_flag="; base::to_str( o.manual_save_flag, out ); out += ',';
  out += "unknown44ab="; base::to_str( o.unknown44ab, out ); out += ',';
  out += "end_of_turn_sign="; base::to_str( o.end_of_turn_sign, out ); out += ',';
  out += "nation_relation="; base::to_str( o.nation_relation, out ); out += ',';
  out += "rebel_sentiment_report="; base::to_str( o.rebel_sentiment_report, out ); out += ',';
  out += "unknown45a="; base::to_str( o.unknown45a, out ); out += ',';
  out += "last_reported_rebel_sentiment="; base::to_str( o.last_reported_rebel_sentiment, out ); out += ',';
  out += "expeditionary_force="; base::to_str( o.expeditionary_force, out ); out += ',';
  out += "backup_force="; base::to_str( o.backup_force, out ); out += ',';
  out += "price_group_state="; base::to_str( o.price_group_state, out ); out += ',';
  out += "event="; base::to_str( o.event, out ); out += ',';
  out += "unknown05="; base::to_str( o.unknown05, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, HEADER& o ) {
  return true
    && read_binary( b, o.colonize )
    && read_binary( b, o.unknown00 )
    && read_binary( b, o.map_size_x )
    && read_binary( b, o.map_size_y )
    && read_binary( b, o.tutorial_help )
    && read_binary( b, o.unknown03 )
    && read_binary( b, o.game_flags_1 )
    && read_binary( b, o.colony_report_options_to_disable )
    && read_binary( b, o.game_flags_2 )
    && read_binary( b, o.unknown39 )
    && read_binary( b, o.year )
    && read_binary( b, o.season )
    && read_binary( b, o.turn )
    && read_binary( b, o.tile_selection_mode )
    && read_binary( b, o.unknown40 )
    && read_binary( b, o.active_unit )
    && read_binary( b, o.nation_turn )
    && read_binary( b, o.curr_nation_map_view )
    && read_binary( b, o.human_player )
    && read_binary( b, o.dwelling_count )
    && read_binary( b, o.unit_count )
    && read_binary( b, o.colony_count )
    && read_binary( b, o.trade_route_count )
    && read_binary( b, o.show_entire_map )
    && read_binary( b, o.fixed_nation_map_view )
    && read_binary( b, o.difficulty )
    && read_binary( b, o.unknown43a )
    && read_binary( b, o.unknown43b )
    && read_binary( b, o.founding_father )
    && read_binary( b, o.unknown44aa )
    && read_binary( b, o.manual_save_flag )
    && read_binary( b, o.unknown44ab )
    && read_binary( b, o.end_of_turn_sign )
    && read_binary( b, o.nation_relation )
    && read_binary( b, o.rebel_sentiment_report )
    && read_binary( b, o.unknown45a )
    && read_binary( b, o.last_reported_rebel_sentiment )
    && read_binary( b, o.expeditionary_force )
    && read_binary( b, o.backup_force )
    && read_binary( b, o.price_group_state )
    && read_binary( b, o.event )
    && read_binary( b, o.unknown05 )
    ;
}

bool write_binary( base::IBinaryIO& b, HEADER const& o ) {
  return true
    && write_binary( b, o.colonize )
    && write_binary( b, o.unknown00 )
    && write_binary( b, o.map_size_x )
    && write_binary( b, o.map_size_y )
    && write_binary( b, o.tutorial_help )
    && write_binary( b, o.unknown03 )
    && write_binary( b, o.game_flags_1 )
    && write_binary( b, o.colony_report_options_to_disable )
    && write_binary( b, o.game_flags_2 )
    && write_binary( b, o.unknown39 )
    && write_binary( b, o.year )
    && write_binary( b, o.season )
    && write_binary( b, o.turn )
    && write_binary( b, o.tile_selection_mode )
    && write_binary( b, o.unknown40 )
    && write_binary( b, o.active_unit )
    && write_binary( b, o.nation_turn )
    && write_binary( b, o.curr_nation_map_view )
    && write_binary( b, o.human_player )
    && write_binary( b, o.dwelling_count )
    && write_binary( b, o.unit_count )
    && write_binary( b, o.colony_count )
    && write_binary( b, o.trade_route_count )
    && write_binary( b, o.show_entire_map )
    && write_binary( b, o.fixed_nation_map_view )
    && write_binary( b, o.difficulty )
    && write_binary( b, o.unknown43a )
    && write_binary( b, o.unknown43b )
    && write_binary( b, o.founding_father )
    && write_binary( b, o.unknown44aa )
    && write_binary( b, o.manual_save_flag )
    && write_binary( b, o.unknown44ab )
    && write_binary( b, o.end_of_turn_sign )
    && write_binary( b, o.nation_relation )
    && write_binary( b, o.rebel_sentiment_report )
    && write_binary( b, o.unknown45a )
    && write_binary( b, o.last_reported_rebel_sentiment )
    && write_binary( b, o.expeditionary_force )
    && write_binary( b, o.backup_force )
    && write_binary( b, o.price_group_state )
    && write_binary( b, o.event )
    && write_binary( b, o.unknown05 )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         HEADER const& o,
                         cdr::tag_t<HEADER> ) {
  cdr::table tbl;
  conv.to_field( tbl, "colonize", o.colonize );
  conv.to_field( tbl, "unknown00", o.unknown00 );
  conv.to_field( tbl, "map_size_x", o.map_size_x );
  conv.to_field( tbl, "map_size_y", o.map_size_y );
  conv.to_field( tbl, "tutorial_help", o.tutorial_help );
  conv.to_field( tbl, "unknown03", o.unknown03 );
  conv.to_field( tbl, "game_flags_1", o.game_flags_1 );
  conv.to_field( tbl, "colony_report_options_to_disable", o.colony_report_options_to_disable );
  conv.to_field( tbl, "game_flags_2", o.game_flags_2 );
  conv.to_field( tbl, "unknown39", o.unknown39 );
  conv.to_field( tbl, "year", o.year );
  conv.to_field( tbl, "season", o.season );
  conv.to_field( tbl, "turn", o.turn );
  conv.to_field( tbl, "tile_selection_mode", o.tile_selection_mode );
  conv.to_field( tbl, "unknown40", o.unknown40 );
  conv.to_field( tbl, "active_unit", o.active_unit );
  conv.to_field( tbl, "nation_turn", o.nation_turn );
  conv.to_field( tbl, "curr_nation_map_view", o.curr_nation_map_view );
  conv.to_field( tbl, "human_player", o.human_player );
  conv.to_field( tbl, "dwelling_count", o.dwelling_count );
  conv.to_field( tbl, "unit_count", o.unit_count );
  conv.to_field( tbl, "colony_count", o.colony_count );
  conv.to_field( tbl, "trade_route_count", o.trade_route_count );
  conv.to_field( tbl, "show_entire_map", o.show_entire_map );
  conv.to_field( tbl, "fixed_nation_map_view", o.fixed_nation_map_view );
  conv.to_field( tbl, "difficulty", o.difficulty );
  conv.to_field( tbl, "unknown43a", o.unknown43a );
  conv.to_field( tbl, "unknown43b", o.unknown43b );
  conv.to_field( tbl, "founding_father", o.founding_father );
  conv.to_field( tbl, "unknown44aa", o.unknown44aa );
  conv.to_field( tbl, "manual_save_flag", o.manual_save_flag );
  conv.to_field( tbl, "unknown44ab", o.unknown44ab );
  conv.to_field( tbl, "end_of_turn_sign", o.end_of_turn_sign );
  conv.to_field( tbl, "nation_relation", o.nation_relation );
  conv.to_field( tbl, "rebel_sentiment_report", o.rebel_sentiment_report );
  conv.to_field( tbl, "unknown45a", o.unknown45a );
  conv.to_field( tbl, "last_reported_rebel_sentiment", o.last_reported_rebel_sentiment );
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
    "tutorial_help",
    "unknown03",
    "game_flags_1",
    "colony_report_options_to_disable",
    "game_flags_2",
    "unknown39",
    "year",
    "season",
    "turn",
    "tile_selection_mode",
    "unknown40",
    "active_unit",
    "nation_turn",
    "curr_nation_map_view",
    "human_player",
    "dwelling_count",
    "unit_count",
    "colony_count",
    "trade_route_count",
    "show_entire_map",
    "fixed_nation_map_view",
    "difficulty",
    "unknown43a",
    "unknown43b",
    "founding_father",
    "unknown44aa",
    "manual_save_flag",
    "unknown44ab",
    "end_of_turn_sign",
    "nation_relation",
    "rebel_sentiment_report",
    "unknown45a",
    "last_reported_rebel_sentiment",
    "expeditionary_force",
    "backup_force",
    "price_group_state",
    "event",
    "unknown05",
  };
  return tbl;
}

cdr::result<HEADER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<HEADER> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  HEADER res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "colonize", colonize );
  CONV_FROM_FIELD( "unknown00", unknown00 );
  CONV_FROM_FIELD( "map_size_x", map_size_x );
  CONV_FROM_FIELD( "map_size_y", map_size_y );
  CONV_FROM_FIELD( "tutorial_help", tutorial_help );
  CONV_FROM_FIELD( "unknown03", unknown03 );
  CONV_FROM_FIELD( "game_flags_1", game_flags_1 );
  CONV_FROM_FIELD( "colony_report_options_to_disable", colony_report_options_to_disable );
  CONV_FROM_FIELD( "game_flags_2", game_flags_2 );
  CONV_FROM_FIELD( "unknown39", unknown39 );
  CONV_FROM_FIELD( "year", year );
  CONV_FROM_FIELD( "season", season );
  CONV_FROM_FIELD( "turn", turn );
  CONV_FROM_FIELD( "tile_selection_mode", tile_selection_mode );
  CONV_FROM_FIELD( "unknown40", unknown40 );
  CONV_FROM_FIELD( "active_unit", active_unit );
  CONV_FROM_FIELD( "nation_turn", nation_turn );
  CONV_FROM_FIELD( "curr_nation_map_view", curr_nation_map_view );
  CONV_FROM_FIELD( "human_player", human_player );
  CONV_FROM_FIELD( "dwelling_count", dwelling_count );
  CONV_FROM_FIELD( "unit_count", unit_count );
  CONV_FROM_FIELD( "colony_count", colony_count );
  CONV_FROM_FIELD( "trade_route_count", trade_route_count );
  CONV_FROM_FIELD( "show_entire_map", show_entire_map );
  CONV_FROM_FIELD( "fixed_nation_map_view", fixed_nation_map_view );
  CONV_FROM_FIELD( "difficulty", difficulty );
  CONV_FROM_FIELD( "unknown43a", unknown43a );
  CONV_FROM_FIELD( "unknown43b", unknown43b );
  CONV_FROM_FIELD( "founding_father", founding_father );
  CONV_FROM_FIELD( "unknown44aa", unknown44aa );
  CONV_FROM_FIELD( "manual_save_flag", manual_save_flag );
  CONV_FROM_FIELD( "unknown44ab", unknown44ab );
  CONV_FROM_FIELD( "end_of_turn_sign", end_of_turn_sign );
  CONV_FROM_FIELD( "nation_relation", nation_relation );
  CONV_FROM_FIELD( "rebel_sentiment_report", rebel_sentiment_report );
  CONV_FROM_FIELD( "unknown45a", unknown45a );
  CONV_FROM_FIELD( "last_reported_rebel_sentiment", last_reported_rebel_sentiment );
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
void to_str( PLAYER const& o, std::string& out, base::tag<PLAYER> ) {
  out += "PLAYER{";
  out += "name="; base::to_str( o.name, out ); out += ',';
  out += "country_name="; base::to_str( o.country_name, out ); out += ',';
  out += "player_flags="; base::to_str( o.player_flags, out ); out += ',';
  out += "control="; base::to_str( o.control, out ); out += ',';
  out += "founded_colonies="; base::to_str( o.founded_colonies, out ); out += ',';
  out += "diplomacy="; base::to_str( o.diplomacy, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PLAYER& o ) {
  return true
    && read_binary( b, o.name )
    && read_binary( b, o.country_name )
    && read_binary( b, o.player_flags )
    && read_binary( b, o.control )
    && read_binary( b, o.founded_colonies )
    && read_binary( b, o.diplomacy )
    ;
}

bool write_binary( base::IBinaryIO& b, PLAYER const& o ) {
  return true
    && write_binary( b, o.name )
    && write_binary( b, o.country_name )
    && write_binary( b, o.player_flags )
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
  conv.to_field( tbl, "player_flags", o.player_flags );
  conv.to_field( tbl, "control", o.control );
  conv.to_field( tbl, "founded_colonies", o.founded_colonies );
  conv.to_field( tbl, "diplomacy", o.diplomacy );
  tbl["__key_order"] = cdr::list{
    "name",
    "country_name",
    "player_flags",
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
  CONV_FROM_FIELD( "player_flags", player_flags );
  CONV_FROM_FIELD( "control", control );
  CONV_FROM_FIELD( "founded_colonies", founded_colonies );
  CONV_FROM_FIELD( "diplomacy", diplomacy );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** OTHER
*****************************************************************/
void to_str( OTHER const& o, std::string& out, base::tag<OTHER> ) {
  out += "OTHER{";
  out += "unknown51a="; base::to_str( o.unknown51a, out ); out += ',';
  out += "click_before_open_colony_x_y="; base::to_str( o.click_before_open_colony_x_y, out ); out += ',';
  out += "unknown51b="; base::to_str( o.unknown51b, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, OTHER& o ) {
  return true
    && read_binary( b, o.unknown51a )
    && read_binary( b, o.click_before_open_colony_x_y )
    && read_binary( b, o.unknown51b )
    ;
}

bool write_binary( base::IBinaryIO& b, OTHER const& o ) {
  return true
    && write_binary( b, o.unknown51a )
    && write_binary( b, o.click_before_open_colony_x_y )
    && write_binary( b, o.unknown51b )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         OTHER const& o,
                         cdr::tag_t<OTHER> ) {
  cdr::table tbl;
  conv.to_field( tbl, "unknown51a", o.unknown51a );
  conv.to_field( tbl, "click_before_open_colony x, y", o.click_before_open_colony_x_y );
  conv.to_field( tbl, "unknown51b", o.unknown51b );
  tbl["__key_order"] = cdr::list{
    "unknown51a",
    "click_before_open_colony x, y",
    "unknown51b",
  };
  return tbl;
}

cdr::result<OTHER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<OTHER> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  OTHER res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "unknown51a", unknown51a );
  CONV_FROM_FIELD( "click_before_open_colony x, y", click_before_open_colony_x_y );
  CONV_FROM_FIELD( "unknown51b", unknown51b );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Tiles
*****************************************************************/
void to_str( Tiles const& o, std::string& out, base::tag<Tiles> ) {
  out += "Tiles{";
  out += "tile_n="; base::to_str( o.tile_n, out ); out += ',';
  out += "tile_e="; base::to_str( o.tile_e, out ); out += ',';
  out += "tile_s="; base::to_str( o.tile_s, out ); out += ',';
  out += "tile_w="; base::to_str( o.tile_w, out ); out += ',';
  out += "tile_nw="; base::to_str( o.tile_nw, out ); out += ',';
  out += "tile_ne="; base::to_str( o.tile_ne, out ); out += ',';
  out += "tile_se="; base::to_str( o.tile_se, out ); out += ',';
  out += "tile_sw="; base::to_str( o.tile_sw, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Tiles& o ) {
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

bool write_binary( base::IBinaryIO& b, Tiles const& o ) {
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
void to_str( Stock const& o, std::string& out, base::tag<Stock> ) {
  out += "Stock{";
  out += "food="; base::to_str( o.food, out ); out += ',';
  out += "sugar="; base::to_str( o.sugar, out ); out += ',';
  out += "tobacco="; base::to_str( o.tobacco, out ); out += ',';
  out += "cotton="; base::to_str( o.cotton, out ); out += ',';
  out += "furs="; base::to_str( o.furs, out ); out += ',';
  out += "lumber="; base::to_str( o.lumber, out ); out += ',';
  out += "ore="; base::to_str( o.ore, out ); out += ',';
  out += "silver="; base::to_str( o.silver, out ); out += ',';
  out += "horses="; base::to_str( o.horses, out ); out += ',';
  out += "rum="; base::to_str( o.rum, out ); out += ',';
  out += "cigars="; base::to_str( o.cigars, out ); out += ',';
  out += "cloth="; base::to_str( o.cloth, out ); out += ',';
  out += "coats="; base::to_str( o.coats, out ); out += ',';
  out += "trade_goods="; base::to_str( o.trade_goods, out ); out += ',';
  out += "tools="; base::to_str( o.tools, out ); out += ',';
  out += "muskets="; base::to_str( o.muskets, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stock& o ) {
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

bool write_binary( base::IBinaryIO& b, Stock const& o ) {
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
void to_str( PopulationOnMap const& o, std::string& out, base::tag<PopulationOnMap> ) {
  out += "PopulationOnMap{";
  out += "for_english="; base::to_str( o.for_english, out ); out += ',';
  out += "for_french="; base::to_str( o.for_french, out ); out += ',';
  out += "for_spanish="; base::to_str( o.for_spanish, out ); out += ',';
  out += "for_dutch="; base::to_str( o.for_dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PopulationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, PopulationOnMap const& o ) {
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
void to_str( FortificationOnMap const& o, std::string& out, base::tag<FortificationOnMap> ) {
  out += "FortificationOnMap{";
  out += "for_english="; base::to_str( o.for_english, out ); out += ',';
  out += "for_french="; base::to_str( o.for_french, out ); out += ',';
  out += "for_spanish="; base::to_str( o.for_spanish, out ); out += ',';
  out += "for_dutch="; base::to_str( o.for_dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, FortificationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, FortificationOnMap const& o ) {
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
void to_str( COLONY const& o, std::string& out, base::tag<COLONY> ) {
  out += "COLONY{";
  out += "x_y="; base::to_str( o.x_y, out ); out += ',';
  out += "name="; base::to_str( o.name, out ); out += ',';
  out += "nation_id="; base::to_str( o.nation_id, out ); out += ',';
  out += "unknown08a="; base::to_str( o.unknown08a, out ); out += ',';
  out += "colony_flags="; base::to_str( o.colony_flags, out ); out += ',';
  out += "unknown08b="; base::to_str( o.unknown08b, out ); out += ',';
  out += "population="; base::to_str( o.population, out ); out += ',';
  out += "occupation="; base::to_str( o.occupation, out ); out += ',';
  out += "profession="; base::to_str( o.profession, out ); out += ',';
  out += "duration="; base::to_str( o.duration, out ); out += ',';
  out += "tiles="; base::to_str( o.tiles, out ); out += ',';
  out += "unknown10="; base::to_str( o.unknown10, out ); out += ',';
  out += "buildings="; base::to_str( o.buildings, out ); out += ',';
  out += "custom_house_flags="; base::to_str( o.custom_house_flags, out ); out += ',';
  out += "unknown11="; base::to_str( o.unknown11, out ); out += ',';
  out += "hammers="; base::to_str( o.hammers, out ); out += ',';
  out += "building_in_production="; base::to_str( o.building_in_production, out ); out += ',';
  out += "warehouse_level="; base::to_str( o.warehouse_level, out ); out += ',';
  out += "unknown12a="; base::to_str( o.unknown12a, out ); out += ',';
  out += "depletion_counter="; base::to_str( o.depletion_counter, out ); out += ',';
  out += "hammers_purchased="; base::to_str( o.hammers_purchased, out ); out += ',';
  out += "stock="; base::to_str( o.stock, out ); out += ',';
  out += "population_on_map="; base::to_str( o.population_on_map, out ); out += ',';
  out += "fortification_on_map="; base::to_str( o.fortification_on_map, out ); out += ',';
  out += "rebel_dividend="; base::to_str( o.rebel_dividend, out ); out += ',';
  out += "rebel_divisor="; base::to_str( o.rebel_divisor, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, COLONY& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.name )
    && read_binary( b, o.nation_id )
    && read_binary( b, o.unknown08a )
    && read_binary( b, o.colony_flags )
    && read_binary( b, o.unknown08b )
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
    && read_binary( b, o.unknown12a )
    && read_binary( b, o.depletion_counter )
    && read_binary( b, o.hammers_purchased )
    && read_binary( b, o.stock )
    && read_binary( b, o.population_on_map )
    && read_binary( b, o.fortification_on_map )
    && read_binary( b, o.rebel_dividend )
    && read_binary( b, o.rebel_divisor )
    ;
}

bool write_binary( base::IBinaryIO& b, COLONY const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.name )
    && write_binary( b, o.nation_id )
    && write_binary( b, o.unknown08a )
    && write_binary( b, o.colony_flags )
    && write_binary( b, o.unknown08b )
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
    && write_binary( b, o.unknown12a )
    && write_binary( b, o.depletion_counter )
    && write_binary( b, o.hammers_purchased )
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
  conv.to_field( tbl, "unknown08a", o.unknown08a );
  conv.to_field( tbl, "colony_flags", o.colony_flags );
  conv.to_field( tbl, "unknown08b", o.unknown08b );
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
  conv.to_field( tbl, "unknown12a", o.unknown12a );
  conv.to_field( tbl, "depletion_counter", o.depletion_counter );
  conv.to_field( tbl, "hammers_purchased", o.hammers_purchased );
  conv.to_field( tbl, "stock", o.stock );
  conv.to_field( tbl, "population_on_map", o.population_on_map );
  conv.to_field( tbl, "fortification_on_map", o.fortification_on_map );
  conv.to_field( tbl, "rebel_dividend", o.rebel_dividend );
  conv.to_field( tbl, "rebel_divisor", o.rebel_divisor );
  tbl["__key_order"] = cdr::list{
    "x, y",
    "name",
    "nation_id",
    "unknown08a",
    "colony_flags",
    "unknown08b",
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
    "unknown12a",
    "depletion_counter",
    "hammers_purchased",
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
  CONV_FROM_FIELD( "unknown08a", unknown08a );
  CONV_FROM_FIELD( "colony_flags", colony_flags );
  CONV_FROM_FIELD( "unknown08b", unknown08b );
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
  CONV_FROM_FIELD( "unknown12a", unknown12a );
  CONV_FROM_FIELD( "depletion_counter", depletion_counter );
  CONV_FROM_FIELD( "hammers_purchased", hammers_purchased );
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
void to_str( TransportChain const& o, std::string& out, base::tag<TransportChain> ) {
  out += "TransportChain{";
  out += "next_unit_idx="; base::to_str( o.next_unit_idx, out ); out += ',';
  out += "prev_unit_idx="; base::to_str( o.prev_unit_idx, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TransportChain& o ) {
  return true
    && read_binary( b, o.next_unit_idx )
    && read_binary( b, o.prev_unit_idx )
    ;
}

bool write_binary( base::IBinaryIO& b, TransportChain const& o ) {
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
void to_str( UNIT const& o, std::string& out, base::tag<UNIT> ) {
  out += "UNIT{";
  out += "x_y="; base::to_str( o.x_y, out ); out += ',';
  out += "type="; base::to_str( o.type, out ); out += ',';
  out += "nation_info="; base::to_str( o.nation_info, out ); out += ',';
  out += "unknown15="; base::to_str( o.unknown15, out ); out += ',';
  out += "moves="; base::to_str( o.moves, out ); out += ',';
  out += "origin_settlement="; base::to_str( o.origin_settlement, out ); out += ',';
  out += "ai_plan_mode="; base::to_str( o.ai_plan_mode, out ); out += ',';
  out += "orders="; base::to_str( o.orders, out ); out += ',';
  out += "goto_x="; base::to_str( o.goto_x, out ); out += ',';
  out += "goto_y="; base::to_str( o.goto_y, out ); out += ',';
  out += "unknown18="; base::to_str( o.unknown18, out ); out += ',';
  out += "holds_occupied="; base::to_str( o.holds_occupied, out ); out += ',';
  out += "cargo_items="; base::to_str( o.cargo_items, out ); out += ',';
  out += "cargo_hold="; base::to_str( o.cargo_hold, out ); out += ',';
  out += "turns_worked="; base::to_str( o.turns_worked, out ); out += ',';
  out += "profession_or_treasure_amount="; base::to_str( o.profession_or_treasure_amount, out ); out += ',';
  out += "transport_chain="; base::to_str( o.transport_chain, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UNIT& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.type )
    && read_binary( b, o.nation_info )
    && read_binary( b, o.unknown15 )
    && read_binary( b, o.moves )
    && read_binary( b, o.origin_settlement )
    && read_binary( b, o.ai_plan_mode )
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

bool write_binary( base::IBinaryIO& b, UNIT const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.type )
    && write_binary( b, o.nation_info )
    && write_binary( b, o.unknown15 )
    && write_binary( b, o.moves )
    && write_binary( b, o.origin_settlement )
    && write_binary( b, o.ai_plan_mode )
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
  conv.to_field( tbl, "ai_plan_mode", o.ai_plan_mode );
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
    "ai_plan_mode",
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
  CONV_FROM_FIELD( "ai_plan_mode", ai_plan_mode );
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
** IntrinsicVolume
*****************************************************************/
void to_str( IntrinsicVolume const& o, std::string& out, base::tag<IntrinsicVolume> ) {
  out += "IntrinsicVolume{";
  out += "food="; base::to_str( o.food, out ); out += ',';
  out += "sugar="; base::to_str( o.sugar, out ); out += ',';
  out += "tobacco="; base::to_str( o.tobacco, out ); out += ',';
  out += "cotton="; base::to_str( o.cotton, out ); out += ',';
  out += "furs="; base::to_str( o.furs, out ); out += ',';
  out += "lumber="; base::to_str( o.lumber, out ); out += ',';
  out += "ore="; base::to_str( o.ore, out ); out += ',';
  out += "silver="; base::to_str( o.silver, out ); out += ',';
  out += "horses="; base::to_str( o.horses, out ); out += ',';
  out += "rum="; base::to_str( o.rum, out ); out += ',';
  out += "cigars="; base::to_str( o.cigars, out ); out += ',';
  out += "cloth="; base::to_str( o.cloth, out ); out += ',';
  out += "coats="; base::to_str( o.coats, out ); out += ',';
  out += "trade_goods="; base::to_str( o.trade_goods, out ); out += ',';
  out += "tools="; base::to_str( o.tools, out ); out += ',';
  out += "muskets="; base::to_str( o.muskets, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, IntrinsicVolume& o ) {
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

bool write_binary( base::IBinaryIO& b, IntrinsicVolume const& o ) {
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
                         IntrinsicVolume const& o,
                         cdr::tag_t<IntrinsicVolume> ) {
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

cdr::result<IntrinsicVolume> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<IntrinsicVolume> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  IntrinsicVolume res = {};
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
** Trade
*****************************************************************/
void to_str( Trade const& o, std::string& out, base::tag<Trade> ) {
  out += "Trade{";
  out += "euro_price="; base::to_str( o.euro_price, out ); out += ',';
  out += "intrinsic_volume="; base::to_str( o.intrinsic_volume, out ); out += ',';
  out += "gold="; base::to_str( o.gold, out ); out += ',';
  out += "tons_traded="; base::to_str( o.tons_traded, out ); out += ',';
  out += "tons_traded2="; base::to_str( o.tons_traded2, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Trade& o ) {
  return true
    && read_binary( b, o.euro_price )
    && read_binary( b, o.intrinsic_volume )
    && read_binary( b, o.gold )
    && read_binary( b, o.tons_traded )
    && read_binary( b, o.tons_traded2 )
    ;
}

bool write_binary( base::IBinaryIO& b, Trade const& o ) {
  return true
    && write_binary( b, o.euro_price )
    && write_binary( b, o.intrinsic_volume )
    && write_binary( b, o.gold )
    && write_binary( b, o.tons_traded )
    && write_binary( b, o.tons_traded2 )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         Trade const& o,
                         cdr::tag_t<Trade> ) {
  cdr::table tbl;
  conv.to_field( tbl, "euro_price", o.euro_price );
  conv.to_field( tbl, "intrinsic_volume", o.intrinsic_volume );
  conv.to_field( tbl, "gold", o.gold );
  conv.to_field( tbl, "tons_traded", o.tons_traded );
  conv.to_field( tbl, "tons_traded2", o.tons_traded2 );
  tbl["__key_order"] = cdr::list{
    "euro_price",
    "intrinsic_volume",
    "gold",
    "tons_traded",
    "tons_traded2",
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
  CONV_FROM_FIELD( "euro_price", euro_price );
  CONV_FROM_FIELD( "intrinsic_volume", intrinsic_volume );
  CONV_FROM_FIELD( "gold", gold );
  CONV_FROM_FIELD( "tons_traded", tons_traded );
  CONV_FROM_FIELD( "tons_traded2", tons_traded2 );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** NATION
*****************************************************************/
void to_str( NATION const& o, std::string& out, base::tag<NATION> ) {
  out += "NATION{";
  out += "nation_flags="; base::to_str( o.nation_flags, out ); out += ',';
  out += "tax_rate="; base::to_str( o.tax_rate, out ); out += ',';
  out += "recruit="; base::to_str( o.recruit, out ); out += ',';
  out += "unused07="; base::to_str( o.unused07, out ); out += ',';
  out += "recruit_count="; base::to_str( o.recruit_count, out ); out += ',';
  out += "founding_fathers="; base::to_str( o.founding_fathers, out ); out += ',';
  out += "unknown21="; base::to_str( o.unknown21, out ); out += ',';
  out += "liberty_bells_total="; base::to_str( o.liberty_bells_total, out ); out += ',';
  out += "liberty_bells_last_turn="; base::to_str( o.liberty_bells_last_turn, out ); out += ',';
  out += "unknown22="; base::to_str( o.unknown22, out ); out += ',';
  out += "next_founding_father="; base::to_str( o.next_founding_father, out ); out += ',';
  out += "founding_father_count="; base::to_str( o.founding_father_count, out ); out += ',';
  out += "prob_founding_father_count_end="; base::to_str( o.prob_founding_father_count_end, out ); out += ',';
  out += "villages_burned="; base::to_str( o.villages_burned, out ); out += ',';
  out += "rebel_sentiment="; base::to_str( o.rebel_sentiment, out ); out += ',';
  out += "unknown23="; base::to_str( o.unknown23, out ); out += ',';
  out += "artillery_bought_count="; base::to_str( o.artillery_bought_count, out ); out += ',';
  out += "boycott_bitmap="; base::to_str( o.boycott_bitmap, out ); out += ',';
  out += "royal_money="; base::to_str( o.royal_money, out ); out += ',';
  out += "player_total_income="; base::to_str( o.player_total_income, out ); out += ',';
  out += "gold="; base::to_str( o.gold, out ); out += ',';
  out += "current_crosses="; base::to_str( o.current_crosses, out ); out += ',';
  out += "needed_crosses="; base::to_str( o.needed_crosses, out ); out += ',';
  out += "point_return_from_europe="; base::to_str( o.point_return_from_europe, out ); out += ',';
  out += "relation_by_nations="; base::to_str( o.relation_by_nations, out ); out += ',';
  out += "relation_by_indian="; base::to_str( o.relation_by_indian, out ); out += ',';
  out += "unknown26a="; base::to_str( o.unknown26a, out ); out += ',';
  out += "unknown26b="; base::to_str( o.unknown26b, out ); out += ',';
  out += "unknown26c="; base::to_str( o.unknown26c, out ); out += ',';
  out += "trade="; base::to_str( o.trade, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NATION& o ) {
  return true
    && read_binary( b, o.nation_flags )
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
    && read_binary( b, o.rebel_sentiment )
    && read_binary( b, o.unknown23 )
    && read_binary( b, o.artillery_bought_count )
    && read_binary( b, o.boycott_bitmap )
    && read_binary( b, o.royal_money )
    && read_binary( b, o.player_total_income )
    && read_binary( b, o.gold )
    && read_binary( b, o.current_crosses )
    && read_binary( b, o.needed_crosses )
    && read_binary( b, o.point_return_from_europe )
    && read_binary( b, o.relation_by_nations )
    && read_binary( b, o.relation_by_indian )
    && read_binary( b, o.unknown26a )
    && read_binary( b, o.unknown26b )
    && read_binary( b, o.unknown26c )
    && read_binary( b, o.trade )
    ;
}

bool write_binary( base::IBinaryIO& b, NATION const& o ) {
  return true
    && write_binary( b, o.nation_flags )
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
    && write_binary( b, o.rebel_sentiment )
    && write_binary( b, o.unknown23 )
    && write_binary( b, o.artillery_bought_count )
    && write_binary( b, o.boycott_bitmap )
    && write_binary( b, o.royal_money )
    && write_binary( b, o.player_total_income )
    && write_binary( b, o.gold )
    && write_binary( b, o.current_crosses )
    && write_binary( b, o.needed_crosses )
    && write_binary( b, o.point_return_from_europe )
    && write_binary( b, o.relation_by_nations )
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
  conv.to_field( tbl, "nation_flags", o.nation_flags );
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
  conv.to_field( tbl, "rebel_sentiment", o.rebel_sentiment );
  conv.to_field( tbl, "unknown23", o.unknown23 );
  conv.to_field( tbl, "artillery_bought_count", o.artillery_bought_count );
  conv.to_field( tbl, "boycott_bitmap", o.boycott_bitmap );
  conv.to_field( tbl, "royal_money", o.royal_money );
  conv.to_field( tbl, "player_total_income", o.player_total_income );
  conv.to_field( tbl, "gold", o.gold );
  conv.to_field( tbl, "current_crosses", o.current_crosses );
  conv.to_field( tbl, "needed_crosses", o.needed_crosses );
  conv.to_field( tbl, "point_return_from_europe", o.point_return_from_europe );
  conv.to_field( tbl, "relation_by_nations", o.relation_by_nations );
  conv.to_field( tbl, "relation_by_indian", o.relation_by_indian );
  conv.to_field( tbl, "unknown26a", o.unknown26a );
  conv.to_field( tbl, "unknown26b", o.unknown26b );
  conv.to_field( tbl, "unknown26c", o.unknown26c );
  conv.to_field( tbl, "trade", o.trade );
  tbl["__key_order"] = cdr::list{
    "nation_flags",
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
    "rebel_sentiment",
    "unknown23",
    "artillery_bought_count",
    "boycott_bitmap",
    "royal_money",
    "player_total_income",
    "gold",
    "current_crosses",
    "needed_crosses",
    "point_return_from_europe",
    "relation_by_nations",
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
  CONV_FROM_FIELD( "nation_flags", nation_flags );
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
  CONV_FROM_FIELD( "rebel_sentiment", rebel_sentiment );
  CONV_FROM_FIELD( "unknown23", unknown23 );
  CONV_FROM_FIELD( "artillery_bought_count", artillery_bought_count );
  CONV_FROM_FIELD( "boycott_bitmap", boycott_bitmap );
  CONV_FROM_FIELD( "royal_money", royal_money );
  CONV_FROM_FIELD( "player_total_income", player_total_income );
  CONV_FROM_FIELD( "gold", gold );
  CONV_FROM_FIELD( "current_crosses", current_crosses );
  CONV_FROM_FIELD( "needed_crosses", needed_crosses );
  CONV_FROM_FIELD( "point_return_from_europe", point_return_from_europe );
  CONV_FROM_FIELD( "relation_by_nations", relation_by_nations );
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
void to_str( Alarm const& o, std::string& out, base::tag<Alarm> ) {
  out += "Alarm{";
  out += "friction="; base::to_str( o.friction, out ); out += ',';
  out += "attacks="; base::to_str( o.attacks, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Alarm& o ) {
  return true
    && read_binary( b, o.friction )
    && read_binary( b, o.attacks )
    ;
}

bool write_binary( base::IBinaryIO& b, Alarm const& o ) {
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
** DWELLING
*****************************************************************/
void to_str( DWELLING const& o, std::string& out, base::tag<DWELLING> ) {
  out += "DWELLING{";
  out += "x_y="; base::to_str( o.x_y, out ); out += ',';
  out += "nation_id="; base::to_str( o.nation_id, out ); out += ',';
  out += "blcs="; base::to_str( o.blcs, out ); out += ',';
  out += "population="; base::to_str( o.population, out ); out += ',';
  out += "mission="; base::to_str( o.mission, out ); out += ',';
  out += "growth_counter="; base::to_str( o.growth_counter, out ); out += ',';
  out += "unknown28a="; base::to_str( o.unknown28a, out ); out += ',';
  out += "last_bought="; base::to_str( o.last_bought, out ); out += ',';
  out += "last_sold="; base::to_str( o.last_sold, out ); out += ',';
  out += "alarm="; base::to_str( o.alarm, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, DWELLING& o ) {
  return true
    && read_binary( b, o.x_y )
    && read_binary( b, o.nation_id )
    && read_binary( b, o.blcs )
    && read_binary( b, o.population )
    && read_binary( b, o.mission )
    && read_binary( b, o.growth_counter )
    && read_binary( b, o.unknown28a )
    && read_binary( b, o.last_bought )
    && read_binary( b, o.last_sold )
    && read_binary( b, o.alarm )
    ;
}

bool write_binary( base::IBinaryIO& b, DWELLING const& o ) {
  return true
    && write_binary( b, o.x_y )
    && write_binary( b, o.nation_id )
    && write_binary( b, o.blcs )
    && write_binary( b, o.population )
    && write_binary( b, o.mission )
    && write_binary( b, o.growth_counter )
    && write_binary( b, o.unknown28a )
    && write_binary( b, o.last_bought )
    && write_binary( b, o.last_sold )
    && write_binary( b, o.alarm )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         DWELLING const& o,
                         cdr::tag_t<DWELLING> ) {
  cdr::table tbl;
  conv.to_field( tbl, "x, y", o.x_y );
  conv.to_field( tbl, "nation_id", o.nation_id );
  conv.to_field( tbl, "BLCS", o.blcs );
  conv.to_field( tbl, "population", o.population );
  conv.to_field( tbl, "mission", o.mission );
  conv.to_field( tbl, "growth_counter", o.growth_counter );
  conv.to_field( tbl, "unknown28a", o.unknown28a );
  conv.to_field( tbl, "last_bought", o.last_bought );
  conv.to_field( tbl, "last_sold", o.last_sold );
  conv.to_field( tbl, "alarm", o.alarm );
  tbl["__key_order"] = cdr::list{
    "x, y",
    "nation_id",
    "BLCS",
    "population",
    "mission",
    "growth_counter",
    "unknown28a",
    "last_bought",
    "last_sold",
    "alarm",
  };
  return tbl;
}

cdr::result<DWELLING> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<DWELLING> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  DWELLING res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "x, y", x_y );
  CONV_FROM_FIELD( "nation_id", nation_id );
  CONV_FROM_FIELD( "BLCS", blcs );
  CONV_FROM_FIELD( "population", population );
  CONV_FROM_FIELD( "mission", mission );
  CONV_FROM_FIELD( "growth_counter", growth_counter );
  CONV_FROM_FIELD( "unknown28a", unknown28a );
  CONV_FROM_FIELD( "last_bought", last_bought );
  CONV_FROM_FIELD( "last_sold", last_sold );
  CONV_FROM_FIELD( "alarm", alarm );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TRIBE
*****************************************************************/
void to_str( TRIBE const& o, std::string& out, base::tag<TRIBE> ) {
  out += "TRIBE{";
  out += "capitol_x_y="; base::to_str( o.capitol_x_y, out ); out += ',';
  out += "tech="; base::to_str( o.tech, out ); out += ',';
  out += "tribe_flags="; base::to_str( o.tribe_flags, out ); out += ',';
  out += "unknown31b="; base::to_str( o.unknown31b, out ); out += ',';
  out += "muskets="; base::to_str( o.muskets, out ); out += ',';
  out += "horse_herds="; base::to_str( o.horse_herds, out ); out += ',';
  out += "unknown31c="; base::to_str( o.unknown31c, out ); out += ',';
  out += "horse_breeding="; base::to_str( o.horse_breeding, out ); out += ',';
  out += "unknown31d="; base::to_str( o.unknown31d, out ); out += ',';
  out += "stock="; base::to_str( o.stock, out ); out += ',';
  out += "unknown32="; base::to_str( o.unknown32, out ); out += ',';
  out += "relation_by_nations="; base::to_str( o.relation_by_nations, out ); out += ',';
  out += "zeros33="; base::to_str( o.zeros33, out ); out += ',';
  out += "alarm_by_player="; base::to_str( o.alarm_by_player, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TRIBE& o ) {
  return true
    && read_binary( b, o.capitol_x_y )
    && read_binary( b, o.tech )
    && read_binary( b, o.tribe_flags )
    && read_binary( b, o.unknown31b )
    && read_binary( b, o.muskets )
    && read_binary( b, o.horse_herds )
    && read_binary( b, o.unknown31c )
    && read_binary( b, o.horse_breeding )
    && read_binary( b, o.unknown31d )
    && read_binary( b, o.stock )
    && read_binary( b, o.unknown32 )
    && read_binary( b, o.relation_by_nations )
    && read_binary( b, o.zeros33 )
    && read_binary( b, o.alarm_by_player )
    ;
}

bool write_binary( base::IBinaryIO& b, TRIBE const& o ) {
  return true
    && write_binary( b, o.capitol_x_y )
    && write_binary( b, o.tech )
    && write_binary( b, o.tribe_flags )
    && write_binary( b, o.unknown31b )
    && write_binary( b, o.muskets )
    && write_binary( b, o.horse_herds )
    && write_binary( b, o.unknown31c )
    && write_binary( b, o.horse_breeding )
    && write_binary( b, o.unknown31d )
    && write_binary( b, o.stock )
    && write_binary( b, o.unknown32 )
    && write_binary( b, o.relation_by_nations )
    && write_binary( b, o.zeros33 )
    && write_binary( b, o.alarm_by_player )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         TRIBE const& o,
                         cdr::tag_t<TRIBE> ) {
  cdr::table tbl;
  conv.to_field( tbl, "capitol (x, y)", o.capitol_x_y );
  conv.to_field( tbl, "tech", o.tech );
  conv.to_field( tbl, "tribe_flags", o.tribe_flags );
  conv.to_field( tbl, "unknown31b", o.unknown31b );
  conv.to_field( tbl, "muskets", o.muskets );
  conv.to_field( tbl, "horse_herds", o.horse_herds );
  conv.to_field( tbl, "unknown31c", o.unknown31c );
  conv.to_field( tbl, "horse_breeding", o.horse_breeding );
  conv.to_field( tbl, "unknown31d", o.unknown31d );
  conv.to_field( tbl, "stock", o.stock );
  conv.to_field( tbl, "unknown32", o.unknown32 );
  conv.to_field( tbl, "relation_by_nations", o.relation_by_nations );
  conv.to_field( tbl, "zeros33", o.zeros33 );
  conv.to_field( tbl, "alarm_by_player", o.alarm_by_player );
  tbl["__key_order"] = cdr::list{
    "capitol (x, y)",
    "tech",
    "tribe_flags",
    "unknown31b",
    "muskets",
    "horse_herds",
    "unknown31c",
    "horse_breeding",
    "unknown31d",
    "stock",
    "unknown32",
    "relation_by_nations",
    "zeros33",
    "alarm_by_player",
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
  CONV_FROM_FIELD( "capitol (x, y)", capitol_x_y );
  CONV_FROM_FIELD( "tech", tech );
  CONV_FROM_FIELD( "tribe_flags", tribe_flags );
  CONV_FROM_FIELD( "unknown31b", unknown31b );
  CONV_FROM_FIELD( "muskets", muskets );
  CONV_FROM_FIELD( "horse_herds", horse_herds );
  CONV_FROM_FIELD( "unknown31c", unknown31c );
  CONV_FROM_FIELD( "horse_breeding", horse_breeding );
  CONV_FROM_FIELD( "unknown31d", unknown31d );
  CONV_FROM_FIELD( "stock", stock );
  CONV_FROM_FIELD( "unknown32", unknown32 );
  CONV_FROM_FIELD( "relation_by_nations", relation_by_nations );
  CONV_FROM_FIELD( "zeros33", zeros33 );
  CONV_FROM_FIELD( "alarm_by_player", alarm_by_player );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** NationUnitCount
*****************************************************************/
void to_str( NationUnitCount const& o, std::string& out, base::tag<NationUnitCount> ) {
  out += "NationUnitCount{";
  out += "english="; base::to_str( o.english, out ); out += ',';
  out += "french="; base::to_str( o.french, out ); out += ',';
  out += "spanish="; base::to_str( o.spanish, out ); out += ',';
  out += "dutch="; base::to_str( o.dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationUnitCount& o ) {
  return true
    && read_binary( b, o.english )
    && read_binary( b, o.french )
    && read_binary( b, o.spanish )
    && read_binary( b, o.dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, NationUnitCount const& o ) {
  return true
    && write_binary( b, o.english )
    && write_binary( b, o.french )
    && write_binary( b, o.spanish )
    && write_binary( b, o.dutch )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         NationUnitCount const& o,
                         cdr::tag_t<NationUnitCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "english", o.english );
  conv.to_field( tbl, "french", o.french );
  conv.to_field( tbl, "spanish", o.spanish );
  conv.to_field( tbl, "dutch", o.dutch );
  tbl["__key_order"] = cdr::list{
    "english",
    "french",
    "spanish",
    "dutch",
  };
  return tbl;
}

cdr::result<NationUnitCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationUnitCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  NationUnitCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "english", english );
  CONV_FROM_FIELD( "french", french );
  CONV_FROM_FIELD( "spanish", spanish );
  CONV_FROM_FIELD( "dutch", dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** NationColonyCount
*****************************************************************/
void to_str( NationColonyCount const& o, std::string& out, base::tag<NationColonyCount> ) {
  out += "NationColonyCount{";
  out += "english="; base::to_str( o.english, out ); out += ',';
  out += "french="; base::to_str( o.french, out ); out += ',';
  out += "spanish="; base::to_str( o.spanish, out ); out += ',';
  out += "dutch="; base::to_str( o.dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationColonyCount& o ) {
  return true
    && read_binary( b, o.english )
    && read_binary( b, o.french )
    && read_binary( b, o.spanish )
    && read_binary( b, o.dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, NationColonyCount const& o ) {
  return true
    && write_binary( b, o.english )
    && write_binary( b, o.french )
    && write_binary( b, o.spanish )
    && write_binary( b, o.dutch )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         NationColonyCount const& o,
                         cdr::tag_t<NationColonyCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "english", o.english );
  conv.to_field( tbl, "french", o.french );
  conv.to_field( tbl, "spanish", o.spanish );
  conv.to_field( tbl, "dutch", o.dutch );
  tbl["__key_order"] = cdr::list{
    "english",
    "french",
    "spanish",
    "dutch",
  };
  return tbl;
}

cdr::result<NationColonyCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationColonyCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  NationColonyCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "english", english );
  CONV_FROM_FIELD( "french", french );
  CONV_FROM_FIELD( "spanish", spanish );
  CONV_FROM_FIELD( "dutch", dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Unknown34a
*****************************************************************/
void to_str( Unknown34a const& o, std::string& out, base::tag<Unknown34a> ) {
  out += "Unknown34a{";
  out += "english="; base::to_str( o.english, out ); out += ',';
  out += "french="; base::to_str( o.french, out ); out += ',';
  out += "spanish="; base::to_str( o.spanish, out ); out += ',';
  out += "dutch="; base::to_str( o.dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown34a& o ) {
  return true
    && read_binary( b, o.english )
    && read_binary( b, o.french )
    && read_binary( b, o.spanish )
    && read_binary( b, o.dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, Unknown34a const& o ) {
  return true
    && write_binary( b, o.english )
    && write_binary( b, o.french )
    && write_binary( b, o.spanish )
    && write_binary( b, o.dutch )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         Unknown34a const& o,
                         cdr::tag_t<Unknown34a> ) {
  cdr::table tbl;
  conv.to_field( tbl, "english", o.english );
  conv.to_field( tbl, "french", o.french );
  conv.to_field( tbl, "spanish", o.spanish );
  conv.to_field( tbl, "dutch", o.dutch );
  tbl["__key_order"] = cdr::list{
    "english",
    "french",
    "spanish",
    "dutch",
  };
  return tbl;
}

cdr::result<Unknown34a> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown34a> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Unknown34a res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "english", english );
  CONV_FROM_FIELD( "french", french );
  CONV_FROM_FIELD( "spanish", spanish );
  CONV_FROM_FIELD( "dutch", dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TotalColoniesPopulation
*****************************************************************/
void to_str( TotalColoniesPopulation const& o, std::string& out, base::tag<TotalColoniesPopulation> ) {
  out += "TotalColoniesPopulation{";
  out += "english="; base::to_str( o.english, out ); out += ',';
  out += "french="; base::to_str( o.french, out ); out += ',';
  out += "spanish="; base::to_str( o.spanish, out ); out += ',';
  out += "dutch="; base::to_str( o.dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TotalColoniesPopulation& o ) {
  return true
    && read_binary( b, o.english )
    && read_binary( b, o.french )
    && read_binary( b, o.spanish )
    && read_binary( b, o.dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, TotalColoniesPopulation const& o ) {
  return true
    && write_binary( b, o.english )
    && write_binary( b, o.french )
    && write_binary( b, o.spanish )
    && write_binary( b, o.dutch )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         TotalColoniesPopulation const& o,
                         cdr::tag_t<TotalColoniesPopulation> ) {
  cdr::table tbl;
  conv.to_field( tbl, "english", o.english );
  conv.to_field( tbl, "french", o.french );
  conv.to_field( tbl, "spanish", o.spanish );
  conv.to_field( tbl, "dutch", o.dutch );
  tbl["__key_order"] = cdr::list{
    "english",
    "french",
    "spanish",
    "dutch",
  };
  return tbl;
}

cdr::result<TotalColoniesPopulation> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TotalColoniesPopulation> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TotalColoniesPopulation res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "english", english );
  CONV_FROM_FIELD( "french", french );
  CONV_FROM_FIELD( "spanish", spanish );
  CONV_FROM_FIELD( "dutch", dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** Unknown36ab
*****************************************************************/
void to_str( Unknown36ab const& o, std::string& out, base::tag<Unknown36ab> ) {
  out += "Unknown36ab{";
  out += "english="; base::to_str( o.english, out ); out += ',';
  out += "french="; base::to_str( o.french, out ); out += ',';
  out += "spanish="; base::to_str( o.spanish, out ); out += ',';
  out += "dutch="; base::to_str( o.dutch, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown36ab& o ) {
  return true
    && read_binary( b, o.english )
    && read_binary( b, o.french )
    && read_binary( b, o.spanish )
    && read_binary( b, o.dutch )
    ;
}

bool write_binary( base::IBinaryIO& b, Unknown36ab const& o ) {
  return true
    && write_binary( b, o.english )
    && write_binary( b, o.french )
    && write_binary( b, o.spanish )
    && write_binary( b, o.dutch )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         Unknown36ab const& o,
                         cdr::tag_t<Unknown36ab> ) {
  cdr::table tbl;
  conv.to_field( tbl, "english", o.english );
  conv.to_field( tbl, "french", o.french );
  conv.to_field( tbl, "spanish", o.spanish );
  conv.to_field( tbl, "dutch", o.dutch );
  tbl["__key_order"] = cdr::list{
    "english",
    "french",
    "spanish",
    "dutch",
  };
  return tbl;
}

cdr::result<Unknown36ab> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown36ab> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  Unknown36ab res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "english", english );
  CONV_FROM_FIELD( "french", french );
  CONV_FROM_FIELD( "spanish", spanish );
  CONV_FROM_FIELD( "dutch", dutch );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** ForeignAffairsReport
*****************************************************************/
void to_str( ForeignAffairsReport const& o, std::string& out, base::tag<ForeignAffairsReport> ) {
  out += "ForeignAffairsReport{";
  out += "population="; base::to_str( o.population, out ); out += ',';
  out += "unknown36ab="; base::to_str( o.unknown36ab, out ); out += ',';
  out += "merchant_marine="; base::to_str( o.merchant_marine, out ); out += ',';
  out += "ship_counts="; base::to_str( o.ship_counts, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ForeignAffairsReport& o ) {
  return true
    && read_binary( b, o.population )
    && read_binary( b, o.unknown36ab )
    && read_binary( b, o.merchant_marine )
    && read_binary( b, o.ship_counts )
    ;
}

bool write_binary( base::IBinaryIO& b, ForeignAffairsReport const& o ) {
  return true
    && write_binary( b, o.population )
    && write_binary( b, o.unknown36ab )
    && write_binary( b, o.merchant_marine )
    && write_binary( b, o.ship_counts )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         ForeignAffairsReport const& o,
                         cdr::tag_t<ForeignAffairsReport> ) {
  cdr::table tbl;
  conv.to_field( tbl, "population", o.population );
  conv.to_field( tbl, "unknown36ab", o.unknown36ab );
  conv.to_field( tbl, "merchant_marine", o.merchant_marine );
  conv.to_field( tbl, "ship_counts", o.ship_counts );
  tbl["__key_order"] = cdr::list{
    "population",
    "unknown36ab",
    "merchant_marine",
    "ship_counts",
  };
  return tbl;
}

cdr::result<ForeignAffairsReport> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ForeignAffairsReport> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  ForeignAffairsReport res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "population", population );
  CONV_FROM_FIELD( "unknown36ab", unknown36ab );
  CONV_FROM_FIELD( "merchant_marine", merchant_marine );
  CONV_FROM_FIELD( "ship_counts", ship_counts );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** UnknownTribeData1
*****************************************************************/
void to_str( UnknownTribeData1 const& o, std::string& out, base::tag<UnknownTribeData1> ) {
  out += "UnknownTribeData1{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData1& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, UnknownTribeData1 const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData1 const& o,
                         cdr::tag_t<UnknownTribeData1> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<UnknownTribeData1> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData1> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UnknownTribeData1 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** UnknownTribeData2
*****************************************************************/
void to_str( UnknownTribeData2 const& o, std::string& out, base::tag<UnknownTribeData2> ) {
  out += "UnknownTribeData2{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData2& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, UnknownTribeData2 const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData2 const& o,
                         cdr::tag_t<UnknownTribeData2> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<UnknownTribeData2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData2> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UnknownTribeData2 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TribeDwellingCount
*****************************************************************/
void to_str( TribeDwellingCount const& o, std::string& out, base::tag<TribeDwellingCount> ) {
  out += "TribeDwellingCount{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TribeDwellingCount& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, TribeDwellingCount const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         TribeDwellingCount const& o,
                         cdr::tag_t<TribeDwellingCount> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<TribeDwellingCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TribeDwellingCount> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  TribeDwellingCount res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** UnknownTribeData4
*****************************************************************/
void to_str( UnknownTribeData4 const& o, std::string& out, base::tag<UnknownTribeData4> ) {
  out += "UnknownTribeData4{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData4& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, UnknownTribeData4 const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData4 const& o,
                         cdr::tag_t<UnknownTribeData4> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<UnknownTribeData4> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData4> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UnknownTribeData4 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** UnknownTribeData5
*****************************************************************/
void to_str( UnknownTribeData5 const& o, std::string& out, base::tag<UnknownTribeData5> ) {
  out += "UnknownTribeData5{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData5& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, UnknownTribeData5 const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData5 const& o,
                         cdr::tag_t<UnknownTribeData5> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<UnknownTribeData5> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData5> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UnknownTribeData5 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** UnknownTribeData6
*****************************************************************/
void to_str( UnknownTribeData6 const& o, std::string& out, base::tag<UnknownTribeData6> ) {
  out += "UnknownTribeData6{";
  out += "inca="; base::to_str( o.inca, out ); out += ',';
  out += "aztec="; base::to_str( o.aztec, out ); out += ',';
  out += "arawak="; base::to_str( o.arawak, out ); out += ',';
  out += "iroquois="; base::to_str( o.iroquois, out ); out += ',';
  out += "cherokee="; base::to_str( o.cherokee, out ); out += ',';
  out += "apache="; base::to_str( o.apache, out ); out += ',';
  out += "sioux="; base::to_str( o.sioux, out ); out += ',';
  out += "tupi="; base::to_str( o.tupi, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData6& o ) {
  return true
    && read_binary( b, o.inca )
    && read_binary( b, o.aztec )
    && read_binary( b, o.arawak )
    && read_binary( b, o.iroquois )
    && read_binary( b, o.cherokee )
    && read_binary( b, o.apache )
    && read_binary( b, o.sioux )
    && read_binary( b, o.tupi )
    ;
}

bool write_binary( base::IBinaryIO& b, UnknownTribeData6 const& o ) {
  return true
    && write_binary( b, o.inca )
    && write_binary( b, o.aztec )
    && write_binary( b, o.arawak )
    && write_binary( b, o.iroquois )
    && write_binary( b, o.cherokee )
    && write_binary( b, o.apache )
    && write_binary( b, o.sioux )
    && write_binary( b, o.tupi )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData6 const& o,
                         cdr::tag_t<UnknownTribeData6> ) {
  cdr::table tbl;
  conv.to_field( tbl, "inca", o.inca );
  conv.to_field( tbl, "aztec", o.aztec );
  conv.to_field( tbl, "arawak", o.arawak );
  conv.to_field( tbl, "iroquois", o.iroquois );
  conv.to_field( tbl, "cherokee", o.cherokee );
  conv.to_field( tbl, "apache", o.apache );
  conv.to_field( tbl, "sioux", o.sioux );
  conv.to_field( tbl, "tupi", o.tupi );
  tbl["__key_order"] = cdr::list{
    "inca",
    "aztec",
    "arawak",
    "iroquois",
    "cherokee",
    "apache",
    "sioux",
    "tupi",
  };
  return tbl;
}

cdr::result<UnknownTribeData6> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData6> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  UnknownTribeData6 res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "inca", inca );
  CONV_FROM_FIELD( "aztec", aztec );
  CONV_FROM_FIELD( "arawak", arawak );
  CONV_FROM_FIELD( "iroquois", iroquois );
  CONV_FROM_FIELD( "cherokee", cherokee );
  CONV_FROM_FIELD( "apache", apache );
  CONV_FROM_FIELD( "sioux", sioux );
  CONV_FROM_FIELD( "tupi", tupi );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** STUFF
*****************************************************************/
void to_str( STUFF const& o, std::string& out, base::tag<STUFF> ) {
  out += "STUFF{";
  out += "unknown34="; base::to_str( o.unknown34, out ); out += ',';
  out += "nation_unit_count="; base::to_str( o.nation_unit_count, out ); out += ',';
  out += "nation_colony_count="; base::to_str( o.nation_colony_count, out ); out += ',';
  out += "unknown34a="; base::to_str( o.unknown34a, out ); out += ',';
  out += "total_colonies_population="; base::to_str( o.total_colonies_population, out ); out += ',';
  out += "foreign_affairs_report="; base::to_str( o.foreign_affairs_report, out ); out += ',';
  out += "unknown36ac="; base::to_str( o.unknown36ac, out ); out += ',';
  out += "unknown36ad="; base::to_str( o.unknown36ad, out ); out += ',';
  out += "show_colony_prod_quantities="; base::to_str( o.show_colony_prod_quantities, out ); out += ',';
  out += "unknown_tribe_data_1="; base::to_str( o.unknown_tribe_data_1, out ); out += ',';
  out += "unknown_tribe_data_2="; base::to_str( o.unknown_tribe_data_2, out ); out += ',';
  out += "tribe_dwelling_count="; base::to_str( o.tribe_dwelling_count, out ); out += ',';
  out += "unknown_tribe_data_4="; base::to_str( o.unknown_tribe_data_4, out ); out += ',';
  out += "unknown_tribe_data_5="; base::to_str( o.unknown_tribe_data_5, out ); out += ',';
  out += "unknown_tribe_data_6="; base::to_str( o.unknown_tribe_data_6, out ); out += ',';
  out += "unknown36b="; base::to_str( o.unknown36b, out ); out += ',';
  out += "white_box_x="; base::to_str( o.white_box_x, out ); out += ',';
  out += "white_box_y="; base::to_str( o.white_box_y, out ); out += ',';
  out += "zoom_level="; base::to_str( o.zoom_level, out ); out += ',';
  out += "unknown37="; base::to_str( o.unknown37, out ); out += ',';
  out += "viewport_x="; base::to_str( o.viewport_x, out ); out += ',';
  out += "viewport_y="; base::to_str( o.viewport_y, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, STUFF& o ) {
  return true
    && read_binary( b, o.unknown34 )
    && read_binary( b, o.nation_unit_count )
    && read_binary( b, o.nation_colony_count )
    && read_binary( b, o.unknown34a )
    && read_binary( b, o.total_colonies_population )
    && read_binary( b, o.foreign_affairs_report )
    && read_binary( b, o.unknown36ac )
    && read_binary( b, o.unknown36ad )
    && read_binary( b, o.show_colony_prod_quantities )
    && read_binary( b, o.unknown_tribe_data_1 )
    && read_binary( b, o.unknown_tribe_data_2 )
    && read_binary( b, o.tribe_dwelling_count )
    && read_binary( b, o.unknown_tribe_data_4 )
    && read_binary( b, o.unknown_tribe_data_5 )
    && read_binary( b, o.unknown_tribe_data_6 )
    && read_binary( b, o.unknown36b )
    && read_binary( b, o.white_box_x )
    && read_binary( b, o.white_box_y )
    && read_binary( b, o.zoom_level )
    && read_binary( b, o.unknown37 )
    && read_binary( b, o.viewport_x )
    && read_binary( b, o.viewport_y )
    ;
}

bool write_binary( base::IBinaryIO& b, STUFF const& o ) {
  return true
    && write_binary( b, o.unknown34 )
    && write_binary( b, o.nation_unit_count )
    && write_binary( b, o.nation_colony_count )
    && write_binary( b, o.unknown34a )
    && write_binary( b, o.total_colonies_population )
    && write_binary( b, o.foreign_affairs_report )
    && write_binary( b, o.unknown36ac )
    && write_binary( b, o.unknown36ad )
    && write_binary( b, o.show_colony_prod_quantities )
    && write_binary( b, o.unknown_tribe_data_1 )
    && write_binary( b, o.unknown_tribe_data_2 )
    && write_binary( b, o.tribe_dwelling_count )
    && write_binary( b, o.unknown_tribe_data_4 )
    && write_binary( b, o.unknown_tribe_data_5 )
    && write_binary( b, o.unknown_tribe_data_6 )
    && write_binary( b, o.unknown36b )
    && write_binary( b, o.white_box_x )
    && write_binary( b, o.white_box_y )
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
  conv.to_field( tbl, "nation_unit_count", o.nation_unit_count );
  conv.to_field( tbl, "nation_colony_count", o.nation_colony_count );
  conv.to_field( tbl, "unknown34a", o.unknown34a );
  conv.to_field( tbl, "total_colonies_population", o.total_colonies_population );
  conv.to_field( tbl, "foreign_affairs_report", o.foreign_affairs_report );
  conv.to_field( tbl, "unknown36ac", o.unknown36ac );
  conv.to_field( tbl, "unknown36ad", o.unknown36ad );
  conv.to_field( tbl, "show_colony_prod_quantities", o.show_colony_prod_quantities );
  conv.to_field( tbl, "unknown_tribe_data_1", o.unknown_tribe_data_1 );
  conv.to_field( tbl, "unknown_tribe_data_2", o.unknown_tribe_data_2 );
  conv.to_field( tbl, "tribe_dwelling_count", o.tribe_dwelling_count );
  conv.to_field( tbl, "unknown_tribe_data_4", o.unknown_tribe_data_4 );
  conv.to_field( tbl, "unknown_tribe_data_5", o.unknown_tribe_data_5 );
  conv.to_field( tbl, "unknown_tribe_data_6", o.unknown_tribe_data_6 );
  conv.to_field( tbl, "unknown36b", o.unknown36b );
  conv.to_field( tbl, "white_box_x", o.white_box_x );
  conv.to_field( tbl, "white_box_y", o.white_box_y );
  conv.to_field( tbl, "zoom_level", o.zoom_level );
  conv.to_field( tbl, "unknown37", o.unknown37 );
  conv.to_field( tbl, "viewport_x", o.viewport_x );
  conv.to_field( tbl, "viewport_y", o.viewport_y );
  tbl["__key_order"] = cdr::list{
    "unknown34",
    "nation_unit_count",
    "nation_colony_count",
    "unknown34a",
    "total_colonies_population",
    "foreign_affairs_report",
    "unknown36ac",
    "unknown36ad",
    "show_colony_prod_quantities",
    "unknown_tribe_data_1",
    "unknown_tribe_data_2",
    "tribe_dwelling_count",
    "unknown_tribe_data_4",
    "unknown_tribe_data_5",
    "unknown_tribe_data_6",
    "unknown36b",
    "white_box_x",
    "white_box_y",
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
  CONV_FROM_FIELD( "nation_unit_count", nation_unit_count );
  CONV_FROM_FIELD( "nation_colony_count", nation_colony_count );
  CONV_FROM_FIELD( "unknown34a", unknown34a );
  CONV_FROM_FIELD( "total_colonies_population", total_colonies_population );
  CONV_FROM_FIELD( "foreign_affairs_report", foreign_affairs_report );
  CONV_FROM_FIELD( "unknown36ac", unknown36ac );
  CONV_FROM_FIELD( "unknown36ad", unknown36ad );
  CONV_FROM_FIELD( "show_colony_prod_quantities", show_colony_prod_quantities );
  CONV_FROM_FIELD( "unknown_tribe_data_1", unknown_tribe_data_1 );
  CONV_FROM_FIELD( "unknown_tribe_data_2", unknown_tribe_data_2 );
  CONV_FROM_FIELD( "tribe_dwelling_count", tribe_dwelling_count );
  CONV_FROM_FIELD( "unknown_tribe_data_4", unknown_tribe_data_4 );
  CONV_FROM_FIELD( "unknown_tribe_data_5", unknown_tribe_data_5 );
  CONV_FROM_FIELD( "unknown_tribe_data_6", unknown_tribe_data_6 );
  CONV_FROM_FIELD( "unknown36b", unknown36b );
  CONV_FROM_FIELD( "white_box_x", white_box_x );
  CONV_FROM_FIELD( "white_box_y", white_box_y );
  CONV_FROM_FIELD( "zoom_level", zoom_level );
  CONV_FROM_FIELD( "unknown37", unknown37 );
  CONV_FROM_FIELD( "viewport_x", viewport_x );
  CONV_FROM_FIELD( "viewport_y", viewport_y );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** CONNECTIVITY
*****************************************************************/
void to_str( CONNECTIVITY const& o, std::string& out, base::tag<CONNECTIVITY> ) {
  out += "CONNECTIVITY{";
  out += "sea_lane_connectivity="; base::to_str( o.sea_lane_connectivity, out ); out += ',';
  out += "land_connectivity="; base::to_str( o.land_connectivity, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CONNECTIVITY& o ) {
  return true
    && read_binary( b, o.sea_lane_connectivity )
    && read_binary( b, o.land_connectivity )
    ;
}

bool write_binary( base::IBinaryIO& b, CONNECTIVITY const& o ) {
  return true
    && write_binary( b, o.sea_lane_connectivity )
    && write_binary( b, o.land_connectivity )
    ;
}

cdr::value to_canonical( cdr::converter& conv,
                         CONNECTIVITY const& o,
                         cdr::tag_t<CONNECTIVITY> ) {
  cdr::table tbl;
  conv.to_field( tbl, "sea_lane_connectivity", o.sea_lane_connectivity );
  conv.to_field( tbl, "land_connectivity", o.land_connectivity );
  tbl["__key_order"] = cdr::list{
    "sea_lane_connectivity",
    "land_connectivity",
  };
  return tbl;
}

cdr::result<CONNECTIVITY> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CONNECTIVITY> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  CONNECTIVITY res = {};
  std::set<std::string> used_keys;
  CONV_FROM_FIELD( "sea_lane_connectivity", sea_lane_connectivity );
  CONV_FROM_FIELD( "land_connectivity", land_connectivity );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

/****************************************************************
** TRADEROUTE
*****************************************************************/
void to_str( TRADEROUTE const& o, std::string& out, base::tag<TRADEROUTE> ) {
  out += "TRADEROUTE{";
  out += "name="; base::to_str( o.name, out ); out += ',';
  out += "land_or_sea="; base::to_str( o.land_or_sea, out ); out += ',';
  out += "stops_count="; base::to_str( o.stops_count, out ); out += ',';
  out += "stop_1_colony_index="; base::to_str( o.stop_1_colony_index, out ); out += ',';
  out += "stop_1_loads_and_unloads_count="; base::to_str( o.stop_1_loads_and_unloads_count, out ); out += ',';
  out += "stop_1_loads_cargo="; base::to_str( o.stop_1_loads_cargo, out ); out += ',';
  out += "stop_1_unloads_cargo="; base::to_str( o.stop_1_unloads_cargo, out ); out += ',';
  out += "unknown47="; base::to_str( o.unknown47, out ); out += ',';
  out += "stop_2_colony_index="; base::to_str( o.stop_2_colony_index, out ); out += ',';
  out += "stop_2_loads_and_unloads_count="; base::to_str( o.stop_2_loads_and_unloads_count, out ); out += ',';
  out += "stop_2_loads_cargo="; base::to_str( o.stop_2_loads_cargo, out ); out += ',';
  out += "stop_2_unloads_cargo="; base::to_str( o.stop_2_unloads_cargo, out ); out += ',';
  out += "unknown48="; base::to_str( o.unknown48, out ); out += ',';
  out += "stop_3_colony_index="; base::to_str( o.stop_3_colony_index, out ); out += ',';
  out += "stop_3_loads_and_unloads_count="; base::to_str( o.stop_3_loads_and_unloads_count, out ); out += ',';
  out += "stop_3_loads_cargo="; base::to_str( o.stop_3_loads_cargo, out ); out += ',';
  out += "stop_3_unloads_cargo="; base::to_str( o.stop_3_unloads_cargo, out ); out += ',';
  out += "unknown49="; base::to_str( o.unknown49, out ); out += ',';
  out += "stop_4_colony_index="; base::to_str( o.stop_4_colony_index, out ); out += ',';
  out += "stop_4_loads_and_unloads_count="; base::to_str( o.stop_4_loads_and_unloads_count, out ); out += ',';
  out += "stop_4_loads_cargo="; base::to_str( o.stop_4_loads_cargo, out ); out += ',';
  out += "stop_4_unloads_cargo="; base::to_str( o.stop_4_unloads_cargo, out ); out += ',';
  out += "unknown50="; base::to_str( o.unknown50, out );
  out += '}';
}

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TRADEROUTE& o ) {
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

bool write_binary( base::IBinaryIO& b, TRADEROUTE const& o ) {
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
void to_str( ColonySAV const& o, std::string& out, base::tag<ColonySAV> ) {
  out += "ColonySAV{";
  out += "header="; base::to_str( o.header, out ); out += ',';
  out += "player="; base::to_str( o.player, out ); out += ',';
  out += "other="; base::to_str( o.other, out ); out += ',';
  out += "colony="; base::to_str( o.colony, out ); out += ',';
  out += "unit="; base::to_str( o.unit, out ); out += ',';
  out += "nation="; base::to_str( o.nation, out ); out += ',';
  out += "dwelling="; base::to_str( o.dwelling, out ); out += ',';
  out += "tribe="; base::to_str( o.tribe, out ); out += ',';
  out += "stuff="; base::to_str( o.stuff, out ); out += ',';
  out += "tile="; base::to_str( o.tile, out ); out += ',';
  out += "mask="; base::to_str( o.mask, out ); out += ',';
  out += "path="; base::to_str( o.path, out ); out += ',';
  out += "seen="; base::to_str( o.seen, out ); out += ',';
  out += "connectivity="; base::to_str( o.connectivity, out ); out += ',';
  out += "unknown_map38c2="; base::to_str( o.unknown_map38c2, out ); out += ',';
  out += "unknown_map38c3="; base::to_str( o.unknown_map38c3, out ); out += ',';
  out += "strategy="; base::to_str( o.strategy, out ); out += ',';
  out += "unknown_map38d="; base::to_str( o.unknown_map38d, out ); out += ',';
  out += "prime_resource_seed="; base::to_str( o.prime_resource_seed, out ); out += ',';
  out += "unknown39d="; base::to_str( o.unknown39d, out ); out += ',';
  out += "trade_route="; base::to_str( o.trade_route, out );
  out += '}';
}

// NOTE: binary conversion manually implemented.

cdr::value to_canonical( cdr::converter& conv,
                         ColonySAV const& o,
                         cdr::tag_t<ColonySAV> ) {
  cdr::table tbl;
  conv.to_field( tbl, "HEADER", o.header );
  conv.to_field( tbl, "PLAYER", o.player );
  conv.to_field( tbl, "OTHER", o.other );
  conv.to_field( tbl, "COLONY", o.colony );
  conv.to_field( tbl, "UNIT", o.unit );
  conv.to_field( tbl, "NATION", o.nation );
  conv.to_field( tbl, "DWELLING", o.dwelling );
  conv.to_field( tbl, "TRIBE", o.tribe );
  conv.to_field( tbl, "STUFF", o.stuff );
  conv.to_field( tbl, "TILE", o.tile );
  conv.to_field( tbl, "MASK", o.mask );
  conv.to_field( tbl, "PATH", o.path );
  conv.to_field( tbl, "SEEN", o.seen );
  conv.to_field( tbl, "CONNECTIVITY", o.connectivity );
  conv.to_field( tbl, "unknown_map38c2", o.unknown_map38c2 );
  conv.to_field( tbl, "unknown_map38c3", o.unknown_map38c3 );
  conv.to_field( tbl, "strategy", o.strategy );
  conv.to_field( tbl, "unknown_map38d", o.unknown_map38d );
  conv.to_field( tbl, "prime_resource_seed", o.prime_resource_seed );
  conv.to_field( tbl, "unknown39d", o.unknown39d );
  conv.to_field( tbl, "TRADE_ROUTE", o.trade_route );
  tbl["__key_order"] = cdr::list{
    "HEADER",
    "PLAYER",
    "OTHER",
    "COLONY",
    "UNIT",
    "NATION",
    "DWELLING",
    "TRIBE",
    "STUFF",
    "TILE",
    "MASK",
    "PATH",
    "SEEN",
    "CONNECTIVITY",
    "unknown_map38c2",
    "unknown_map38c3",
    "strategy",
    "unknown_map38d",
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
  CONV_FROM_FIELD( "HEADER", header );
  CONV_FROM_FIELD( "PLAYER", player );
  CONV_FROM_FIELD( "OTHER", other );
  CONV_FROM_FIELD( "COLONY", colony );
  CONV_FROM_FIELD( "UNIT", unit );
  CONV_FROM_FIELD( "NATION", nation );
  CONV_FROM_FIELD( "DWELLING", dwelling );
  CONV_FROM_FIELD( "TRIBE", tribe );
  CONV_FROM_FIELD( "STUFF", stuff );
  CONV_FROM_FIELD( "TILE", tile );
  CONV_FROM_FIELD( "MASK", mask );
  CONV_FROM_FIELD( "PATH", path );
  CONV_FROM_FIELD( "SEEN", seen );
  CONV_FROM_FIELD( "CONNECTIVITY", connectivity );
  CONV_FROM_FIELD( "unknown_map38c2", unknown_map38c2 );
  CONV_FROM_FIELD( "unknown_map38c3", unknown_map38c3 );
  CONV_FROM_FIELD( "strategy", strategy );
  CONV_FROM_FIELD( "unknown_map38d", unknown_map38d );
  CONV_FROM_FIELD( "prime_resource_seed", prime_resource_seed );
  CONV_FROM_FIELD( "unknown39d", unknown39d );
  CONV_FROM_FIELD( "TRADE_ROUTE", trade_route );
  HAS_VALUE_OR_RET( conv.end_field_tracking( tbl, used_keys ) );
  return res;
}

}  // namespace sav
