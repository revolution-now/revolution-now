/****************************************************************
** Classic Colonization Save File Structure.
*****************************************************************/
// NOTE: this file was auto-generated. DO NOT MODIFY!

// sav
#include "sav-struct.hpp"

// base
#include "base/binary-data.hpp"

namespace sav {

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
  bits <<= 7; bits |= (o.unused01 & 0b1111111);
  bits <<= 1; bits |= (o.tutorial_hints & 0b1);
  bits <<= 1; bits |= (o.water_color_cycling & 0b1);
  bits <<= 1; bits |= (o.combat_analysis & 0b1);
  bits <<= 1; bits |= (o.autosave & 0b1);
  bits <<= 1; bits |= (o.end_of_turn & 0b1);
  bits <<= 1; bits |= (o.fast_piece_slide & 0b1);
  bits <<= 1; bits |= (o.cheats_enabled & 0b1);
  bits <<= 1; bits |= (o.show_foreign_moves & 0b1);
  bits <<= 1; bits |= (o.show_indian_moves & 0b1);
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
  bits <<= 1; bits |= (o.labels_on_cargo_and_terrain & 0b1);
  bits <<= 1; bits |= (o.labels_on_buildings & 0b1);
  bits <<= 1; bits |= (o.report_new_cargos_available & 0b1);
  bits <<= 1; bits |= (o.report_inefficient_government & 0b1);
  bits <<= 1; bits |= (o.report_tools_needed_for_production & 0b1);
  bits <<= 1; bits |= (o.report_raw_materials_shortages & 0b1);
  bits <<= 1; bits |= (o.report_food_shortages & 0b1);
  bits <<= 1; bits |= (o.report_when_colonists_trained & 0b1);
  bits <<= 1; bits |= (o.report_sons_of_liberty_membership & 0b1);
  bits <<= 1; bits |= (o.report_rebel_majorities & 0b1);
  bits <<= 6; bits |= (o.unused03 & 0b111111);
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
  bits <<= 1; bits |= (o.discovery_of_the_new_world & 0b1);
  bits <<= 1; bits |= (o.building_a_colony & 0b1);
  bits <<= 1; bits |= (o.meeting_the_natives & 0b1);
  bits <<= 1; bits |= (o.the_aztec_empire & 0b1);
  bits <<= 1; bits |= (o.the_inca_nation & 0b1);
  bits <<= 1; bits |= (o.discovery_of_the_pacific_ocean & 0b1);
  bits <<= 1; bits |= (o.entering_indian_village & 0b1);
  bits <<= 1; bits |= (o.the_fountain_of_youth & 0b1);
  bits <<= 1; bits |= (o.cargo_from_the_new_world & 0b1);
  bits <<= 1; bits |= (o.meeting_fellow_europeans & 0b1);
  bits <<= 1; bits |= (o.colony_burning & 0b1);
  bits <<= 1; bits |= (o.colony_destroyed & 0b1);
  bits <<= 1; bits |= (o.indian_raid & 0b1);
  bits <<= 1; bits |= (o.woodcut14 & 0b1);
  bits <<= 1; bits |= (o.woodcut15 & 0b1);
  bits <<= 1; bits |= (o.woodcut16 & 0b1);
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
  bits <<= 4; bits |= (o.dur_1 & 0b1111);
  bits <<= 4; bits |= (o.dur_2 & 0b1111);
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
  bits <<= 3; bits |= (static_cast<uint64_t>( o.fortification ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.armory ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.docks ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.town_hall ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.schoolhouse ) & 0b111);
  bits <<= 1; bits |= (o.warehouse & 0b1);
  bits <<= 1; bits |= (o.unused05a & 0b1);
  bits <<= 1; bits |= (o.stables & 0b1);
  bits <<= 1; bits |= (o.custom_house & 0b1);
  bits <<= 2; bits |= (static_cast<uint64_t>( o.printing_press ) & 0b11);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.weavers_house ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.tobacconists_house ) & 0b111);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.rum_distillers_house ) & 0b111);
  bits <<= 2; bits |= (static_cast<uint64_t>( o.capitol_unused ) & 0b11);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.fur_traders_house ) & 0b111);
  bits <<= 2; bits |= (static_cast<uint64_t>( o.carpenters_shop ) & 0b11);
  bits <<= 2; bits |= (static_cast<uint64_t>( o.church ) & 0b11);
  bits <<= 3; bits |= (static_cast<uint64_t>( o.blacksmiths_house ) & 0b111);
  bits <<= 6; bits |= (o.unused05b & 0b111111);
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
  bits <<= 1; bits |= (o.food & 0b1);
  bits <<= 1; bits |= (o.sugar & 0b1);
  bits <<= 1; bits |= (o.tobacco & 0b1);
  bits <<= 1; bits |= (o.cotton & 0b1);
  bits <<= 1; bits |= (o.furs & 0b1);
  bits <<= 1; bits |= (o.lumber & 0b1);
  bits <<= 1; bits |= (o.ore & 0b1);
  bits <<= 1; bits |= (o.silver & 0b1);
  bits <<= 1; bits |= (o.horses & 0b1);
  bits <<= 1; bits |= (o.rum & 0b1);
  bits <<= 1; bits |= (o.cigars & 0b1);
  bits <<= 1; bits |= (o.cloth & 0b1);
  bits <<= 1; bits |= (o.coats & 0b1);
  bits <<= 1; bits |= (o.trade_goods & 0b1);
  bits <<= 1; bits |= (o.tools & 0b1);
  bits <<= 1; bits |= (o.muskets & 0b1);
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
  o.unknown14 = (bits & 0b1111); bits >>= 4;
  return true;
}

bool write_binary( base::BinaryData& b, NationInfo const& o ) {
  uint8_t bits = 0;
  bits <<= 4; bits |= (static_cast<uint8_t>( o.nation_id ) & 0b1111);
  bits <<= 4; bits |= (o.unknown14 & 0b1111);
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
  bits <<= 7; bits |= (o.unknown15a & 0b1111111);
  bits <<= 1; bits |= (o.damaged & 0b1);
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
  bits <<= 4; bits |= (static_cast<uint8_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint8_t>( o.cargo_2 ) & 0b1111);
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
  bits <<= 1; bits |= (o.food & 0b1);
  bits <<= 1; bits |= (o.sugar & 0b1);
  bits <<= 1; bits |= (o.tobacco & 0b1);
  bits <<= 1; bits |= (o.cotton & 0b1);
  bits <<= 1; bits |= (o.furs & 0b1);
  bits <<= 1; bits |= (o.lumber & 0b1);
  bits <<= 1; bits |= (o.ore & 0b1);
  bits <<= 1; bits |= (o.silver & 0b1);
  bits <<= 1; bits |= (o.horses & 0b1);
  bits <<= 1; bits |= (o.rum & 0b1);
  bits <<= 1; bits |= (o.cigars & 0b1);
  bits <<= 1; bits |= (o.cloth & 0b1);
  bits <<= 1; bits |= (o.coats & 0b1);
  bits <<= 1; bits |= (o.trade_goods & 0b1);
  bits <<= 1; bits |= (o.tools & 0b1);
  bits <<= 1; bits |= (o.muskets & 0b1);
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
  bits <<= 1; bits |= (o.artillery_near & 0b1);
  bits <<= 1; bits |= (o.learned & 0b1);
  bits <<= 1; bits |= (o.capital & 0b1);
  bits <<= 1; bits |= (o.scouted & 0b1);
  bits <<= 4; bits |= (o.unused09 & 0b1111);
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
  bits <<= 5; bits |= (static_cast<uint8_t>( o.tile ) & 0b11111);
  bits <<= 3; bits |= (static_cast<uint8_t>( o.hill_river ) & 0b111);
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
  bits <<= 1; bits |= (static_cast<uint8_t>( o.has_unit ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.has_city ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.suppress ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.road ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.purchased ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.pacific ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.plowed ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.unused ) & 0b1);
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
  bits <<= 4; bits |= (static_cast<uint8_t>( o.region_id ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint8_t>( o.visitor_nation ) & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint8_t>( o.score ) & 0b1111);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.vis2en ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.vis2fr ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.vis2sp ) & 0b1);
  bits <<= 1; bits |= (static_cast<uint8_t>( o.vis2du ) & 0b1);
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
  bits <<= 4; bits |= (o.unloads_count & 0b1111);
  bits <<= 4; bits |= (o.loads_count & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (o.unloads_count & 0b1111);
  bits <<= 4; bits |= (o.loads_count & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (o.unloads_count & 0b1111);
  bits <<= 4; bits |= (o.loads_count & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (o.unloads_count & 0b1111);
  bits <<= 4; bits |= (o.loads_count & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_1 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_2 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_3 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_4 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_5 ) & 0b1111);
  bits <<= 4; bits |= (static_cast<uint32_t>( o.cargo_6 ) & 0b1111);
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
    && read_binary( b, o.unknown16 )
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
    && write_binary( b, o.unknown16 )
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
    && read_binary( b, o.unknown25a )
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
    && write_binary( b, o.unknown25a )
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
