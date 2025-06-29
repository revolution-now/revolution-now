/****************************************************************
** Classic Colonization Save File Structure.
*****************************************************************/
// NOTE: this file was auto-generated. DO NOT MODIFY!
#pragma once

// sav
#include "sav/bits.hpp"
#include "sav/bytes.hpp"
#include "sav/string.hpp"

// cdr
#include "cdr/ext.hpp"

// base
#include "base/to-str.hpp"

// C++ standard libary
#include <array>
#include <cstdint>
#include <vector>

/****************************************************************
** Forward Declarations.
*****************************************************************/
namespace base {

struct IBinaryIO;

}  // namespace base

/****************************************************************
** Structure definitions.
*****************************************************************/
namespace sav {

/****************************************************************
** cargo_4bit_type
*****************************************************************/
enum class cargo_4bit_type : uint8_t {
  food    = 0b0000,
  sugar   = 0b0001,
  tobacco = 0b0010,
  cotton  = 0b0011,
  furs    = 0b0100,
  lumber  = 0b0101,
  ore     = 0b0110,
  silver  = 0b0111,
  horses  = 0b1000,
  rum     = 0b1001,
  cigars  = 0b1010,
  cloth   = 0b1011,
  coats   = 0b1100,
  goods   = 0b1101,
  tools   = 0b1110,
  muskets = 0b1111,
};

// String conversion.
void to_str( cargo_4bit_type const& o, std::string& out, base::tag<cargo_4bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         cargo_4bit_type const& o,
                         cdr::tag_t<cargo_4bit_type> );

cdr::result<cargo_4bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<cargo_4bit_type> );

/****************************************************************
** control_type
*****************************************************************/
enum class control_type : uint8_t {
  player    = 0x00,
  ai        = 0x01,
  withdrawn = 0x02,
};

// String conversion.
void to_str( control_type const& o, std::string& out, base::tag<control_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         control_type const& o,
                         cdr::tag_t<control_type> );

cdr::result<control_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<control_type> );

/****************************************************************
** difficulty_type
*****************************************************************/
enum class difficulty_type : uint8_t {
  discoverer   = 0x00,
  explorer     = 0x01,
  conquistador = 0x02,
  governor     = 0x03,
  viceroy      = 0x04,
};

// String conversion.
void to_str( difficulty_type const& o, std::string& out, base::tag<difficulty_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         difficulty_type const& o,
                         cdr::tag_t<difficulty_type> );

cdr::result<difficulty_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<difficulty_type> );

/****************************************************************
** end_of_turn_sign_type
*****************************************************************/
enum class end_of_turn_sign_type : uint16_t {
  not_shown = 0x0000,
  flashing  = 0x0001,
};

// String conversion.
void to_str( end_of_turn_sign_type const& o, std::string& out, base::tag<end_of_turn_sign_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         end_of_turn_sign_type const& o,
                         cdr::tag_t<end_of_turn_sign_type> );

cdr::result<end_of_turn_sign_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<end_of_turn_sign_type> );

/****************************************************************
** fortification_level_type
*****************************************************************/
enum class fortification_level_type : uint8_t {
  none     = 0x00,
  stockade = 0x01,
  fort     = 0x02,
  fortress = 0x03,
};

// String conversion.
void to_str( fortification_level_type const& o, std::string& out, base::tag<fortification_level_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         fortification_level_type const& o,
                         cdr::tag_t<fortification_level_type> );

cdr::result<fortification_level_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<fortification_level_type> );

/****************************************************************
** has_city_1bit_type
*****************************************************************/
enum class has_city_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  c     = 0b1,
};

// String conversion.
void to_str( has_city_1bit_type const& o, std::string& out, base::tag<has_city_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         has_city_1bit_type const& o,
                         cdr::tag_t<has_city_1bit_type> );

cdr::result<has_city_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<has_city_1bit_type> );

/****************************************************************
** has_unit_1bit_type
*****************************************************************/
enum class has_unit_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  u     = 0b1,
};

// String conversion.
void to_str( has_unit_1bit_type const& o, std::string& out, base::tag<has_unit_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         has_unit_1bit_type const& o,
                         cdr::tag_t<has_unit_1bit_type> );

cdr::result<has_unit_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<has_unit_1bit_type> );

/****************************************************************
** hills_river_3bit_type
*****************************************************************/
enum class hills_river_3bit_type : uint8_t {
  empty = 0b000,  // original: "  "
  c     = 0b001,  // original: "^ "
  t     = 0b010,  // original: "~ "
  tc    = 0b011,  // original: "~^"
  qq    = 0b100,  // original: "??"
  cc    = 0b101,  // original: "^^"
  tt    = 0b110,  // original: "~~"
};

// String conversion.
void to_str( hills_river_3bit_type const& o, std::string& out, base::tag<hills_river_3bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         hills_river_3bit_type const& o,
                         cdr::tag_t<hills_river_3bit_type> );

cdr::result<hills_river_3bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<hills_river_3bit_type> );

/****************************************************************
** level_2bit_type
*****************************************************************/
enum class level_2bit_type : uint8_t {
  _0 = 0b00,
  _1 = 0b01,
  _2 = 0b11,
};

// String conversion.
void to_str( level_2bit_type const& o, std::string& out, base::tag<level_2bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         level_2bit_type const& o,
                         cdr::tag_t<level_2bit_type> );

cdr::result<level_2bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<level_2bit_type> );

/****************************************************************
** level_3bit_type
*****************************************************************/
enum class level_3bit_type : uint8_t {
  _0 = 0b000,
  _1 = 0b001,
  _2 = 0b011,
  _3 = 0b111,
};

// String conversion.
void to_str( level_3bit_type const& o, std::string& out, base::tag<level_3bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         level_3bit_type const& o,
                         cdr::tag_t<level_3bit_type> );

cdr::result<level_3bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<level_3bit_type> );

/****************************************************************
** nation_2byte_type
*****************************************************************/
enum class nation_2byte_type : uint16_t {
  england     = 0x0000,
  france      = 0x0001,
  spain       = 0x0002,
  netherlands = 0x0003,
  inca        = 0x0004,
  aztec       = 0x0005,
  arawak      = 0x0006,
  iroquois    = 0x0007,
  cherokee    = 0x0008,
  apache      = 0x0009,
  sioux       = 0x000A,
  tupi        = 0x000B,
  none        = 0xFFFF,
};

// String conversion.
void to_str( nation_2byte_type const& o, std::string& out, base::tag<nation_2byte_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         nation_2byte_type const& o,
                         cdr::tag_t<nation_2byte_type> );

cdr::result<nation_2byte_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_2byte_type> );

/****************************************************************
** nation_4bit_short_type
*****************************************************************/
enum class nation_4bit_short_type : uint8_t {
  en    = 0b0000,
  fr    = 0b0001,
  sp    = 0b0010,
  nl    = 0b0011,
  in    = 0b0100,
  az    = 0b0101,
  aw    = 0b0110,
  ir    = 0b0111,
  ch    = 0b1000,
  ap    = 0b1001,
  si    = 0b1010,
  tu    = 0b1011,
  empty = 0b1111,  // original: "  "
};

// String conversion.
void to_str( nation_4bit_short_type const& o, std::string& out, base::tag<nation_4bit_short_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         nation_4bit_short_type const& o,
                         cdr::tag_t<nation_4bit_short_type> );

cdr::result<nation_4bit_short_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_4bit_short_type> );

/****************************************************************
** nation_4bit_type
*****************************************************************/
enum class nation_4bit_type : uint8_t {
  england     = 0b0000,
  france      = 0b0001,
  spain       = 0b0010,
  netherlands = 0b0011,
  inca        = 0b0100,
  aztec       = 0b0101,
  arawak      = 0b0110,
  iroquois    = 0b0111,
  cherokee    = 0b1000,
  apache      = 0b1001,
  sioux       = 0b1010,
  tupi        = 0b1011,
  none        = 0b1111,
};

// String conversion.
void to_str( nation_4bit_type const& o, std::string& out, base::tag<nation_4bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         nation_4bit_type const& o,
                         cdr::tag_t<nation_4bit_type> );

cdr::result<nation_4bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_4bit_type> );

/****************************************************************
** nation_type
*****************************************************************/
enum class nation_type : uint8_t {
  england     = 0x00,
  france      = 0x01,
  spain       = 0x02,
  netherlands = 0x03,
  inca        = 0x04,
  aztec       = 0x05,
  arawak      = 0x06,
  iroquois    = 0x07,
  cherokee    = 0x08,
  apache      = 0x09,
  sioux       = 0x0A,
  tupi        = 0x0B,
  none        = 0xFF,
};

// String conversion.
void to_str( nation_type const& o, std::string& out, base::tag<nation_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         nation_type const& o,
                         cdr::tag_t<nation_type> );

cdr::result<nation_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<nation_type> );

/****************************************************************
** occupation_type
*****************************************************************/
enum class occupation_type : uint8_t {
  farmer          = 0x00,
  sugar_planter   = 0x01,
  tobacco_planter = 0x02,
  cotton_planter  = 0x03,
  fur_trapper     = 0x04,
  lumberjack      = 0x05,
  ore_miner       = 0x06,
  silver_miner    = 0x07,
  fisherman       = 0x08,
  distiller       = 0x09,
  tobacconist     = 0x0A,
  weaver          = 0x0B,
  fur_trader      = 0x0C,
  carpenter       = 0x0D,
  blacksmith      = 0x0E,
  gunsmith        = 0x0F,
  preacher        = 0x10,
  statesman       = 0x11,
  teacher         = 0x12,
  qqqqqqqqqq      = 0x13,  // original: "??????????"
};

// String conversion.
void to_str( occupation_type const& o, std::string& out, base::tag<occupation_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         occupation_type const& o,
                         cdr::tag_t<occupation_type> );

cdr::result<occupation_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<occupation_type> );

/****************************************************************
** orders_type
*****************************************************************/
enum class orders_type : uint8_t {
  none      = 0x00,
  sentry    = 0x01,
  trading   = 0x02,
  g0to      = 0x03,
  fortify   = 0x05,
  fortified = 0x06,
  plow      = 0x08,
  road      = 0x09,
  unknowna  = 0x0A,
  unknownb  = 0x0B,
  unknownc  = 0x0C,
};

// String conversion.
void to_str( orders_type const& o, std::string& out, base::tag<orders_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         orders_type const& o,
                         cdr::tag_t<orders_type> );

cdr::result<orders_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<orders_type> );

/****************************************************************
** pacific_1bit_type
*****************************************************************/
enum class pacific_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  t     = 0b1,  // original: "~"
};

// String conversion.
void to_str( pacific_1bit_type const& o, std::string& out, base::tag<pacific_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         pacific_1bit_type const& o,
                         cdr::tag_t<pacific_1bit_type> );

cdr::result<pacific_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<pacific_1bit_type> );

/****************************************************************
** plowed_1bit_type
*****************************************************************/
enum class plowed_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  h     = 0b1,  // original: "#"
};

// String conversion.
void to_str( plowed_1bit_type const& o, std::string& out, base::tag<plowed_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         plowed_1bit_type const& o,
                         cdr::tag_t<plowed_1bit_type> );

cdr::result<plowed_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<plowed_1bit_type> );

/****************************************************************
** profession_type
*****************************************************************/
enum class profession_type : uint8_t {
  expert_farmer          = 0x00,
  master_sugar_planter   = 0x01,
  master_tobacco_planter = 0x02,
  master_cotton_planter  = 0x03,
  expert_fur_trapper     = 0x04,
  expert_lumberjack      = 0x05,
  expert_ore_miner       = 0x06,
  expert_silver_miner    = 0x07,
  expert_fisherman       = 0x08,
  master_distiller       = 0x09,
  master_tobacconist     = 0x0A,
  master_weaver          = 0x0B,
  master_fur_trader      = 0x0C,
  master_carpenter       = 0x0D,
  master_blacksmith      = 0x0E,
  master_gunsmith        = 0x0F,
  firebrand_preacher     = 0x10,
  elder_statesman        = 0x11,
  expert_teacher         = 0x12,
  a_free_colonist        = 0x13,  // original: "*(Free colonist)"
  hardy_pioneer          = 0x14,
  veteran_soldier        = 0x15,
  seasoned_scout         = 0x16,
  veteran_dragoon        = 0x17,
  jesuit_missionary      = 0x18,
  indentured_servant     = 0x19,
  petty_criminal         = 0x1A,
  indian_convert         = 0x1B,
  free_colonist          = 0x1C,
};

// String conversion.
void to_str( profession_type const& o, std::string& out, base::tag<profession_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         profession_type const& o,
                         cdr::tag_t<profession_type> );

cdr::result<profession_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<profession_type> );

/****************************************************************
** purchased_1bit_type
*****************************************************************/
enum class purchased_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  a     = 0b1,  // original: "*"
};

// String conversion.
void to_str( purchased_1bit_type const& o, std::string& out, base::tag<purchased_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         purchased_1bit_type const& o,
                         cdr::tag_t<purchased_1bit_type> );

cdr::result<purchased_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<purchased_1bit_type> );

/****************************************************************
** region_id_4bit_type
*****************************************************************/
enum class region_id_4bit_type : uint8_t {
  _0  = 0b0000,
  _1  = 0b0001,
  _2  = 0b0010,
  _3  = 0b0011,
  _4  = 0b0100,
  _5  = 0b0101,
  _6  = 0b0110,
  _7  = 0b0111,
  _8  = 0b1000,
  _9  = 0b1001,
  _10 = 0b1010,
  _11 = 0b1011,
  _12 = 0b1100,
  _13 = 0b1101,
  _14 = 0b1110,
  _15 = 0b1111,
};

// String conversion.
void to_str( region_id_4bit_type const& o, std::string& out, base::tag<region_id_4bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         region_id_4bit_type const& o,
                         cdr::tag_t<region_id_4bit_type> );

cdr::result<region_id_4bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<region_id_4bit_type> );

/****************************************************************
** relation_3bit_type
*****************************************************************/
enum class relation_3bit_type : uint8_t {
  self_vanished_not_met     = 0b000,  // original: "self/vanished/not met"
  war                       = 0b010,
  post_granted_independence = 0b100,
  peace                     = 0b110,
};

// String conversion.
void to_str( relation_3bit_type const& o, std::string& out, base::tag<relation_3bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         relation_3bit_type const& o,
                         cdr::tag_t<relation_3bit_type> );

cdr::result<relation_3bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<relation_3bit_type> );

/****************************************************************
** road_1bit_type
*****************************************************************/
enum class road_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  e     = 0b1,  // original: "="
};

// String conversion.
void to_str( road_1bit_type const& o, std::string& out, base::tag<road_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         road_1bit_type const& o,
                         cdr::tag_t<road_1bit_type> );

cdr::result<road_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<road_1bit_type> );

/****************************************************************
** season_type
*****************************************************************/
enum class season_type : uint16_t {
  spring = 0x0000,
  autumn = 0x0001,
};

// String conversion.
void to_str( season_type const& o, std::string& out, base::tag<season_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         season_type const& o,
                         cdr::tag_t<season_type> );

cdr::result<season_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<season_type> );

/****************************************************************
** suppress_1bit_type
*****************************************************************/
enum class suppress_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  _     = 0b1,
};

// String conversion.
void to_str( suppress_1bit_type const& o, std::string& out, base::tag<suppress_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         suppress_1bit_type const& o,
                         cdr::tag_t<suppress_1bit_type> );

cdr::result<suppress_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<suppress_1bit_type> );

/****************************************************************
** tech_type
*****************************************************************/
enum class tech_type : uint8_t {
  semi_nomadic = 0x00,
  agrarian     = 0x01,
  advanced     = 0x02,
  civilized    = 0x03,
};

// String conversion.
void to_str( tech_type const& o, std::string& out, base::tag<tech_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         tech_type const& o,
                         cdr::tag_t<tech_type> );

cdr::result<tech_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<tech_type> );

/****************************************************************
** terrain_5bit_type
*****************************************************************/
enum class terrain_5bit_type : uint8_t {
  tu  = 0b00000,  // original: "tu "
  de  = 0b00001,  // original: "de "
  pl  = 0b00010,  // original: "pl "
  pr  = 0b00011,  // original: "pr "
  gr  = 0b00100,  // original: "gr "
  sa  = 0b00101,  // original: "sa "
  mr  = 0b00110,  // original: "mr "
  sw  = 0b00111,  // original: "sw "
  tuf = 0b01000,
  def = 0b01001,
  plf = 0b01010,
  prf = 0b01011,
  grf = 0b01100,
  saf = 0b01101,
  mrf = 0b01110,
  swf = 0b01111,
  tuw = 0b10000,
  dew = 0b10001,
  plw = 0b10010,
  prw = 0b10011,
  grw = 0b10100,
  saw = 0b10101,
  mrw = 0b10110,
  sww = 0b10111,
  arc = 0b11000,
  ttt = 0b11001,  // original: "~~~"
  tnt = 0b11010,  // original: "~:~"
};

// String conversion.
void to_str( terrain_5bit_type const& o, std::string& out, base::tag<terrain_5bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         terrain_5bit_type const& o,
                         cdr::tag_t<terrain_5bit_type> );

cdr::result<terrain_5bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<terrain_5bit_type> );

/****************************************************************
** trade_route_type
*****************************************************************/
enum class trade_route_type : uint8_t {
  land = 0x00,
  sea  = 0x01,
};

// String conversion.
void to_str( trade_route_type const& o, std::string& out, base::tag<trade_route_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         trade_route_type const& o,
                         cdr::tag_t<trade_route_type> );

cdr::result<trade_route_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<trade_route_type> );

/****************************************************************
** unit_type
*****************************************************************/
enum class unit_type : uint8_t {
  colonist            = 0x00,
  soldier             = 0x01,
  pioneer             = 0x02,
  missionary          = 0x03,
  dragoon             = 0x04,
  scout               = 0x05,
  tory_regular        = 0x06,
  continental_cavalry = 0x07,
  tory_cavalry        = 0x08,
  continental_army    = 0x09,
  treasure            = 0x0A,
  artillery           = 0x0B,
  wagon_train         = 0x0C,
  caravel             = 0x0D,
  merchantman         = 0x0E,
  galleon             = 0x0F,
  privateer           = 0x10,
  frigate             = 0x11,
  man_o_war           = 0x12,
  brave               = 0x13,
  armed_brave         = 0x14,
  mounted_brave       = 0x15,
  mounted_warrior     = 0x16,
};

// String conversion.
void to_str( unit_type const& o, std::string& out, base::tag<unit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         unit_type const& o,
                         cdr::tag_t<unit_type> );

cdr::result<unit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<unit_type> );

/****************************************************************
** visible_to_dutch_1bit_type
*****************************************************************/
enum class visible_to_dutch_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  d     = 0b1,
};

// String conversion.
void to_str( visible_to_dutch_1bit_type const& o, std::string& out, base::tag<visible_to_dutch_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         visible_to_dutch_1bit_type const& o,
                         cdr::tag_t<visible_to_dutch_1bit_type> );

cdr::result<visible_to_dutch_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_dutch_1bit_type> );

/****************************************************************
** visible_to_english_1bit_type
*****************************************************************/
enum class visible_to_english_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  e     = 0b1,
};

// String conversion.
void to_str( visible_to_english_1bit_type const& o, std::string& out, base::tag<visible_to_english_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         visible_to_english_1bit_type const& o,
                         cdr::tag_t<visible_to_english_1bit_type> );

cdr::result<visible_to_english_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_english_1bit_type> );

/****************************************************************
** visible_to_french_1bit_type
*****************************************************************/
enum class visible_to_french_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  f     = 0b1,
};

// String conversion.
void to_str( visible_to_french_1bit_type const& o, std::string& out, base::tag<visible_to_french_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         visible_to_french_1bit_type const& o,
                         cdr::tag_t<visible_to_french_1bit_type> );

cdr::result<visible_to_french_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_french_1bit_type> );

/****************************************************************
** visible_to_spanish_1bit_type
*****************************************************************/
enum class visible_to_spanish_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  s     = 0b1,
};

// String conversion.
void to_str( visible_to_spanish_1bit_type const& o, std::string& out, base::tag<visible_to_spanish_1bit_type> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         visible_to_spanish_1bit_type const& o,
                         cdr::tag_t<visible_to_spanish_1bit_type> );

cdr::result<visible_to_spanish_1bit_type> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<visible_to_spanish_1bit_type> );

/****************************************************************
** yes_no_byte
*****************************************************************/
enum class yes_no_byte : uint8_t {
  no  = 0x00,
  yes = 0x01,
};

// String conversion.
void to_str( yes_no_byte const& o, std::string& out, base::tag<yes_no_byte> );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         yes_no_byte const& o,
                         cdr::tag_t<yes_no_byte> );

cdr::result<yes_no_byte> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<yes_no_byte> );

/****************************************************************
** TutorialHelp
*****************************************************************/
struct TutorialHelp {
  bool hint_pioneer : 1 = {};
  bool hint_soldier : 1 = {};
  bool unknown02 : 1 = {};
  bool hint_new_colonist_in_colony : 1 = {};
  bool hint_food_deficit : 1 = {};
  bool hint_harbor : 1 = {};
  bool unknown06 : 1 = {};
  bool hint_native_convert : 1 = {};

  bool operator==( TutorialHelp const& ) const = default;
};

// String conversion.
void to_str( TutorialHelp const& o, std::string& out, base::tag<TutorialHelp> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TutorialHelp& o );

bool write_binary( base::IBinaryIO& b, TutorialHelp const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TutorialHelp const& o,
                         cdr::tag_t<TutorialHelp> );

cdr::result<TutorialHelp> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TutorialHelp> );

/****************************************************************
** GameFlags1
*****************************************************************/
struct GameFlags1 {
  bool independence_declared : 1 = {};
  bool deploy_intervention_force : 1 = {};
  bool independence_war_intro : 1 = {};
  bool won_independence : 1 = {};
  bool score_sequence_done : 1 = {};
  bool ref_will_forfeight : 1 = {};
  bool ref_captured_colony : 1 = {};
  bool tutorial_hints : 1 = {};
  bool disable_water_color_cycling : 1 = {};
  bool combat_analysis : 1 = {};
  bool autosave : 1 = {};
  bool end_of_turn : 1 = {};
  bool fast_piece_slide : 1 = {};
  bool cheats_enabled : 1 = {};
  bool show_foreign_moves : 1 = {};
  bool show_indian_moves : 1 = {};

  bool operator==( GameFlags1 const& ) const = default;
};

// String conversion.
void to_str( GameFlags1 const& o, std::string& out, base::tag<GameFlags1> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, GameFlags1& o );

bool write_binary( base::IBinaryIO& b, GameFlags1 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         GameFlags1 const& o,
                         cdr::tag_t<GameFlags1> );

cdr::result<GameFlags1> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<GameFlags1> );

/****************************************************************
** ColonyReportOptionsToDisable
*****************************************************************/
struct ColonyReportOptionsToDisable {
  bool labels_on_cargo_and_terrain : 1 = {};
  bool labels_on_buildings : 1 = {};
  bool report_new_cargos_available : 1 = {};
  bool report_inefficient_government : 1 = {};
  bool report_tools_needed_for_production : 1 = {};
  bool report_raw_materials_shortages : 1 = {};
  bool report_food_shortages : 1 = {};
  bool report_when_colonists_trained : 1 = {};
  bool report_sons_of_liberty_membership : 1 = {};
  bool report_rebel_majorities : 1 = {};
  uint8_t unused03 : 6 = {};

  bool operator==( ColonyReportOptionsToDisable const& ) const = default;
};

// String conversion.
void to_str( ColonyReportOptionsToDisable const& o, std::string& out, base::tag<ColonyReportOptionsToDisable> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ColonyReportOptionsToDisable& o );

bool write_binary( base::IBinaryIO& b, ColonyReportOptionsToDisable const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         ColonyReportOptionsToDisable const& o,
                         cdr::tag_t<ColonyReportOptionsToDisable> );

cdr::result<ColonyReportOptionsToDisable> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonyReportOptionsToDisable> );

/****************************************************************
** GameFlags2
*****************************************************************/
struct GameFlags2 {
  bool how_to_win : 1 = {};
  bool background_music : 1 = {};
  bool event_music : 1 = {};
  bool sound_effects : 1 = {};
  bool hint_how_to_move_ship : 1 = {};
  bool unknown_hint01 : 1 = {};
  bool hint_lumber_abundance : 1 = {};
  bool hint_colony_view : 1 = {};
  bool hint_dock_units_waiting : 1 = {};
  bool hint_full_cargo : 1 = {};
  bool hint_build_stockade : 1 = {};
  bool hint_free_colonist : 1 = {};
  bool unknown_hint08 : 1 = {};
  bool unknown_hint09 : 1 = {};
  bool hint_ship_valuable : 1 = {};
  bool hint_ship_in_colony : 1 = {};

  bool operator==( GameFlags2 const& ) const = default;
};

// String conversion.
void to_str( GameFlags2 const& o, std::string& out, base::tag<GameFlags2> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, GameFlags2& o );

bool write_binary( base::IBinaryIO& b, GameFlags2 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         GameFlags2 const& o,
                         cdr::tag_t<GameFlags2> );

cdr::result<GameFlags2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<GameFlags2> );

/****************************************************************
** Event
*****************************************************************/
struct Event {
  bool discovery_of_the_new_world : 1 = {};
  bool building_a_colony : 1 = {};
  bool meeting_the_natives : 1 = {};
  bool the_aztec_empire : 1 = {};
  bool the_inca_nation : 1 = {};
  bool discovery_of_the_pacific_ocean : 1 = {};
  bool entering_indian_village : 1 = {};
  bool the_fountain_of_youth : 1 = {};
  bool cargo_from_the_new_world : 1 = {};
  bool meeting_fellow_europeans : 1 = {};
  bool colony_burning : 1 = {};
  bool colony_destroyed : 1 = {};
  bool indian_raid : 1 = {};
  bool woodcut14 : 1 = {};
  bool woodcut15 : 1 = {};
  bool woodcut16 : 1 = {};

  bool operator==( Event const& ) const = default;
};

// String conversion.
void to_str( Event const& o, std::string& out, base::tag<Event> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Event& o );

bool write_binary( base::IBinaryIO& b, Event const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Event const& o,
                         cdr::tag_t<Event> );

cdr::result<Event> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Event> );

/****************************************************************
** PlayerFlags
*****************************************************************/
struct PlayerFlags {
  uint8_t unknown06a : 7 = {};
  bool named_new_world : 1 = {};

  bool operator==( PlayerFlags const& ) const = default;
};

// String conversion.
void to_str( PlayerFlags const& o, std::string& out, base::tag<PlayerFlags> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PlayerFlags& o );

bool write_binary( base::IBinaryIO& b, PlayerFlags const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         PlayerFlags const& o,
                         cdr::tag_t<PlayerFlags> );

cdr::result<PlayerFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PlayerFlags> );

/****************************************************************
** ColonyFlags
*****************************************************************/
struct ColonyFlags {
  bool unknown00 : 1 = {};
  bool level2_sol_bonus : 1 = {};
  bool level1_sol_bonus : 1 = {};
  bool inefficient_govt_notified : 1 = {};
  bool unknown04 : 1 = {};
  bool unknown05 : 1 = {};
  bool port_colony : 1 = {};
  bool construction_complete_blinking : 1 = {};

  bool operator==( ColonyFlags const& ) const = default;
};

// String conversion.
void to_str( ColonyFlags const& o, std::string& out, base::tag<ColonyFlags> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ColonyFlags& o );

bool write_binary( base::IBinaryIO& b, ColonyFlags const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         ColonyFlags const& o,
                         cdr::tag_t<ColonyFlags> );

cdr::result<ColonyFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonyFlags> );

/****************************************************************
** Duration
*****************************************************************/
struct Duration {
  uint8_t dur_1 : 4 = {};
  uint8_t dur_2 : 4 = {};

  bool operator==( Duration const& ) const = default;
};

// String conversion.
void to_str( Duration const& o, std::string& out, base::tag<Duration> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Duration& o );

bool write_binary( base::IBinaryIO& b, Duration const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Duration const& o,
                         cdr::tag_t<Duration> );

cdr::result<Duration> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Duration> );

/****************************************************************
** Buildings
*****************************************************************/
struct Buildings {
  level_3bit_type fortification : 3 = {};
  level_3bit_type armory : 3 = {};
  level_3bit_type docks : 3 = {};
  level_3bit_type town_hall : 3 = {};
  level_3bit_type schoolhouse : 3 = {};
  bool warehouse : 1 = {};
  bool unused05a : 1 = {};
  bool stables : 1 = {};
  bool custom_house : 1 = {};
  level_2bit_type printing_press : 2 = {};
  level_3bit_type weavers_house : 3 = {};
  level_3bit_type tobacconists_house : 3 = {};
  level_3bit_type rum_distillers_house : 3 = {};
  level_2bit_type capitol_unused : 2 = {};
  level_3bit_type fur_traders_house : 3 = {};
  level_2bit_type carpenters_shop : 2 = {};
  level_2bit_type church : 2 = {};
  level_3bit_type blacksmiths_house : 3 = {};
  uint8_t unused05b : 6 = {};

  bool operator==( Buildings const& ) const = default;
};

// String conversion.
void to_str( Buildings const& o, std::string& out, base::tag<Buildings> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Buildings& o );

bool write_binary( base::IBinaryIO& b, Buildings const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Buildings const& o,
                         cdr::tag_t<Buildings> );

cdr::result<Buildings> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Buildings> );

/****************************************************************
** CustomHouseFlags
*****************************************************************/
struct CustomHouseFlags {
  bool food : 1 = {};
  bool sugar : 1 = {};
  bool tobacco : 1 = {};
  bool cotton : 1 = {};
  bool furs : 1 = {};
  bool lumber : 1 = {};
  bool ore : 1 = {};
  bool silver : 1 = {};
  bool horses : 1 = {};
  bool rum : 1 = {};
  bool cigars : 1 = {};
  bool cloth : 1 = {};
  bool coats : 1 = {};
  bool trade_goods : 1 = {};
  bool tools : 1 = {};
  bool muskets : 1 = {};

  bool operator==( CustomHouseFlags const& ) const = default;
};

// String conversion.
void to_str( CustomHouseFlags const& o, std::string& out, base::tag<CustomHouseFlags> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CustomHouseFlags& o );

bool write_binary( base::IBinaryIO& b, CustomHouseFlags const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         CustomHouseFlags const& o,
                         cdr::tag_t<CustomHouseFlags> );

cdr::result<CustomHouseFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CustomHouseFlags> );

/****************************************************************
** NationInfo
*****************************************************************/
struct NationInfo {
  nation_4bit_type nation_id : 4 = {};
  bool vis_to_english : 1 = {};
  bool vis_to_french : 1 = {};
  bool vis_to_spanish : 1 = {};
  bool vis_to_dutch : 1 = {};

  bool operator==( NationInfo const& ) const = default;
};

// String conversion.
void to_str( NationInfo const& o, std::string& out, base::tag<NationInfo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationInfo& o );

bool write_binary( base::IBinaryIO& b, NationInfo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         NationInfo const& o,
                         cdr::tag_t<NationInfo> );

cdr::result<NationInfo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationInfo> );

/****************************************************************
** Unknown15
*****************************************************************/
struct Unknown15 {
  uint8_t unknown15a : 7 = {};
  bool damaged : 1 = {};

  bool operator==( Unknown15 const& ) const = default;
};

// String conversion.
void to_str( Unknown15 const& o, std::string& out, base::tag<Unknown15> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown15& o );

bool write_binary( base::IBinaryIO& b, Unknown15 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Unknown15 const& o,
                         cdr::tag_t<Unknown15> );

cdr::result<Unknown15> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown15> );

/****************************************************************
** CargoItems
*****************************************************************/
struct CargoItems {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};

  bool operator==( CargoItems const& ) const = default;
};

// String conversion.
void to_str( CargoItems const& o, std::string& out, base::tag<CargoItems> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CargoItems& o );

bool write_binary( base::IBinaryIO& b, CargoItems const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         CargoItems const& o,
                         cdr::tag_t<CargoItems> );

cdr::result<CargoItems> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CargoItems> );

/****************************************************************
** NationFlags
*****************************************************************/
struct NationFlags {
  uint8_t unknown19a : 2 = {};
  bool granted_independence : 1 = {};
  bool promoted_continental_units : 1 = {};
  uint8_t unknown19b : 2 = {};
  bool immigration_started : 1 = {};
  uint8_t unknown19c : 1 = {};

  bool operator==( NationFlags const& ) const = default;
};

// String conversion.
void to_str( NationFlags const& o, std::string& out, base::tag<NationFlags> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationFlags& o );

bool write_binary( base::IBinaryIO& b, NationFlags const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         NationFlags const& o,
                         cdr::tag_t<NationFlags> );

cdr::result<NationFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationFlags> );

/****************************************************************
** FoundingFathers
*****************************************************************/
struct FoundingFathers {
  bool adam_smith : 1 = {};
  bool jakob_fugger : 1 = {};
  bool peter_minuit : 1 = {};
  bool peter_stuyvesant : 1 = {};
  bool jan_de_witt : 1 = {};
  bool ferdinand_magellan : 1 = {};
  bool francisco_coronado : 1 = {};
  bool hernando_de_soto : 1 = {};
  bool henry_hudson : 1 = {};
  bool sieur_de_la_salle : 1 = {};
  bool hernan_cortes : 1 = {};
  bool george_washington : 1 = {};
  bool paul_revere : 1 = {};
  bool francis_drake : 1 = {};
  bool john_paul_jones : 1 = {};
  bool thomas_jefferson : 1 = {};
  bool pocahontas : 1 = {};
  bool thomas_paine : 1 = {};
  bool simon_bolivar : 1 = {};
  bool benjamin_franklin : 1 = {};
  bool william_brewster : 1 = {};
  bool william_penn : 1 = {};
  bool jean_de_brebeuf : 1 = {};
  bool juan_de_sepulveda : 1 = {};
  bool bartolme_de_las_casas : 1 = {};
  uint8_t unknown00 : 7 = {};

  bool operator==( FoundingFathers const& ) const = default;
};

// String conversion.
void to_str( FoundingFathers const& o, std::string& out, base::tag<FoundingFathers> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, FoundingFathers& o );

bool write_binary( base::IBinaryIO& b, FoundingFathers const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         FoundingFathers const& o,
                         cdr::tag_t<FoundingFathers> );

cdr::result<FoundingFathers> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<FoundingFathers> );

/****************************************************************
** BoycottBitmap
*****************************************************************/
struct BoycottBitmap {
  bool food : 1 = {};
  bool sugar : 1 = {};
  bool tobacco : 1 = {};
  bool cotton : 1 = {};
  bool furs : 1 = {};
  bool lumber : 1 = {};
  bool ore : 1 = {};
  bool silver : 1 = {};
  bool horses : 1 = {};
  bool rum : 1 = {};
  bool cigars : 1 = {};
  bool cloth : 1 = {};
  bool coats : 1 = {};
  bool trade_goods : 1 = {};
  bool tools : 1 = {};
  bool muskets : 1 = {};

  bool operator==( BoycottBitmap const& ) const = default;
};

// String conversion.
void to_str( BoycottBitmap const& o, std::string& out, base::tag<BoycottBitmap> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BoycottBitmap& o );

bool write_binary( base::IBinaryIO& b, BoycottBitmap const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         BoycottBitmap const& o,
                         cdr::tag_t<BoycottBitmap> );

cdr::result<BoycottBitmap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BoycottBitmap> );

/****************************************************************
** RelationByNations
*****************************************************************/
struct RelationByNations {
  uint8_t attitudeq : 4 = {};
  relation_3bit_type status : 3 = {};
  bool irritated_by_piracy : 1 = {};

  bool operator==( RelationByNations const& ) const = default;
};

// String conversion.
void to_str( RelationByNations const& o, std::string& out, base::tag<RelationByNations> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByNations& o );

bool write_binary( base::IBinaryIO& b, RelationByNations const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         RelationByNations const& o,
                         cdr::tag_t<RelationByNations> );

cdr::result<RelationByNations> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByNations> );

/****************************************************************
** RelationByIndian
*****************************************************************/
struct RelationByIndian {
  uint8_t attitudeq : 4 = {};
  relation_3bit_type status : 3 = {};
  bool unused : 1 = {};

  bool operator==( RelationByIndian const& ) const = default;
};

// String conversion.
void to_str( RelationByIndian const& o, std::string& out, base::tag<RelationByIndian> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByIndian& o );

bool write_binary( base::IBinaryIO& b, RelationByIndian const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         RelationByIndian const& o,
                         cdr::tag_t<RelationByIndian> );

cdr::result<RelationByIndian> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByIndian> );

/****************************************************************
** BLCS
*****************************************************************/
struct BLCS {
  bool brave_missing : 1 = {};
  bool learned : 1 = {};
  bool capital : 1 = {};
  bool scouted : 1 = {};
  uint8_t unused09 : 4 = {};

  bool operator==( BLCS const& ) const = default;
};

// String conversion.
void to_str( BLCS const& o, std::string& out, base::tag<BLCS> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BLCS& o );

bool write_binary( base::IBinaryIO& b, BLCS const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         BLCS const& o,
                         cdr::tag_t<BLCS> );

cdr::result<BLCS> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BLCS> );

/****************************************************************
** Mission
*****************************************************************/
struct Mission {
  nation_4bit_type nation_id : 4 = {};
  bool expert : 1 = {};
  uint8_t unknown : 3 = {};

  bool operator==( Mission const& ) const = default;
};

// String conversion.
void to_str( Mission const& o, std::string& out, base::tag<Mission> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Mission& o );

bool write_binary( base::IBinaryIO& b, Mission const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Mission const& o,
                         cdr::tag_t<Mission> );

cdr::result<Mission> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Mission> );

/****************************************************************
** TribeFlags
*****************************************************************/
struct TribeFlags {
  uint8_t unknown01 : 5 = {};
  bool joined_ref : 1 = {};
  uint8_t unknown02 : 1 = {};
  bool extinct : 1 = {};

  bool operator==( TribeFlags const& ) const = default;
};

// String conversion.
void to_str( TribeFlags const& o, std::string& out, base::tag<TribeFlags> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TribeFlags& o );

bool write_binary( base::IBinaryIO& b, TribeFlags const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TribeFlags const& o,
                         cdr::tag_t<TribeFlags> );

cdr::result<TribeFlags> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TribeFlags> );

/****************************************************************
** RelationByNations2
*****************************************************************/
struct RelationByNations2 {
  uint8_t attitudeq : 4 = {};
  relation_3bit_type status : 3 = {};
  bool unused : 1 = {};

  bool operator==( RelationByNations2 const& ) const = default;
};

// String conversion.
void to_str( RelationByNations2 const& o, std::string& out, base::tag<RelationByNations2> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, RelationByNations2& o );

bool write_binary( base::IBinaryIO& b, RelationByNations2 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         RelationByNations2 const& o,
                         cdr::tag_t<RelationByNations2> );

cdr::result<RelationByNations2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<RelationByNations2> );

/****************************************************************
** TILE
*****************************************************************/
struct TILE {
  terrain_5bit_type tile : 5 = {};
  hills_river_3bit_type hill_river : 3 = {};

  bool operator==( TILE const& ) const = default;
};

// String conversion.
void to_str( TILE const& o, std::string& out, base::tag<TILE> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TILE& o );

bool write_binary( base::IBinaryIO& b, TILE const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TILE const& o,
                         cdr::tag_t<TILE> );

cdr::result<TILE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TILE> );

/****************************************************************
** MASK
*****************************************************************/
struct MASK {
  has_unit_1bit_type has_unit : 1 = {};
  has_city_1bit_type has_city : 1 = {};
  suppress_1bit_type suppress : 1 = {};
  road_1bit_type road : 1 = {};
  purchased_1bit_type purchased : 1 = {};
  pacific_1bit_type pacific : 1 = {};
  plowed_1bit_type plowed : 1 = {};
  suppress_1bit_type unused : 1 = {};

  bool operator==( MASK const& ) const = default;
};

// String conversion.
void to_str( MASK const& o, std::string& out, base::tag<MASK> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, MASK& o );

bool write_binary( base::IBinaryIO& b, MASK const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         MASK const& o,
                         cdr::tag_t<MASK> );

cdr::result<MASK> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<MASK> );

/****************************************************************
** PATH
*****************************************************************/
struct PATH {
  region_id_4bit_type region_id : 4 = {};
  nation_4bit_short_type visitor_nation : 4 = {};

  bool operator==( PATH const& ) const = default;
};

// String conversion.
void to_str( PATH const& o, std::string& out, base::tag<PATH> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PATH& o );

bool write_binary( base::IBinaryIO& b, PATH const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         PATH const& o,
                         cdr::tag_t<PATH> );

cdr::result<PATH> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PATH> );

/****************************************************************
** SEEN
*****************************************************************/
struct SEEN {
  region_id_4bit_type score : 4 = {};
  visible_to_english_1bit_type vis2en : 1 = {};
  visible_to_french_1bit_type vis2fr : 1 = {};
  visible_to_spanish_1bit_type vis2sp : 1 = {};
  visible_to_dutch_1bit_type vis2du : 1 = {};

  bool operator==( SEEN const& ) const = default;
};

// String conversion.
void to_str( SEEN const& o, std::string& out, base::tag<SEEN> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, SEEN& o );

bool write_binary( base::IBinaryIO& b, SEEN const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         SEEN const& o,
                         cdr::tag_t<SEEN> );

cdr::result<SEEN> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<SEEN> );

/****************************************************************
** SeaLaneConnectivity
*****************************************************************/
struct SeaLaneConnectivity {
  bool north : 1 = {};
  bool neast : 1 = {};
  bool east : 1 = {};
  bool seast : 1 = {};
  bool south : 1 = {};
  bool swest : 1 = {};
  bool west : 1 = {};
  bool nwest : 1 = {};

  bool operator==( SeaLaneConnectivity const& ) const = default;
};

// String conversion.
void to_str( SeaLaneConnectivity const& o, std::string& out, base::tag<SeaLaneConnectivity> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, SeaLaneConnectivity& o );

bool write_binary( base::IBinaryIO& b, SeaLaneConnectivity const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         SeaLaneConnectivity const& o,
                         cdr::tag_t<SeaLaneConnectivity> );

cdr::result<SeaLaneConnectivity> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<SeaLaneConnectivity> );

/****************************************************************
** LandConnectivity
*****************************************************************/
struct LandConnectivity {
  bool north : 1 = {};
  bool neast : 1 = {};
  bool east : 1 = {};
  bool seast : 1 = {};
  bool south : 1 = {};
  bool swest : 1 = {};
  bool west : 1 = {};
  bool nwest : 1 = {};

  bool operator==( LandConnectivity const& ) const = default;
};

// String conversion.
void to_str( LandConnectivity const& o, std::string& out, base::tag<LandConnectivity> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, LandConnectivity& o );

bool write_binary( base::IBinaryIO& b, LandConnectivity const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         LandConnectivity const& o,
                         cdr::tag_t<LandConnectivity> );

cdr::result<LandConnectivity> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<LandConnectivity> );

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
struct Stop1LoadsAndUnloadsCount {
  uint8_t unloads_count : 4 = {};
  uint8_t loads_count : 4 = {};

  bool operator==( Stop1LoadsAndUnloadsCount const& ) const = default;
};

// String conversion.
void to_str( Stop1LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop1LoadsAndUnloadsCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1LoadsAndUnloadsCount& o );

bool write_binary( base::IBinaryIO& b, Stop1LoadsAndUnloadsCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop1LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop1LoadsAndUnloadsCount> );

cdr::result<Stop1LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1LoadsAndUnloadsCount> );

/****************************************************************
** Stop1LoadsCargo
*****************************************************************/
struct Stop1LoadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop1LoadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop1LoadsCargo const& o, std::string& out, base::tag<Stop1LoadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1LoadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop1LoadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop1LoadsCargo const& o,
                         cdr::tag_t<Stop1LoadsCargo> );

cdr::result<Stop1LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1LoadsCargo> );

/****************************************************************
** Stop1UnloadsCargo
*****************************************************************/
struct Stop1UnloadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop1UnloadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop1UnloadsCargo const& o, std::string& out, base::tag<Stop1UnloadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop1UnloadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop1UnloadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop1UnloadsCargo const& o,
                         cdr::tag_t<Stop1UnloadsCargo> );

cdr::result<Stop1UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop1UnloadsCargo> );

/****************************************************************
** Stop2LoadsAndUnloadsCount
*****************************************************************/
struct Stop2LoadsAndUnloadsCount {
  uint8_t unloads_count : 4 = {};
  uint8_t loads_count : 4 = {};

  bool operator==( Stop2LoadsAndUnloadsCount const& ) const = default;
};

// String conversion.
void to_str( Stop2LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop2LoadsAndUnloadsCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2LoadsAndUnloadsCount& o );

bool write_binary( base::IBinaryIO& b, Stop2LoadsAndUnloadsCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop2LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop2LoadsAndUnloadsCount> );

cdr::result<Stop2LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2LoadsAndUnloadsCount> );

/****************************************************************
** Stop2LoadsCargo
*****************************************************************/
struct Stop2LoadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop2LoadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop2LoadsCargo const& o, std::string& out, base::tag<Stop2LoadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2LoadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop2LoadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop2LoadsCargo const& o,
                         cdr::tag_t<Stop2LoadsCargo> );

cdr::result<Stop2LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2LoadsCargo> );

/****************************************************************
** Stop2UnloadsCargo
*****************************************************************/
struct Stop2UnloadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop2UnloadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop2UnloadsCargo const& o, std::string& out, base::tag<Stop2UnloadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop2UnloadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop2UnloadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop2UnloadsCargo const& o,
                         cdr::tag_t<Stop2UnloadsCargo> );

cdr::result<Stop2UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop2UnloadsCargo> );

/****************************************************************
** Stop3LoadsAndUnloadsCount
*****************************************************************/
struct Stop3LoadsAndUnloadsCount {
  uint8_t unloads_count : 4 = {};
  uint8_t loads_count : 4 = {};

  bool operator==( Stop3LoadsAndUnloadsCount const& ) const = default;
};

// String conversion.
void to_str( Stop3LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop3LoadsAndUnloadsCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3LoadsAndUnloadsCount& o );

bool write_binary( base::IBinaryIO& b, Stop3LoadsAndUnloadsCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop3LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop3LoadsAndUnloadsCount> );

cdr::result<Stop3LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3LoadsAndUnloadsCount> );

/****************************************************************
** Stop3LoadsCargo
*****************************************************************/
struct Stop3LoadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop3LoadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop3LoadsCargo const& o, std::string& out, base::tag<Stop3LoadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3LoadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop3LoadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop3LoadsCargo const& o,
                         cdr::tag_t<Stop3LoadsCargo> );

cdr::result<Stop3LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3LoadsCargo> );

/****************************************************************
** Stop3UnloadsCargo
*****************************************************************/
struct Stop3UnloadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop3UnloadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop3UnloadsCargo const& o, std::string& out, base::tag<Stop3UnloadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop3UnloadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop3UnloadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop3UnloadsCargo const& o,
                         cdr::tag_t<Stop3UnloadsCargo> );

cdr::result<Stop3UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop3UnloadsCargo> );

/****************************************************************
** Stop4LoadsAndUnloadsCount
*****************************************************************/
struct Stop4LoadsAndUnloadsCount {
  uint8_t unloads_count : 4 = {};
  uint8_t loads_count : 4 = {};

  bool operator==( Stop4LoadsAndUnloadsCount const& ) const = default;
};

// String conversion.
void to_str( Stop4LoadsAndUnloadsCount const& o, std::string& out, base::tag<Stop4LoadsAndUnloadsCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4LoadsAndUnloadsCount& o );

bool write_binary( base::IBinaryIO& b, Stop4LoadsAndUnloadsCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop4LoadsAndUnloadsCount const& o,
                         cdr::tag_t<Stop4LoadsAndUnloadsCount> );

cdr::result<Stop4LoadsAndUnloadsCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4LoadsAndUnloadsCount> );

/****************************************************************
** Stop4LoadsCargo
*****************************************************************/
struct Stop4LoadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop4LoadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop4LoadsCargo const& o, std::string& out, base::tag<Stop4LoadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4LoadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop4LoadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop4LoadsCargo const& o,
                         cdr::tag_t<Stop4LoadsCargo> );

cdr::result<Stop4LoadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4LoadsCargo> );

/****************************************************************
** Stop4UnloadsCargo
*****************************************************************/
struct Stop4UnloadsCargo {
  cargo_4bit_type cargo_1 : 4 = {};
  cargo_4bit_type cargo_2 : 4 = {};
  cargo_4bit_type cargo_3 : 4 = {};
  cargo_4bit_type cargo_4 : 4 = {};
  cargo_4bit_type cargo_5 : 4 = {};
  cargo_4bit_type cargo_6 : 4 = {};

  bool operator==( Stop4UnloadsCargo const& ) const = default;
};

// String conversion.
void to_str( Stop4UnloadsCargo const& o, std::string& out, base::tag<Stop4UnloadsCargo> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stop4UnloadsCargo& o );

bool write_binary( base::IBinaryIO& b, Stop4UnloadsCargo const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stop4UnloadsCargo const& o,
                         cdr::tag_t<Stop4UnloadsCargo> );

cdr::result<Stop4UnloadsCargo> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stop4UnloadsCargo> );

/****************************************************************
** ExpeditionaryForce
*****************************************************************/
struct ExpeditionaryForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};

  bool operator==( ExpeditionaryForce const& ) const = default;
};

// String conversion.
void to_str( ExpeditionaryForce const& o, std::string& out, base::tag<ExpeditionaryForce> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ExpeditionaryForce& o );

bool write_binary( base::IBinaryIO& b, ExpeditionaryForce const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         ExpeditionaryForce const& o,
                         cdr::tag_t<ExpeditionaryForce> );

cdr::result<ExpeditionaryForce> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ExpeditionaryForce> );

/****************************************************************
** BackupForce
*****************************************************************/
struct BackupForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};

  bool operator==( BackupForce const& ) const = default;
};

// String conversion.
void to_str( BackupForce const& o, std::string& out, base::tag<BackupForce> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, BackupForce& o );

bool write_binary( base::IBinaryIO& b, BackupForce const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         BackupForce const& o,
                         cdr::tag_t<BackupForce> );

cdr::result<BackupForce> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<BackupForce> );

/****************************************************************
** PriceGroupState
*****************************************************************/
struct PriceGroupState {
  bytes<18> unused1 = {};
  uint16_t rum = {};
  uint16_t cigars = {};
  uint16_t cloth = {};
  uint16_t coats = {};
  bytes<6> unused2 = {};

  bool operator==( PriceGroupState const& ) const = default;
};

// String conversion.
void to_str( PriceGroupState const& o, std::string& out, base::tag<PriceGroupState> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PriceGroupState& o );

bool write_binary( base::IBinaryIO& b, PriceGroupState const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         PriceGroupState const& o,
                         cdr::tag_t<PriceGroupState> );

cdr::result<PriceGroupState> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PriceGroupState> );

/****************************************************************
** HEADER
*****************************************************************/
struct HEADER {
  array_string<9> colonize = {};
  bytes<3> unknown00 = {};
  uint16_t map_size_x = {};
  uint16_t map_size_y = {};
  TutorialHelp tutorial_help = {};
  bytes<1> unknown03 = {};
  GameFlags1 game_flags_1 = {};
  ColonyReportOptionsToDisable colony_report_options_to_disable = {};
  GameFlags2 game_flags_2 = {};
  bytes<2> unknown39 = {};
  uint16_t year = {};
  season_type season = {};
  uint16_t turn = {};
  bytes<1> tile_selection_mode = {};
  bytes<1> unknown40 = {};
  int16_t active_unit = {};
  nation_2byte_type nation_turn = {};
  nation_2byte_type curr_nation_map_view = {};
  nation_2byte_type human_player = {};
  uint16_t dwelling_count = {};
  uint16_t unit_count = {};
  uint16_t colony_count = {};
  uint16_t trade_route_count = {};
  uint16_t show_entire_map = {};
  nation_2byte_type fixed_nation_map_view = {};
  difficulty_type difficulty = {};
  bytes<1> unknown43a = {};
  bytes<1> unknown43b = {};
  bytes<25> founding_father = {};
  bytes<2> unknown44aa = {};
  bytes<1> manual_save_flag = {};
  bytes<1> unknown44ab = {};
  end_of_turn_sign_type end_of_turn_sign = {};
  bytes<8> nation_relation = {};
  int16_t rebel_sentiment_report = {};
  bytes<6> unknown45a = {};
  int16_t last_reported_rebel_sentiment = {};
  ExpeditionaryForce expeditionary_force = {};
  BackupForce backup_force = {};
  PriceGroupState price_group_state = {};
  Event event = {};
  bytes<2> unknown05 = {};

  bool operator==( HEADER const& ) const = default;
};

// String conversion.
void to_str( HEADER const& o, std::string& out, base::tag<HEADER> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, HEADER& o );

bool write_binary( base::IBinaryIO& b, HEADER const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         HEADER const& o,
                         cdr::tag_t<HEADER> );

cdr::result<HEADER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<HEADER> );

/****************************************************************
** PLAYER
*****************************************************************/
struct PLAYER {
  array_string<24> name = {};
  array_string<24> country_name = {};
  PlayerFlags player_flags = {};
  control_type control = {};
  uint8_t founded_colonies = {};
  bytes<1> diplomacy = {};

  bool operator==( PLAYER const& ) const = default;
};

// String conversion.
void to_str( PLAYER const& o, std::string& out, base::tag<PLAYER> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PLAYER& o );

bool write_binary( base::IBinaryIO& b, PLAYER const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         PLAYER const& o,
                         cdr::tag_t<PLAYER> );

cdr::result<PLAYER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PLAYER> );

/****************************************************************
** OTHER
*****************************************************************/
struct OTHER {
  bytes<18> unknown51a = {};
  std::array<uint16_t, 2> click_before_open_colony_x_y = {};
  bytes<2> unknown51b = {};

  bool operator==( OTHER const& ) const = default;
};

// String conversion.
void to_str( OTHER const& o, std::string& out, base::tag<OTHER> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, OTHER& o );

bool write_binary( base::IBinaryIO& b, OTHER const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         OTHER const& o,
                         cdr::tag_t<OTHER> );

cdr::result<OTHER> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<OTHER> );

/****************************************************************
** Tiles
*****************************************************************/
struct Tiles {
  int8_t tile_n = {};
  int8_t tile_e = {};
  int8_t tile_s = {};
  int8_t tile_w = {};
  int8_t tile_nw = {};
  int8_t tile_ne = {};
  int8_t tile_se = {};
  int8_t tile_sw = {};

  bool operator==( Tiles const& ) const = default;
};

// String conversion.
void to_str( Tiles const& o, std::string& out, base::tag<Tiles> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Tiles& o );

bool write_binary( base::IBinaryIO& b, Tiles const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Tiles const& o,
                         cdr::tag_t<Tiles> );

cdr::result<Tiles> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Tiles> );

/****************************************************************
** Stock
*****************************************************************/
struct Stock {
  int16_t food = {};
  int16_t sugar = {};
  int16_t tobacco = {};
  int16_t cotton = {};
  int16_t furs = {};
  int16_t lumber = {};
  int16_t ore = {};
  int16_t silver = {};
  int16_t horses = {};
  int16_t rum = {};
  int16_t cigars = {};
  int16_t cloth = {};
  int16_t coats = {};
  int16_t trade_goods = {};
  int16_t tools = {};
  int16_t muskets = {};

  bool operator==( Stock const& ) const = default;
};

// String conversion.
void to_str( Stock const& o, std::string& out, base::tag<Stock> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Stock& o );

bool write_binary( base::IBinaryIO& b, Stock const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Stock const& o,
                         cdr::tag_t<Stock> );

cdr::result<Stock> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Stock> );

/****************************************************************
** PopulationOnMap
*****************************************************************/
struct PopulationOnMap {
  uint8_t for_english = {};
  uint8_t for_french = {};
  uint8_t for_spanish = {};
  uint8_t for_dutch = {};

  bool operator==( PopulationOnMap const& ) const = default;
};

// String conversion.
void to_str( PopulationOnMap const& o, std::string& out, base::tag<PopulationOnMap> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, PopulationOnMap& o );

bool write_binary( base::IBinaryIO& b, PopulationOnMap const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         PopulationOnMap const& o,
                         cdr::tag_t<PopulationOnMap> );

cdr::result<PopulationOnMap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<PopulationOnMap> );

/****************************************************************
** FortificationOnMap
*****************************************************************/
struct FortificationOnMap {
  fortification_level_type for_english = {};
  fortification_level_type for_french = {};
  fortification_level_type for_spanish = {};
  fortification_level_type for_dutch = {};

  bool operator==( FortificationOnMap const& ) const = default;
};

// String conversion.
void to_str( FortificationOnMap const& o, std::string& out, base::tag<FortificationOnMap> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, FortificationOnMap& o );

bool write_binary( base::IBinaryIO& b, FortificationOnMap const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         FortificationOnMap const& o,
                         cdr::tag_t<FortificationOnMap> );

cdr::result<FortificationOnMap> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<FortificationOnMap> );

/****************************************************************
** COLONY
*****************************************************************/
struct COLONY {
  std::array<uint8_t, 2> x_y = {};
  array_string<24> name = {};
  nation_type nation_id = {};
  bytes<1> unknown08a = {};
  ColonyFlags colony_flags = {};
  bytes<2> unknown08b = {};
  uint8_t population = {};
  std::array<occupation_type, 32> occupation = {};
  std::array<profession_type, 32> profession = {};
  std::array<Duration, 16> duration = {};
  Tiles tiles = {};
  bytes<12> unknown10 = {};
  Buildings buildings = {};
  CustomHouseFlags custom_house_flags = {};
  bytes<6> unknown11 = {};
  uint16_t hammers = {};
  bytes<1> building_in_production = {};
  uint8_t warehouse_level = {};
  bytes<1> unknown12a = {};
  uint8_t depletion_counter = {};
  uint16_t hammers_purchased = {};
  Stock stock = {};
  PopulationOnMap population_on_map = {};
  FortificationOnMap fortification_on_map = {};
  int32_t rebel_dividend = {};
  int32_t rebel_divisor = {};

  bool operator==( COLONY const& ) const = default;
};

// String conversion.
void to_str( COLONY const& o, std::string& out, base::tag<COLONY> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, COLONY& o );

bool write_binary( base::IBinaryIO& b, COLONY const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         COLONY const& o,
                         cdr::tag_t<COLONY> );

cdr::result<COLONY> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<COLONY> );

/****************************************************************
** TransportChain
*****************************************************************/
struct TransportChain {
  int16_t next_unit_idx = {};
  int16_t prev_unit_idx = {};

  bool operator==( TransportChain const& ) const = default;
};

// String conversion.
void to_str( TransportChain const& o, std::string& out, base::tag<TransportChain> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TransportChain& o );

bool write_binary( base::IBinaryIO& b, TransportChain const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TransportChain const& o,
                         cdr::tag_t<TransportChain> );

cdr::result<TransportChain> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TransportChain> );

/****************************************************************
** UNIT
*****************************************************************/
struct UNIT {
  std::array<uint8_t, 2> x_y = {};
  unit_type type = {};
  NationInfo nation_info = {};
  Unknown15 unknown15 = {};
  uint8_t moves = {};
  uint8_t origin_settlement = {};
  array_string<1> ai_plan_mode = {};
  orders_type orders = {};
  uint8_t goto_x = {};
  uint8_t goto_y = {};
  bytes<1> unknown18 = {};
  uint8_t holds_occupied = {};
  std::array<CargoItems, 3> cargo_items = {};
  std::array<uint8_t, 6> cargo_hold = {};
  uint8_t turns_worked = {};
  uint8_t profession_or_treasure_amount = {};
  TransportChain transport_chain = {};

  bool operator==( UNIT const& ) const = default;
};

// String conversion.
void to_str( UNIT const& o, std::string& out, base::tag<UNIT> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UNIT& o );

bool write_binary( base::IBinaryIO& b, UNIT const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UNIT const& o,
                         cdr::tag_t<UNIT> );

cdr::result<UNIT> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UNIT> );

/****************************************************************
** IntrinsicVolume
*****************************************************************/
struct IntrinsicVolume {
  int16_t food = {};
  int16_t sugar = {};
  int16_t tobacco = {};
  int16_t cotton = {};
  int16_t furs = {};
  int16_t lumber = {};
  int16_t ore = {};
  int16_t silver = {};
  int16_t horses = {};
  int16_t rum = {};
  int16_t cigars = {};
  int16_t cloth = {};
  int16_t coats = {};
  int16_t trade_goods = {};
  int16_t tools = {};
  int16_t muskets = {};

  bool operator==( IntrinsicVolume const& ) const = default;
};

// String conversion.
void to_str( IntrinsicVolume const& o, std::string& out, base::tag<IntrinsicVolume> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, IntrinsicVolume& o );

bool write_binary( base::IBinaryIO& b, IntrinsicVolume const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         IntrinsicVolume const& o,
                         cdr::tag_t<IntrinsicVolume> );

cdr::result<IntrinsicVolume> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<IntrinsicVolume> );

/****************************************************************
** Trade
*****************************************************************/
struct Trade {
  std::array<uint8_t, 16> euro_price = {};
  IntrinsicVolume intrinsic_volume = {};
  std::array<int32_t, 16> gold = {};
  std::array<int32_t, 16> tons_traded = {};
  std::array<int32_t, 16> tons_traded2 = {};

  bool operator==( Trade const& ) const = default;
};

// String conversion.
void to_str( Trade const& o, std::string& out, base::tag<Trade> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Trade& o );

bool write_binary( base::IBinaryIO& b, Trade const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Trade const& o,
                         cdr::tag_t<Trade> );

cdr::result<Trade> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Trade> );

/****************************************************************
** NATION
*****************************************************************/
struct NATION {
  NationFlags nation_flags = {};
  uint8_t tax_rate = {};
  std::array<profession_type, 3> recruit = {};
  bytes<1> unused07 = {};
  uint8_t recruit_count = {};
  FoundingFathers founding_fathers = {};
  bytes<1> unknown21 = {};
  int16_t liberty_bells_total = {};
  int16_t liberty_bells_last_turn = {};
  bytes<2> unknown22 = {};
  int16_t next_founding_father = {};
  uint16_t founding_father_count = {};
  bytes<2> prob_founding_father_count_end = {};
  uint8_t villages_burned = {};
  int8_t rebel_sentiment = {};
  bytes<4> unknown23 = {};
  uint16_t artillery_bought_count = {};
  BoycottBitmap boycott_bitmap = {};
  int32_t royal_money = {};
  int32_t player_total_income = {};
  int32_t gold = {};
  uint16_t current_crosses = {};
  uint16_t needed_crosses = {};
  std::array<uint8_t, 2> point_return_from_europe = {};
  std::array<RelationByNations, 4> relation_by_nations = {};
  std::array<RelationByIndian, 8> relation_by_indian = {};
  bytes<4> unknown26a = {};
  bytes<2> unknown26b = {};
  bytes<6> unknown26c = {};
  Trade trade = {};

  bool operator==( NATION const& ) const = default;
};

// String conversion.
void to_str( NATION const& o, std::string& out, base::tag<NATION> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NATION& o );

bool write_binary( base::IBinaryIO& b, NATION const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         NATION const& o,
                         cdr::tag_t<NATION> );

cdr::result<NATION> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NATION> );

/****************************************************************
** Alarm
*****************************************************************/
struct Alarm {
  bytes<1> friction = {};
  bytes<1> attacks = {};

  bool operator==( Alarm const& ) const = default;
};

// String conversion.
void to_str( Alarm const& o, std::string& out, base::tag<Alarm> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Alarm& o );

bool write_binary( base::IBinaryIO& b, Alarm const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Alarm const& o,
                         cdr::tag_t<Alarm> );

cdr::result<Alarm> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Alarm> );

/****************************************************************
** DWELLING
*****************************************************************/
struct DWELLING {
  std::array<uint8_t, 2> x_y = {};
  nation_type nation_id = {};
  BLCS blcs = {};
  uint8_t population = {};
  Mission mission = {};
  int8_t growth_counter = {};
  bytes<1> unknown28a = {};
  bytes<1> last_bought = {};
  bytes<1> last_sold = {};
  std::array<Alarm, 4> alarm = {};

  bool operator==( DWELLING const& ) const = default;
};

// String conversion.
void to_str( DWELLING const& o, std::string& out, base::tag<DWELLING> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, DWELLING& o );

bool write_binary( base::IBinaryIO& b, DWELLING const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         DWELLING const& o,
                         cdr::tag_t<DWELLING> );

cdr::result<DWELLING> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<DWELLING> );

/****************************************************************
** TRIBE
*****************************************************************/
struct TRIBE {
  std::array<uint8_t, 2> capitol_x_y = {};
  tech_type tech = {};
  TribeFlags tribe_flags = {};
  bytes<3> unknown31b = {};
  int8_t muskets = {};
  uint8_t horse_herds = {};
  bytes<1> unknown31c = {};
  uint16_t horse_breeding = {};
  bytes<2> unknown31d = {};
  Stock stock = {};
  bytes<12> unknown32 = {};
  std::array<RelationByNations2, 4> relation_by_nations = {};
  bytes<8> zeros33 = {};
  std::array<uint16_t, 4> alarm_by_player = {};

  bool operator==( TRIBE const& ) const = default;
};

// String conversion.
void to_str( TRIBE const& o, std::string& out, base::tag<TRIBE> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TRIBE& o );

bool write_binary( base::IBinaryIO& b, TRIBE const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TRIBE const& o,
                         cdr::tag_t<TRIBE> );

cdr::result<TRIBE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TRIBE> );

/****************************************************************
** NationUnitCount
*****************************************************************/
struct NationUnitCount {
  uint8_t english = {};
  uint8_t french = {};
  uint8_t spanish = {};
  uint8_t dutch = {};

  bool operator==( NationUnitCount const& ) const = default;
};

// String conversion.
void to_str( NationUnitCount const& o, std::string& out, base::tag<NationUnitCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationUnitCount& o );

bool write_binary( base::IBinaryIO& b, NationUnitCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         NationUnitCount const& o,
                         cdr::tag_t<NationUnitCount> );

cdr::result<NationUnitCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationUnitCount> );

/****************************************************************
** NationColonyCount
*****************************************************************/
struct NationColonyCount {
  uint8_t english = {};
  uint8_t french = {};
  uint8_t spanish = {};
  uint8_t dutch = {};

  bool operator==( NationColonyCount const& ) const = default;
};

// String conversion.
void to_str( NationColonyCount const& o, std::string& out, base::tag<NationColonyCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, NationColonyCount& o );

bool write_binary( base::IBinaryIO& b, NationColonyCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         NationColonyCount const& o,
                         cdr::tag_t<NationColonyCount> );

cdr::result<NationColonyCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<NationColonyCount> );

/****************************************************************
** Unknown34a
*****************************************************************/
struct Unknown34a {
  bytes<1> english = {};
  bytes<1> french = {};
  bytes<1> spanish = {};
  bytes<1> dutch = {};

  bool operator==( Unknown34a const& ) const = default;
};

// String conversion.
void to_str( Unknown34a const& o, std::string& out, base::tag<Unknown34a> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown34a& o );

bool write_binary( base::IBinaryIO& b, Unknown34a const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Unknown34a const& o,
                         cdr::tag_t<Unknown34a> );

cdr::result<Unknown34a> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown34a> );

/****************************************************************
** TotalColoniesPopulation
*****************************************************************/
struct TotalColoniesPopulation {
  uint8_t english = {};
  uint8_t french = {};
  uint8_t spanish = {};
  uint8_t dutch = {};

  bool operator==( TotalColoniesPopulation const& ) const = default;
};

// String conversion.
void to_str( TotalColoniesPopulation const& o, std::string& out, base::tag<TotalColoniesPopulation> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TotalColoniesPopulation& o );

bool write_binary( base::IBinaryIO& b, TotalColoniesPopulation const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TotalColoniesPopulation const& o,
                         cdr::tag_t<TotalColoniesPopulation> );

cdr::result<TotalColoniesPopulation> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TotalColoniesPopulation> );

/****************************************************************
** Unknown36ab
*****************************************************************/
struct Unknown36ab {
  bytes<1> english = {};
  bytes<1> french = {};
  bytes<1> spanish = {};
  bytes<1> dutch = {};

  bool operator==( Unknown36ab const& ) const = default;
};

// String conversion.
void to_str( Unknown36ab const& o, std::string& out, base::tag<Unknown36ab> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, Unknown36ab& o );

bool write_binary( base::IBinaryIO& b, Unknown36ab const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         Unknown36ab const& o,
                         cdr::tag_t<Unknown36ab> );

cdr::result<Unknown36ab> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<Unknown36ab> );

/****************************************************************
** ForeignAffairsReport
*****************************************************************/
struct ForeignAffairsReport {
  std::array<uint8_t, 4> population = {};
  Unknown36ab unknown36ab = {};
  std::array<uint8_t, 4> merchant_marine = {};
  std::array<uint8_t, 4> ship_counts = {};

  bool operator==( ForeignAffairsReport const& ) const = default;
};

// String conversion.
void to_str( ForeignAffairsReport const& o, std::string& out, base::tag<ForeignAffairsReport> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ForeignAffairsReport& o );

bool write_binary( base::IBinaryIO& b, ForeignAffairsReport const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         ForeignAffairsReport const& o,
                         cdr::tag_t<ForeignAffairsReport> );

cdr::result<ForeignAffairsReport> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ForeignAffairsReport> );

/****************************************************************
** UnknownTribeData1
*****************************************************************/
struct UnknownTribeData1 {
  bytes<1> inca = {};
  bytes<1> aztec = {};
  bytes<1> arawak = {};
  bytes<1> iroquois = {};
  bytes<1> cherokee = {};
  bytes<1> apache = {};
  bytes<1> sioux = {};
  bytes<1> tupi = {};

  bool operator==( UnknownTribeData1 const& ) const = default;
};

// String conversion.
void to_str( UnknownTribeData1 const& o, std::string& out, base::tag<UnknownTribeData1> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData1& o );

bool write_binary( base::IBinaryIO& b, UnknownTribeData1 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData1 const& o,
                         cdr::tag_t<UnknownTribeData1> );

cdr::result<UnknownTribeData1> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData1> );

/****************************************************************
** UnknownTribeData2
*****************************************************************/
struct UnknownTribeData2 {
  bytes<1> inca = {};
  bytes<1> aztec = {};
  bytes<1> arawak = {};
  bytes<1> iroquois = {};
  bytes<1> cherokee = {};
  bytes<1> apache = {};
  bytes<1> sioux = {};
  bytes<1> tupi = {};

  bool operator==( UnknownTribeData2 const& ) const = default;
};

// String conversion.
void to_str( UnknownTribeData2 const& o, std::string& out, base::tag<UnknownTribeData2> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData2& o );

bool write_binary( base::IBinaryIO& b, UnknownTribeData2 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData2 const& o,
                         cdr::tag_t<UnknownTribeData2> );

cdr::result<UnknownTribeData2> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData2> );

/****************************************************************
** TribeDwellingCount
*****************************************************************/
struct TribeDwellingCount {
  uint8_t inca = {};
  uint8_t aztec = {};
  uint8_t arawak = {};
  uint8_t iroquois = {};
  uint8_t cherokee = {};
  uint8_t apache = {};
  uint8_t sioux = {};
  uint8_t tupi = {};

  bool operator==( TribeDwellingCount const& ) const = default;
};

// String conversion.
void to_str( TribeDwellingCount const& o, std::string& out, base::tag<TribeDwellingCount> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TribeDwellingCount& o );

bool write_binary( base::IBinaryIO& b, TribeDwellingCount const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TribeDwellingCount const& o,
                         cdr::tag_t<TribeDwellingCount> );

cdr::result<TribeDwellingCount> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TribeDwellingCount> );

/****************************************************************
** UnknownTribeData4
*****************************************************************/
struct UnknownTribeData4 {
  bytes<1> inca = {};
  bytes<1> aztec = {};
  bytes<1> arawak = {};
  bytes<1> iroquois = {};
  bytes<1> cherokee = {};
  bytes<1> apache = {};
  bytes<1> sioux = {};
  bytes<1> tupi = {};

  bool operator==( UnknownTribeData4 const& ) const = default;
};

// String conversion.
void to_str( UnknownTribeData4 const& o, std::string& out, base::tag<UnknownTribeData4> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData4& o );

bool write_binary( base::IBinaryIO& b, UnknownTribeData4 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData4 const& o,
                         cdr::tag_t<UnknownTribeData4> );

cdr::result<UnknownTribeData4> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData4> );

/****************************************************************
** UnknownTribeData5
*****************************************************************/
struct UnknownTribeData5 {
  bytes<1> inca = {};
  bytes<1> aztec = {};
  bytes<1> arawak = {};
  bytes<1> iroquois = {};
  bytes<1> cherokee = {};
  bytes<1> apache = {};
  bytes<1> sioux = {};
  bytes<1> tupi = {};

  bool operator==( UnknownTribeData5 const& ) const = default;
};

// String conversion.
void to_str( UnknownTribeData5 const& o, std::string& out, base::tag<UnknownTribeData5> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData5& o );

bool write_binary( base::IBinaryIO& b, UnknownTribeData5 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData5 const& o,
                         cdr::tag_t<UnknownTribeData5> );

cdr::result<UnknownTribeData5> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData5> );

/****************************************************************
** UnknownTribeData6
*****************************************************************/
struct UnknownTribeData6 {
  bytes<1> inca = {};
  bytes<1> aztec = {};
  bytes<1> arawak = {};
  bytes<1> iroquois = {};
  bytes<1> cherokee = {};
  bytes<1> apache = {};
  bytes<1> sioux = {};
  bytes<1> tupi = {};

  bool operator==( UnknownTribeData6 const& ) const = default;
};

// String conversion.
void to_str( UnknownTribeData6 const& o, std::string& out, base::tag<UnknownTribeData6> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, UnknownTribeData6& o );

bool write_binary( base::IBinaryIO& b, UnknownTribeData6 const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         UnknownTribeData6 const& o,
                         cdr::tag_t<UnknownTribeData6> );

cdr::result<UnknownTribeData6> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<UnknownTribeData6> );

/****************************************************************
** STUFF
*****************************************************************/
struct STUFF {
  bytes<12> unknown34 = {};
  NationUnitCount nation_unit_count = {};
  NationColonyCount nation_colony_count = {};
  Unknown34a unknown34a = {};
  TotalColoniesPopulation total_colonies_population = {};
  ForeignAffairsReport foreign_affairs_report = {};
  std::array<bytes<64>, 8> unknown36ac = {};
  bytes<8> unknown36ad = {};
  yes_no_byte show_colony_prod_quantities = {};
  UnknownTribeData1 unknown_tribe_data_1 = {};
  UnknownTribeData2 unknown_tribe_data_2 = {};
  TribeDwellingCount tribe_dwelling_count = {};
  UnknownTribeData4 unknown_tribe_data_4 = {};
  UnknownTribeData5 unknown_tribe_data_5 = {};
  UnknownTribeData6 unknown_tribe_data_6 = {};
  bytes<104> unknown36b = {};
  uint16_t white_box_x = {};
  uint16_t white_box_y = {};
  uint8_t zoom_level = {};
  bytes<1> unknown37 = {};
  uint16_t viewport_x = {};
  uint16_t viewport_y = {};

  bool operator==( STUFF const& ) const = default;
};

// String conversion.
void to_str( STUFF const& o, std::string& out, base::tag<STUFF> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, STUFF& o );

bool write_binary( base::IBinaryIO& b, STUFF const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         STUFF const& o,
                         cdr::tag_t<STUFF> );

cdr::result<STUFF> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<STUFF> );

/****************************************************************
** CONNECTIVITY
*****************************************************************/
struct CONNECTIVITY {
  std::array<SeaLaneConnectivity, 270> sea_lane_connectivity = {};
  std::array<LandConnectivity, 270> land_connectivity = {};

  bool operator==( CONNECTIVITY const& ) const = default;
};

// String conversion.
void to_str( CONNECTIVITY const& o, std::string& out, base::tag<CONNECTIVITY> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, CONNECTIVITY& o );

bool write_binary( base::IBinaryIO& b, CONNECTIVITY const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         CONNECTIVITY const& o,
                         cdr::tag_t<CONNECTIVITY> );

cdr::result<CONNECTIVITY> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<CONNECTIVITY> );

/****************************************************************
** TRADEROUTE
*****************************************************************/
struct TRADEROUTE {
  array_string<32> name = {};
  trade_route_type land_or_sea = {};
  uint8_t stops_count = {};
  uint16_t stop_1_colony_index = {};
  Stop1LoadsAndUnloadsCount stop_1_loads_and_unloads_count = {};
  Stop1LoadsCargo stop_1_loads_cargo = {};
  Stop1UnloadsCargo stop_1_unloads_cargo = {};
  bytes<1> unknown47 = {};
  uint16_t stop_2_colony_index = {};
  Stop2LoadsAndUnloadsCount stop_2_loads_and_unloads_count = {};
  Stop2LoadsCargo stop_2_loads_cargo = {};
  Stop2UnloadsCargo stop_2_unloads_cargo = {};
  bytes<1> unknown48 = {};
  uint16_t stop_3_colony_index = {};
  Stop3LoadsAndUnloadsCount stop_3_loads_and_unloads_count = {};
  Stop3LoadsCargo stop_3_loads_cargo = {};
  Stop3UnloadsCargo stop_3_unloads_cargo = {};
  bytes<1> unknown49 = {};
  uint16_t stop_4_colony_index = {};
  Stop4LoadsAndUnloadsCount stop_4_loads_and_unloads_count = {};
  Stop4LoadsCargo stop_4_loads_cargo = {};
  Stop4UnloadsCargo stop_4_unloads_cargo = {};
  bytes<1> unknown50 = {};

  bool operator==( TRADEROUTE const& ) const = default;
};

// String conversion.
void to_str( TRADEROUTE const& o, std::string& out, base::tag<TRADEROUTE> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, TRADEROUTE& o );

bool write_binary( base::IBinaryIO& b, TRADEROUTE const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         TRADEROUTE const& o,
                         cdr::tag_t<TRADEROUTE> );

cdr::result<TRADEROUTE> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<TRADEROUTE> );

/****************************************************************
** ColonySAV
*****************************************************************/
struct ColonySAV {
  HEADER header = {};
  std::array<PLAYER, 4> player = {};
  OTHER other = {};
  std::vector<COLONY> colony = {};
  std::vector<UNIT> unit = {};
  std::array<NATION, 4> nation = {};
  std::vector<DWELLING> dwelling = {};
  std::array<TRIBE, 8> tribe = {};
  STUFF stuff = {};
  std::vector<TILE> tile = {};
  std::vector<MASK> mask = {};
  std::vector<PATH> path = {};
  std::vector<SEEN> seen = {};
  CONNECTIVITY connectivity = {};
  std::array<bytes<2>, 9> unknown_map38c2 = {};
  bytes<16> unknown_map38c3 = {};
  std::array<uint16_t, 14> strategy = {};
  bytes<10> unknown_map38d = {};
  uint8_t prime_resource_seed = {};
  bytes<1> unknown39d = {};
  std::array<TRADEROUTE, 12> trade_route = {};

  bool operator==( ColonySAV const& ) const = default;
};

// String conversion.
void to_str( ColonySAV const& o, std::string& out, base::tag<ColonySAV> );

// Binary conversion.
bool read_binary( base::IBinaryIO& b, ColonySAV& o );

bool write_binary( base::IBinaryIO& b, ColonySAV const& o );

// Cdr conversions.
cdr::value to_canonical( cdr::converter& conv,
                         ColonySAV const& o,
                         cdr::tag_t<ColonySAV> );

cdr::result<ColonySAV> from_canonical(
                         cdr::converter& conv,
                         cdr::value const& v,
                         cdr::tag_t<ColonySAV> );

}  // namespace sav
