/****************************************************************
** Classic Colonization Save File Structure.
*****************************************************************/
// NOTE: this file was auto-generated. DO NOT MODIFY!

#include <array>
#include <cstdint>
#include <vector>

namespace sav {

/****************************************************************
** GameOptions
*****************************************************************/
struct GameOptions {
  uint8_t unused01 : 7;
  uint8_t tutorial_hints : 1;
  uint8_t water_color_cycling : 1;
  uint8_t combat_analysis : 1;
  uint8_t autosave : 1;
  uint8_t end_of_turn : 1;
  uint8_t fast_piece_slide : 1;
  uint8_t cheats_enabled : 1;
  uint8_t show_foreign_moves : 1;
  uint8_t show_indian_moves : 1;
};

/****************************************************************
** ColonyReportOptions
*****************************************************************/
struct ColonyReportOptions {
  uint8_t labels_on_cargo_and_terrain : 1;
  uint8_t labels_on_buildings : 1;
  uint8_t report_new_cargos_available : 1;
  uint8_t report_inefficient_government : 1;
  uint8_t report_tools_needed_for_production : 1;
  uint8_t report_raw_materials_shortages : 1;
  uint8_t report_food_shortages : 1;
  uint8_t report_when_colonists_trained : 1;
  uint8_t report_sons_of_liberty_membership : 1;
  uint8_t report_rebel_majorities : 1;
  uint8_t unused03 : 6;
};

/****************************************************************
** Event
*****************************************************************/
struct Event {
  uint8_t discovery_of_the_new_world : 1;
  uint8_t building_a_colony : 1;
  uint8_t meeting_the_natives : 1;
  uint8_t the_aztec_empire : 1;
  uint8_t the_inca_nation : 1;
  uint8_t discovery_of_the_pacific_ocean : 1;
  uint8_t entering_indian_village : 1;
  uint8_t the_fountain_of_youth : 1;
  uint8_t cargo_from_the_new_world : 1;
  uint8_t meeting_fellow_europeans : 1;
  uint8_t colony_burning : 1;
  uint8_t colony_destroyed : 1;
  uint8_t indian_raid : 1;
  uint8_t woodcut14 : 1;
  uint8_t woodcut15 : 1;
  uint8_t woodcut16 : 1;
};

/****************************************************************
** Duration
*****************************************************************/
struct Duration {
  uint8_t dur_1 : 4;
  uint8_t dur_2 : 4;
};

/****************************************************************
** Buildings
*****************************************************************/
struct Buildings {
  uint8_t fortification : 3;
  uint8_t armory : 3;
  uint8_t docks : 3;
  uint8_t town_hall : 3;
  uint8_t schoolhouse : 3;
  uint8_t warehouse : 1;
  uint8_t unused05a : 1;
  uint8_t stables : 1;
  uint8_t custom_house : 1;
  uint8_t printing_press : 2;
  uint8_t weavers_house : 3;
  uint8_t tobacconists_house : 3;
  uint8_t rum_distillers_house : 3;
  uint8_t capitol__unused_ : 2;
  uint8_t fur_traders_house : 3;
  uint8_t carpenters_shop : 2;
  uint8_t church : 2;
  uint8_t blacksmiths_house : 3;
  uint8_t unused05b : 6;
};

/****************************************************************
** CustomHouseFlags
*****************************************************************/
struct CustomHouseFlags {
  uint8_t food : 1;
  uint8_t sugar : 1;
  uint8_t tobacco : 1;
  uint8_t cotton : 1;
  uint8_t furs : 1;
  uint8_t lumber : 1;
  uint8_t ore : 1;
  uint8_t silver : 1;
  uint8_t horses : 1;
  uint8_t rum : 1;
  uint8_t cigars : 1;
  uint8_t cloth : 1;
  uint8_t coats : 1;
  uint8_t trade_goods : 1;
  uint8_t tools : 1;
  uint8_t muskets : 1;
};

/****************************************************************
** NationInfo
*****************************************************************/
struct NationInfo {
  uint8_t nation_id : 4;
  uint8_t unknown14 : 4;
};

/****************************************************************
** Unknown15
*****************************************************************/
struct Unknown15 {
  uint8_t unknown15a : 7;
  uint8_t damaged : 1;
};

/****************************************************************
** CargoItems
*****************************************************************/
struct CargoItems {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
};

/****************************************************************
** BoycottBitmap
*****************************************************************/
struct BoycottBitmap {
  uint8_t food : 1;
  uint8_t sugar : 1;
  uint8_t tobacco : 1;
  uint8_t cotton : 1;
  uint8_t furs : 1;
  uint8_t lumber : 1;
  uint8_t ore : 1;
  uint8_t silver : 1;
  uint8_t horses : 1;
  uint8_t rum : 1;
  uint8_t cigars : 1;
  uint8_t cloth : 1;
  uint8_t coats : 1;
  uint8_t trade_goods : 1;
  uint8_t tools : 1;
  uint8_t muskets : 1;
};

/****************************************************************
** ALCS
*****************************************************************/
struct ALCS {
  uint8_t artillery_near : 1;
  uint8_t learned : 1;
  uint8_t capital : 1;
  uint8_t scouted : 1;
  uint8_t unused09 : 4;
};

/****************************************************************
** TILE
*****************************************************************/
struct TILE {
  uint8_t tile : 5;
  uint8_t hill_river : 3;
};

/****************************************************************
** MASK
*****************************************************************/
struct MASK {
  uint8_t has_unit : 1;
  uint8_t has_city : 1;
  uint8_t suppress : 1;
  uint8_t road : 1;
  uint8_t purchased : 1;
  uint8_t pacific : 1;
  uint8_t plowed : 1;
  uint8_t unused : 1;
};

/****************************************************************
** PATH
*****************************************************************/
struct PATH {
  uint8_t region_id : 4;
  uint8_t visitor_nation : 4;
};

/****************************************************************
** SEEN
*****************************************************************/
struct SEEN {
  uint8_t score : 4;
  uint8_t vis2en : 1;
  uint8_t vis2fr : 1;
  uint8_t vis2sp : 1;
  uint8_t vis2du : 1;
};

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
struct Stop1LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

/****************************************************************
** Stop1LoadsCargo
*****************************************************************/
struct Stop1LoadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop1UnloadsCargo
*****************************************************************/
struct Stop1UnloadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop2LoadsAndUnloadsCount
*****************************************************************/
struct Stop2LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

/****************************************************************
** Stop2LoadsCargo
*****************************************************************/
struct Stop2LoadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop2UnloadsCargo
*****************************************************************/
struct Stop2UnloadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop3LoadsAndUnloadsCount
*****************************************************************/
struct Stop3LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

/****************************************************************
** Stop3LoadsCargo
*****************************************************************/
struct Stop3LoadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop3UnloadsCargo
*****************************************************************/
struct Stop3UnloadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop4LoadsAndUnloadsCount
*****************************************************************/
struct Stop4LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

/****************************************************************
** Stop4LoadsCargo
*****************************************************************/
struct Stop4LoadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** Stop4UnloadsCargo
*****************************************************************/
struct Stop4UnloadsCargo {
  uint8_t cargo_1 : 4;
  uint8_t cargo_2 : 4;
  uint8_t cargo_3 : 4;
  uint8_t cargo_4 : 4;
  uint8_t cargo_5 : 4;
  uint8_t cargo_6 : 4;
};

/****************************************************************
** ExpeditionaryForce
*****************************************************************/
struct ExpeditionaryForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};
};

/****************************************************************
** BackupForce
*****************************************************************/
struct BackupForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};
};

/****************************************************************
** PriceGroupState
*****************************************************************/
struct PriceGroupState {
  uint16_t food = {};
  uint16_t sugar = {};
  uint16_t tobacco = {};
  uint16_t cotton = {};
  uint16_t furs = {};
  uint16_t lumber = {};
  uint16_t ore = {};
  uint16_t silver = {};
  uint16_t horses = {};
  uint16_t rum = {};
  uint16_t cigars = {};
  uint16_t cloth = {};
  uint16_t coats = {};
  uint16_t trade_goods = {};
  uint16_t tools = {};
  uint16_t muskets = {};
};

/****************************************************************
** HEAD
*****************************************************************/
struct HEAD {
  char colonize[9] = {};
  std::array<uint8_t, 3> unknown00 = {};
  uint16_t map_size_x = {};
  uint16_t map_size_y = {};
  uint8_t tut1 = {};
  uint8_t unknown03 = {};
  GameOptions game_options = {};
  ColonyReportOptions colony_report_options = {};
  uint8_t tut2 = {};
  uint8_t tut3 = {};
  uint16_t unknown39 = {};
  uint16_t year = {};
  season_type season = {};
  uint16_t turn = {};
  uint8_t tile_selection_mode = {};
  uint8_t unknown40 = {};
  uint16_t active_unit = {};
  std::array<uint8_t, 6> unknown41 = {};
  uint16_t tribe_count = {};
  uint16_t unit_count = {};
  uint16_t colony_count = {};
  uint8_t trade_route_count = {};
  std::array<uint8_t, 5> unknown42 = {};
  difficulty_type difficulty = {};
  uint8_t unknown43a = {};
  uint8_t unknown43b = {};
  std::array<uint8_t, 25> founding_father = {};
  std::array<uint8_t, 6> unknown44 = {};
  std::array<uint8_t, 8> nation_relation = {};
  std::array<uint8_t, 10> unknown45 = {};
  ExpeditionaryForce expeditionary_force = {};
  BackupForce backup_force = {};
  PriceGroupState price_group_state = {};
  Event event = {};
  uint16_t unknown05 = {};
};

/****************************************************************
** PLAYER
*****************************************************************/
struct PLAYER {
  char name[24] = {};
  char country_name[24] = {};
  uint8_t unknown06 = {};
  control_type control = {};
  uint8_t founded_colonies = {};
  uint8_t diplomacy = {};
};

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
};

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
};

/****************************************************************
** PopulationOnMap
*****************************************************************/
struct PopulationOnMap {
  uint8_t for_english = {};
  uint8_t for_french = {};
  uint8_t for_spanish = {};
  uint8_t for_dutch = {};
};

/****************************************************************
** FortificationOnMap
*****************************************************************/
struct FortificationOnMap {
  fortification_level_type for_english = {};
  fortification_level_type for_french = {};
  fortification_level_type for_spanish = {};
  fortification_level_type for_dutch = {};
};

/****************************************************************
** COLONY
*****************************************************************/
struct COLONY {
  std::array<uint8_t, 2> x__y = {};
  char name[24] = {};
  nation_type nation_id = {};
  std::array<uint8_t, 4> unknown08 = {};
  uint8_t population = {};
  std::array<occupation_type, 32> occupation = {};
  std::array<profession_type, 32> profession = {};
  std::vector<Duration> duration = {};
  Tiles tiles = {};
  std::array<uint8_t, 12> unknown10 = {};
  Buildings buildings = {};
  CustomHouseFlags custom_house_flags = {};
  std::array<uint8_t, 6> unknown11 = {};
  uint16_t hammers = {};
  uint8_t building_in_production = {};
  uint8_t warehouse_level = {};
  std::array<uint8_t, 4> unknown12 = {};
  Stock stock = {};
  PopulationOnMap population_on_map = {};
  FortificationOnMap fortification_on_map = {};
  std::array<uint8_t, 4> rebel_dividend = {};
  std::array<uint8_t, 4> rebel_divisor = {};
};

/****************************************************************
** TransportChain
*****************************************************************/
struct TransportChain {
  int16_t next_unit_idx = {};
  int16_t prev_unit_idx = {};
};

/****************************************************************
** UNIT
*****************************************************************/
struct UNIT {
  std::array<uint8_t, 2> x__y = {};
  unit_type type = {};
  NationInfo nation_info = {};
  Unknown15 unknown15 = {};
  uint8_t moves = {};
  uint16_t unknown16 = {};
  orders_type orders = {};
  uint8_t goto_x = {};
  uint8_t goto_y = {};
  uint8_t unknown18 = {};
  uint8_t holds_occupied = {};
  std::vector<CargoItems> cargo_items = {};
  std::array<uint8_t, 6> cargo_hold = {};
  uint8_t turns_worked = {};
  profession_type profession_or_treasure_amount = {};
  TransportChain transport_chain = {};
};

/****************************************************************
** RelationByIndian
*****************************************************************/
struct RelationByIndian {
  relation_type inca = {};
  relation_type aztec = {};
  relation_type awarak = {};
  relation_type iroquois = {};
  relation_type cherokee = {};
  relation_type apache = {};
  relation_type sioux = {};
  relation_type tupi = {};
};

/****************************************************************
** Trade
*****************************************************************/
struct Trade {
  std::array<uint8_t, 16> eu_prc = {};
  std::array<int16_t, 16> nr = {};
  std::array<int32_t, 16> gold = {};
  std::array<int32_t, 16> tons = {};
  std::array<int32_t, 16> tons2 = {};
};

/****************************************************************
** NATION
*****************************************************************/
struct NATION {
  uint8_t unknown19 = {};
  uint8_t tax_rate = {};
  std::array<profession_type, 3> recruit = {};
  uint8_t unused07 = {};
  uint8_t recruit_count = {};
  std::array<uint8_t, 4> founding_fathers = {};
  uint8_t unknown21 = {};
  uint16_t liberty_bells_total = {};
  uint16_t liberty_bells_last_turn = {};
  uint16_t unknown22 = {};
  uint16_t next_founding_father = {};
  uint16_t founding_father_count = {};
  uint16_t prob_founding_father_count_end = {};
  uint8_t villages_burned = {};
  std::array<uint8_t, 5> unknown23 = {};
  uint16_t artillery_bought_count = {};
  BoycottBitmap boycott_bitmap = {};
  int32_t royal_money = {};
  std::array<uint8_t, 4> unknown24b = {};
  int32_t gold = {};
  uint16_t current_crosses = {};
  uint16_t needed_crosses = {};
  uint16_t unknown25a = {};
  std::array<uint8_t, 4> unknown25b = {};
  RelationByIndian relation_by_indian = {};
  std::array<uint8_t, 4> unknown26a = {};
  uint16_t unknown26b = {};
  std::array<uint8_t, 6> unknown26c = {};
  Trade trade = {};
};

/****************************************************************
** Alarm
*****************************************************************/
struct Alarm {
  uint8_t friction = {};
  uint8_t attacks = {};
};

/****************************************************************
** TRIBE
*****************************************************************/
struct TRIBE {
  std::array<uint8_t, 2> x__y = {};
  nation_type nation_id = {};
  ALCS alcs = {};
  uint8_t population = {};
  uint8_t mission = {};
  uint16_t unknown28 = {};
  uint8_t last_bought = {};
  uint8_t last_sold = {};
  std::array<Alarm, 4> alarm = {};
};

/****************************************************************
** RelationByNations
*****************************************************************/
struct RelationByNations {
  relation_type england = {};
  relation_type france = {};
  relation_type spain = {};
  relation_type netherlands = {};
};

/****************************************************************
** INDIAN
*****************************************************************/
struct INDIAN {
  std::array<uint8_t, 2> capitol__x__y_ = {};
  tech_type tech = {};
  std::array<uint8_t, 4> unknown31a = {};
  uint8_t muskets = {};
  uint8_t horse_herds = {};
  std::array<uint8_t, 5> unknown31b = {};
  std::array<int16_t, 16> tons = {};
  std::array<uint8_t, 12> unknown32 = {};
  RelationByNations relation_by_nations = {};
  std::array<uint8_t, 8> unknown33 = {};
  std::array<uint16_t, 4> alarm_by_player = {};
};

/****************************************************************
** STUFF
*****************************************************************/
struct STUFF {
  std::array<uint8_t, 15> unknown34 = {};
  uint16_t counter_decreasing_on_new_colony = {};
  uint16_t unknown35 = {};
  uint16_t counter_increasing_on_new_colony = {};
  std::array<uint8_t, 696> unknown36 = {};
  uint16_t x = {};
  uint16_t y = {};
  uint8_t zoom_level = {};
  uint8_t unknown37 = {};
  uint16_t viewport_x = {};
  uint16_t viewport_y = {};
};

/****************************************************************
** TRADEROUTE
*****************************************************************/
struct TRADEROUTE {
  char name[32] = {};
  trade_route_type land_or_sea = {};
  uint8_t stops_count = {};
  uint16_t stop_1_colony_index = {};
  Stop1LoadsAndUnloadsCount stop_1_loads_and_unloads_count = {};
  Stop1LoadsCargo stop_1_loads_cargo = {};
  Stop1UnloadsCargo stop_1_unloads_cargo = {};
  uint8_t unknown47 = {};
  uint16_t stop_2_colony_index = {};
  Stop2LoadsAndUnloadsCount stop_2_loads_and_unloads_count = {};
  Stop2LoadsCargo stop_2_loads_cargo = {};
  Stop2UnloadsCargo stop_2_unloads_cargo = {};
  uint8_t unknown48 = {};
  uint16_t stop_3_colony_index = {};
  Stop3LoadsAndUnloadsCount stop_3_loads_and_unloads_count = {};
  Stop3LoadsCargo stop_3_loads_cargo = {};
  Stop3UnloadsCargo stop_3_unloads_cargo = {};
  uint8_t unknown49 = {};
  uint16_t stop_4_colony_index = {};
  Stop4LoadsAndUnloadsCount stop_4_loads_and_unloads_count = {};
  Stop4LoadsCargo stop_4_loads_cargo = {};
  Stop4UnloadsCargo stop_4_unloads_cargo = {};
  uint8_t unknown50 = {};
};

/****************************************************************
** ColonySav
*****************************************************************/
struct ColonySav {
  HEAD head = {};
  std::array<PLAYER, 4> player = {};
  std::array<uint8_t, 24> other = {};
  std::vector<COLONY> colony = {};
  std::vector<unit_type> unit = {};
  std::array<NATION, 4> nation = {};
  std::vector<TRIBE> tribe = {};
  std::array<INDIAN, 8> indian = {};
  STUFF stuff = {};
  std::vector<TILE> tile = {};
  std::vector<MASK> mask = {};
  std::vector<PATH> path = {};
  std::vector<SEEN> seen = {};
  std::array<uint8_t, 14> unknown_map38a = {};
  std::array<uint8_t, 14> unknown_map38b = {};
  std::array<uint8_t, 104> unknown39a = {};
  std::array<uint8_t, 3> unknown39b = {};
  uint16_t prime_resource_seed = {};
  uint8_t unknown39d = {};
  std::array<TRADEROUTE, 12> trade_route = {};
};

}  // namespace sav
