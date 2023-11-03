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
bool read_binary( base::BinaryReader& b, GameOptions& o ) {
  o.unused01 = b.read_n_bits<7>();
  o.tutorial_hints = b.read_n_bits<1>();
  o.water_color_cycling = b.read_n_bits<1>();
  o.combat_analysis = b.read_n_bits<1>();
  o.autosave = b.read_n_bits<1>();
  o.end_of_turn = b.read_n_bits<1>();
  o.fast_piece_slide = b.read_n_bits<1>();
  o.cheats_enabled = b.read_n_bits<1>();
  o.show_foreign_moves = b.read_n_bits<1>();
  o.show_indian_moves = b.read_n_bits<1>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, GameOptions const& o ) {
  b.write_bits( 7, o.unused01 );
  b.write_bits( 1, o.tutorial_hints );
  b.write_bits( 1, o.water_color_cycling );
  b.write_bits( 1, o.combat_analysis );
  b.write_bits( 1, o.autosave );
  b.write_bits( 1, o.end_of_turn );
  b.write_bits( 1, o.fast_piece_slide );
  b.write_bits( 1, o.cheats_enabled );
  b.write_bits( 1, o.show_foreign_moves );
  b.write_bits( 1, o.show_indian_moves );
  return b.good();
}

/****************************************************************
** ColonyReportOptions
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, ColonyReportOptions& o ) {
  o.labels_on_cargo_and_terrain = b.read_n_bits<1>();
  o.labels_on_buildings = b.read_n_bits<1>();
  o.report_new_cargos_available = b.read_n_bits<1>();
  o.report_inefficient_government = b.read_n_bits<1>();
  o.report_tools_needed_for_production = b.read_n_bits<1>();
  o.report_raw_materials_shortages = b.read_n_bits<1>();
  o.report_food_shortages = b.read_n_bits<1>();
  o.report_when_colonists_trained = b.read_n_bits<1>();
  o.report_sons_of_liberty_membership = b.read_n_bits<1>();
  o.report_rebel_majorities = b.read_n_bits<1>();
  o.unused03 = b.read_n_bits<6>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, ColonyReportOptions const& o ) {
  b.write_bits( 1, o.labels_on_cargo_and_terrain );
  b.write_bits( 1, o.labels_on_buildings );
  b.write_bits( 1, o.report_new_cargos_available );
  b.write_bits( 1, o.report_inefficient_government );
  b.write_bits( 1, o.report_tools_needed_for_production );
  b.write_bits( 1, o.report_raw_materials_shortages );
  b.write_bits( 1, o.report_food_shortages );
  b.write_bits( 1, o.report_when_colonists_trained );
  b.write_bits( 1, o.report_sons_of_liberty_membership );
  b.write_bits( 1, o.report_rebel_majorities );
  b.write_bits( 6, o.unused03 );
  return b.good();
}

/****************************************************************
** Event
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Event& o ) {
  o.discovery_of_the_new_world = b.read_n_bits<1>();
  o.building_a_colony = b.read_n_bits<1>();
  o.meeting_the_natives = b.read_n_bits<1>();
  o.the_aztec_empire = b.read_n_bits<1>();
  o.the_inca_nation = b.read_n_bits<1>();
  o.discovery_of_the_pacific_ocean = b.read_n_bits<1>();
  o.entering_indian_village = b.read_n_bits<1>();
  o.the_fountain_of_youth = b.read_n_bits<1>();
  o.cargo_from_the_new_world = b.read_n_bits<1>();
  o.meeting_fellow_europeans = b.read_n_bits<1>();
  o.colony_burning = b.read_n_bits<1>();
  o.colony_destroyed = b.read_n_bits<1>();
  o.indian_raid = b.read_n_bits<1>();
  o.woodcut14 = b.read_n_bits<1>();
  o.woodcut15 = b.read_n_bits<1>();
  o.woodcut16 = b.read_n_bits<1>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Event const& o ) {
  b.write_bits( 1, o.discovery_of_the_new_world );
  b.write_bits( 1, o.building_a_colony );
  b.write_bits( 1, o.meeting_the_natives );
  b.write_bits( 1, o.the_aztec_empire );
  b.write_bits( 1, o.the_inca_nation );
  b.write_bits( 1, o.discovery_of_the_pacific_ocean );
  b.write_bits( 1, o.entering_indian_village );
  b.write_bits( 1, o.the_fountain_of_youth );
  b.write_bits( 1, o.cargo_from_the_new_world );
  b.write_bits( 1, o.meeting_fellow_europeans );
  b.write_bits( 1, o.colony_burning );
  b.write_bits( 1, o.colony_destroyed );
  b.write_bits( 1, o.indian_raid );
  b.write_bits( 1, o.woodcut14 );
  b.write_bits( 1, o.woodcut15 );
  b.write_bits( 1, o.woodcut16 );
  return b.good();
}

/****************************************************************
** Duration
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Duration& o ) {
  o.dur_1 = b.read_n_bits<4>();
  o.dur_2 = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Duration const& o ) {
  b.write_bits( 4, o.dur_1 );
  b.write_bits( 4, o.dur_2 );
  return b.good();
}

/****************************************************************
** Buildings
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Buildings& o ) {
  o.fortification = b.read_n_bits<3, level_3bit_type>();
  o.armory = b.read_n_bits<3, level_3bit_type>();
  o.docks = b.read_n_bits<3, level_3bit_type>();
  o.town_hall = b.read_n_bits<3, level_3bit_type>();
  o.schoolhouse = b.read_n_bits<3, level_3bit_type>();
  o.warehouse = b.read_n_bits<1>();
  o.unused05a = b.read_n_bits<1>();
  o.stables = b.read_n_bits<1>();
  o.custom_house = b.read_n_bits<1>();
  o.printing_press = b.read_n_bits<2, level_2bit_type>();
  o.weavers_house = b.read_n_bits<3, level_3bit_type>();
  o.tobacconists_house = b.read_n_bits<3, level_3bit_type>();
  o.rum_distillers_house = b.read_n_bits<3, level_3bit_type>();
  o.capitol_unused = b.read_n_bits<2, level_2bit_type>();
  o.fur_traders_house = b.read_n_bits<3, level_3bit_type>();
  o.carpenters_shop = b.read_n_bits<2, level_2bit_type>();
  o.church = b.read_n_bits<2, level_2bit_type>();
  o.blacksmiths_house = b.read_n_bits<3, level_3bit_type>();
  o.unused05b = b.read_n_bits<6>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Buildings const& o ) {
  b.write_bits( 3, o.fortification );
  b.write_bits( 3, o.armory );
  b.write_bits( 3, o.docks );
  b.write_bits( 3, o.town_hall );
  b.write_bits( 3, o.schoolhouse );
  b.write_bits( 1, o.warehouse );
  b.write_bits( 1, o.unused05a );
  b.write_bits( 1, o.stables );
  b.write_bits( 1, o.custom_house );
  b.write_bits( 2, o.printing_press );
  b.write_bits( 3, o.weavers_house );
  b.write_bits( 3, o.tobacconists_house );
  b.write_bits( 3, o.rum_distillers_house );
  b.write_bits( 2, o.capitol_unused );
  b.write_bits( 3, o.fur_traders_house );
  b.write_bits( 2, o.carpenters_shop );
  b.write_bits( 2, o.church );
  b.write_bits( 3, o.blacksmiths_house );
  b.write_bits( 6, o.unused05b );
  return b.good();
}

/****************************************************************
** CustomHouseFlags
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, CustomHouseFlags& o ) {
  o.food = b.read_n_bits<1>();
  o.sugar = b.read_n_bits<1>();
  o.tobacco = b.read_n_bits<1>();
  o.cotton = b.read_n_bits<1>();
  o.furs = b.read_n_bits<1>();
  o.lumber = b.read_n_bits<1>();
  o.ore = b.read_n_bits<1>();
  o.silver = b.read_n_bits<1>();
  o.horses = b.read_n_bits<1>();
  o.rum = b.read_n_bits<1>();
  o.cigars = b.read_n_bits<1>();
  o.cloth = b.read_n_bits<1>();
  o.coats = b.read_n_bits<1>();
  o.trade_goods = b.read_n_bits<1>();
  o.tools = b.read_n_bits<1>();
  o.muskets = b.read_n_bits<1>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, CustomHouseFlags const& o ) {
  b.write_bits( 1, o.food );
  b.write_bits( 1, o.sugar );
  b.write_bits( 1, o.tobacco );
  b.write_bits( 1, o.cotton );
  b.write_bits( 1, o.furs );
  b.write_bits( 1, o.lumber );
  b.write_bits( 1, o.ore );
  b.write_bits( 1, o.silver );
  b.write_bits( 1, o.horses );
  b.write_bits( 1, o.rum );
  b.write_bits( 1, o.cigars );
  b.write_bits( 1, o.cloth );
  b.write_bits( 1, o.coats );
  b.write_bits( 1, o.trade_goods );
  b.write_bits( 1, o.tools );
  b.write_bits( 1, o.muskets );
  return b.good();
}

/****************************************************************
** NationInfo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, NationInfo& o ) {
  o.nation_id = b.read_n_bits<4, nation_4bit_type>();
  o.unknown14 = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, NationInfo const& o ) {
  b.write_bits( 4, o.nation_id );
  b.write_bits( 4, o.unknown14 );
  return b.good();
}

/****************************************************************
** Unknown15
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Unknown15& o ) {
  o.unknown15a = b.read_n_bits<7>();
  o.damaged = b.read_n_bits<1>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Unknown15 const& o ) {
  b.write_bits( 7, o.unknown15a );
  b.write_bits( 1, o.damaged );
  return b.good();
}

/****************************************************************
** CargoItems
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, CargoItems& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, CargoItems const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  return b.good();
}

/****************************************************************
** BoycottBitmap
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, BoycottBitmap& o ) {
  o.food = b.read_n_bits<1>();
  o.sugar = b.read_n_bits<1>();
  o.tobacco = b.read_n_bits<1>();
  o.cotton = b.read_n_bits<1>();
  o.furs = b.read_n_bits<1>();
  o.lumber = b.read_n_bits<1>();
  o.ore = b.read_n_bits<1>();
  o.silver = b.read_n_bits<1>();
  o.horses = b.read_n_bits<1>();
  o.rum = b.read_n_bits<1>();
  o.cigars = b.read_n_bits<1>();
  o.cloth = b.read_n_bits<1>();
  o.coats = b.read_n_bits<1>();
  o.trade_goods = b.read_n_bits<1>();
  o.tools = b.read_n_bits<1>();
  o.muskets = b.read_n_bits<1>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, BoycottBitmap const& o ) {
  b.write_bits( 1, o.food );
  b.write_bits( 1, o.sugar );
  b.write_bits( 1, o.tobacco );
  b.write_bits( 1, o.cotton );
  b.write_bits( 1, o.furs );
  b.write_bits( 1, o.lumber );
  b.write_bits( 1, o.ore );
  b.write_bits( 1, o.silver );
  b.write_bits( 1, o.horses );
  b.write_bits( 1, o.rum );
  b.write_bits( 1, o.cigars );
  b.write_bits( 1, o.cloth );
  b.write_bits( 1, o.coats );
  b.write_bits( 1, o.trade_goods );
  b.write_bits( 1, o.tools );
  b.write_bits( 1, o.muskets );
  return b.good();
}

/****************************************************************
** ALCS
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, ALCS& o ) {
  o.artillery_near = b.read_n_bits<1>();
  o.learned = b.read_n_bits<1>();
  o.capital = b.read_n_bits<1>();
  o.scouted = b.read_n_bits<1>();
  o.unused09 = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, ALCS const& o ) {
  b.write_bits( 1, o.artillery_near );
  b.write_bits( 1, o.learned );
  b.write_bits( 1, o.capital );
  b.write_bits( 1, o.scouted );
  b.write_bits( 4, o.unused09 );
  return b.good();
}

/****************************************************************
** TILE
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, TILE& o ) {
  o.tile = b.read_n_bits<5, terrain_5bit_type>();
  o.hill_river = b.read_n_bits<3, hills_river_3bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, TILE const& o ) {
  b.write_bits( 5, o.tile );
  b.write_bits( 3, o.hill_river );
  return b.good();
}

/****************************************************************
** MASK
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, MASK& o ) {
  o.has_unit = b.read_n_bits<1, has_unit_1bit_type>();
  o.has_city = b.read_n_bits<1, has_city_1bit_type>();
  o.suppress = b.read_n_bits<1, suppress_1bit_type>();
  o.road = b.read_n_bits<1, road_1bit_type>();
  o.purchased = b.read_n_bits<1, purchased_1bit_type>();
  o.pacific = b.read_n_bits<1, pacific_1bit_type>();
  o.plowed = b.read_n_bits<1, plowed_1bit_type>();
  o.unused = b.read_n_bits<1, suppress_1bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, MASK const& o ) {
  b.write_bits( 1, o.has_unit );
  b.write_bits( 1, o.has_city );
  b.write_bits( 1, o.suppress );
  b.write_bits( 1, o.road );
  b.write_bits( 1, o.purchased );
  b.write_bits( 1, o.pacific );
  b.write_bits( 1, o.plowed );
  b.write_bits( 1, o.unused );
  return b.good();
}

/****************************************************************
** PATH
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, PATH& o ) {
  o.region_id = b.read_n_bits<4, region_id_4bit_type>();
  o.visitor_nation = b.read_n_bits<4, nation_4bit_short_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, PATH const& o ) {
  b.write_bits( 4, o.region_id );
  b.write_bits( 4, o.visitor_nation );
  return b.good();
}

/****************************************************************
** SEEN
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, SEEN& o ) {
  o.score = b.read_n_bits<4, region_id_4bit_type>();
  o.vis2en = b.read_n_bits<1, visible_to_english_1bit_type>();
  o.vis2fr = b.read_n_bits<1, visible_to_french_1bit_type>();
  o.vis2sp = b.read_n_bits<1, visible_to_spanish_1bit_type>();
  o.vis2du = b.read_n_bits<1, visible_to_dutch_1bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, SEEN const& o ) {
  b.write_bits( 4, o.score );
  b.write_bits( 1, o.vis2en );
  b.write_bits( 1, o.vis2fr );
  b.write_bits( 1, o.vis2sp );
  b.write_bits( 1, o.vis2du );
  return b.good();
}

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop1LoadsAndUnloadsCount& o ) {
  o.unloads_count = b.read_n_bits<4>();
  o.loads_count = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop1LoadsAndUnloadsCount const& o ) {
  b.write_bits( 4, o.unloads_count );
  b.write_bits( 4, o.loads_count );
  return b.good();
}

/****************************************************************
** Stop1LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop1LoadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop1LoadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop1UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop1UnloadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop1UnloadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop2LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop2LoadsAndUnloadsCount& o ) {
  o.unloads_count = b.read_n_bits<4>();
  o.loads_count = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop2LoadsAndUnloadsCount const& o ) {
  b.write_bits( 4, o.unloads_count );
  b.write_bits( 4, o.loads_count );
  return b.good();
}

/****************************************************************
** Stop2LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop2LoadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop2LoadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop2UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop2UnloadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop2UnloadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop3LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop3LoadsAndUnloadsCount& o ) {
  o.unloads_count = b.read_n_bits<4>();
  o.loads_count = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop3LoadsAndUnloadsCount const& o ) {
  b.write_bits( 4, o.unloads_count );
  b.write_bits( 4, o.loads_count );
  return b.good();
}

/****************************************************************
** Stop3LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop3LoadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop3LoadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop3UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop3UnloadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop3UnloadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop4LoadsAndUnloadsCount
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop4LoadsAndUnloadsCount& o ) {
  o.unloads_count = b.read_n_bits<4>();
  o.loads_count = b.read_n_bits<4>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop4LoadsAndUnloadsCount const& o ) {
  b.write_bits( 4, o.unloads_count );
  b.write_bits( 4, o.loads_count );
  return b.good();
}

/****************************************************************
** Stop4LoadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop4LoadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop4LoadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** Stop4UnloadsCargo
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, Stop4UnloadsCargo& o ) {
  o.cargo_1 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_2 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_3 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_4 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_5 = b.read_n_bits<4, cargo_4bit_type>();
  o.cargo_6 = b.read_n_bits<4, cargo_4bit_type>();
  return b.good();
}

bool write_binary( base::BinaryWriter& b, Stop4UnloadsCargo const& o ) {
  b.write_bits( 4, o.cargo_1 );
  b.write_bits( 4, o.cargo_2 );
  b.write_bits( 4, o.cargo_3 );
  b.write_bits( 4, o.cargo_4 );
  b.write_bits( 4, o.cargo_5 );
  b.write_bits( 4, o.cargo_6 );
  return b.good();
}

/****************************************************************
** ExpeditionaryForce
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, ExpeditionaryForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::BinaryWriter& b, ExpeditionaryForce const& o ) {
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
bool read_binary( base::BinaryReader& b, BackupForce& o ) {
  return true
    && read_binary( b, o.regulars )
    && read_binary( b, o.dragoons )
    && read_binary( b, o.man_o_wars )
    && read_binary( b, o.artillery )
    ;
}

bool write_binary( base::BinaryWriter& b, BackupForce const& o ) {
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
bool read_binary( base::BinaryReader& b, PriceGroupState& o ) {
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

bool write_binary( base::BinaryWriter& b, PriceGroupState const& o ) {
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
bool read_binary( base::BinaryReader& b, HEAD& o ) {
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

bool write_binary( base::BinaryWriter& b, HEAD const& o ) {
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
bool read_binary( base::BinaryReader& b, PLAYER& o ) {
  return true
    && read_binary( b, o.name )
    && read_binary( b, o.country_name )
    && read_binary( b, o.unknown06 )
    && read_binary( b, o.control )
    && read_binary( b, o.founded_colonies )
    && read_binary( b, o.diplomacy )
    ;
}

bool write_binary( base::BinaryWriter& b, PLAYER const& o ) {
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
bool read_binary( base::BinaryReader& b, Tiles& o ) {
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

bool write_binary( base::BinaryWriter& b, Tiles const& o ) {
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
bool read_binary( base::BinaryReader& b, Stock& o ) {
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

bool write_binary( base::BinaryWriter& b, Stock const& o ) {
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
bool read_binary( base::BinaryReader& b, PopulationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::BinaryWriter& b, PopulationOnMap const& o ) {
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
bool read_binary( base::BinaryReader& b, FortificationOnMap& o ) {
  return true
    && read_binary( b, o.for_english )
    && read_binary( b, o.for_french )
    && read_binary( b, o.for_spanish )
    && read_binary( b, o.for_dutch )
    ;
}

bool write_binary( base::BinaryWriter& b, FortificationOnMap const& o ) {
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
bool read_binary( base::BinaryReader& b, COLONY& o ) {
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

bool write_binary( base::BinaryWriter& b, COLONY const& o ) {
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
bool read_binary( base::BinaryReader& b, TransportChain& o ) {
  return true
    && read_binary( b, o.next_unit_idx )
    && read_binary( b, o.prev_unit_idx )
    ;
}

bool write_binary( base::BinaryWriter& b, TransportChain const& o ) {
  return true
    && write_binary( b, o.next_unit_idx )
    && write_binary( b, o.prev_unit_idx )
    ;
}

/****************************************************************
** UNIT
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, UNIT& o ) {
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

bool write_binary( base::BinaryWriter& b, UNIT const& o ) {
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
bool read_binary( base::BinaryReader& b, RelationByIndian& o ) {
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

bool write_binary( base::BinaryWriter& b, RelationByIndian const& o ) {
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
bool read_binary( base::BinaryReader& b, Trade& o ) {
  return true
    && read_binary( b, o.eu_prc )
    && read_binary( b, o.nr )
    && read_binary( b, o.gold )
    && read_binary( b, o.tons )
    && read_binary( b, o.tons2 )
    ;
}

bool write_binary( base::BinaryWriter& b, Trade const& o ) {
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
bool read_binary( base::BinaryReader& b, NATION& o ) {
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

bool write_binary( base::BinaryWriter& b, NATION const& o ) {
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
bool read_binary( base::BinaryReader& b, Alarm& o ) {
  return true
    && read_binary( b, o.friction )
    && read_binary( b, o.attacks )
    ;
}

bool write_binary( base::BinaryWriter& b, Alarm const& o ) {
  return true
    && write_binary( b, o.friction )
    && write_binary( b, o.attacks )
    ;
}

/****************************************************************
** TRIBE
*****************************************************************/
// Binary conversion.
bool read_binary( base::BinaryReader& b, TRIBE& o ) {
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

bool write_binary( base::BinaryWriter& b, TRIBE const& o ) {
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
bool read_binary( base::BinaryReader& b, RelationByNations& o ) {
  return true
    && read_binary( b, o.england )
    && read_binary( b, o.france )
    && read_binary( b, o.spain )
    && read_binary( b, o.netherlands )
    ;
}

bool write_binary( base::BinaryWriter& b, RelationByNations const& o ) {
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
bool read_binary( base::BinaryReader& b, INDIAN& o ) {
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

bool write_binary( base::BinaryWriter& b, INDIAN const& o ) {
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
bool read_binary( base::BinaryReader& b, STUFF& o ) {
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

bool write_binary( base::BinaryWriter& b, STUFF const& o ) {
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
bool read_binary( base::BinaryReader& b, TRADEROUTE& o ) {
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

bool write_binary( base::BinaryWriter& b, TRADEROUTE const& o ) {
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
