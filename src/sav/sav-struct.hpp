/****************************************************************
** Classic Colonization Save File Structure.
*****************************************************************/
// NOTE: this file was auto-generated. DO NOT MODIFY!

#include <array>
#include <cstdint>
#include <vector>

/****************************************************************
** Forward Declarations.
*****************************************************************/
namespace base {

struct BinaryData;
struct BinaryData;

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

/****************************************************************
** control_type
*****************************************************************/
enum class control_type : uint8_t {
  player    = 0x00,
  ai        = 0x01,
  withdrawn = 0x02,
};

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

/****************************************************************
** fortification_level_type
*****************************************************************/
enum class fortification_level_type : uint8_t {
  none     = 0x00,
  stockade = 0x01,
  fort     = 0x02,
  fortress = 0x03,
};

/****************************************************************
** has_city_1bit_type
*****************************************************************/
enum class has_city_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  c     = 0b1,
};

/****************************************************************
** has_unit_1bit_type
*****************************************************************/
enum class has_unit_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  u     = 0b1,
};

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

/****************************************************************
** level_2bit_type
*****************************************************************/
enum class level_2bit_type : uint8_t {
  _0 = 0b00,
  _1 = 0b01,
  _2 = 0b11,
};

/****************************************************************
** level_3bit_type
*****************************************************************/
enum class level_3bit_type : uint8_t {
  _0 = 0b000,
  _1 = 0b001,
  _2 = 0b011,
  _3 = 0b111,
};

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
  awarak      = 0b0110,
  iroquois    = 0b0111,
  cherokee    = 0b1000,
  apache      = 0b1001,
  sioux       = 0b1010,
  tupi        = 0b1011,
  none        = 0b1111,
};

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
  awarak      = 0x06,
  iroquois    = 0x07,
  cherokee    = 0x08,
  apache      = 0x09,
  sioux       = 0x0A,
  tupi        = 0x0B,
  none        = 0xFF,
};

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

/****************************************************************
** orders_type
*****************************************************************/
enum class orders_type : uint8_t {
  none      = 0x00,
  sentry    = 0x01,
  trading   = 0x02,
  g0to      = 0x03,
  fortified = 0x05,
  fortify   = 0x06,
  plow      = 0x08,
  road      = 0x09,
  unknowna  = 0x0A,
  unknownb  = 0x0B,
  unknownc  = 0x0C,
};

/****************************************************************
** pacific_1bit_type
*****************************************************************/
enum class pacific_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  t     = 0b1,  // original: "~"
};

/****************************************************************
** plowed_1bit_type
*****************************************************************/
enum class plowed_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  h     = 0b1,  // original: "#"
};

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
  a_student              = 0x12,  // original: "*(Student)"
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

/****************************************************************
** purchased_1bit_type
*****************************************************************/
enum class purchased_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  a     = 0b1,  // original: "*"
};

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

/****************************************************************
** relation_type
*****************************************************************/
enum class relation_type : uint8_t {
  not_met      = 0x00,
  war          = 0x20,
  peace        = 0x60,
  unknown_rel2 = 0x64,
  unknown_rel  = 0x66,
};

/****************************************************************
** road_1bit_type
*****************************************************************/
enum class road_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  e     = 0b1,  // original: "="
};

/****************************************************************
** season_type
*****************************************************************/
enum class season_type : uint16_t {
  spring = 0x0000,
  autumn = 0x0001,
};

/****************************************************************
** suppress_1bit_type
*****************************************************************/
enum class suppress_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  _     = 0b1,
};

/****************************************************************
** tech_type
*****************************************************************/
enum class tech_type : uint8_t {
  semi_nomadic = 0x00,
  agrarian     = 0x01,
  advanced     = 0x02,
  civilized    = 0x03,
};

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
  sw  = 0b00110,  // original: "sw "
  mr  = 0b00111,  // original: "mr "
  tuf = 0b01000,
  def = 0b01001,
  plf = 0b01010,
  prf = 0b01011,
  grf = 0b01100,
  saf = 0b01101,
  swf = 0b01110,
  mrf = 0b01111,
  arc = 0b11000,
  ttt = 0b11001,  // original: "~~~"
  tnt = 0b11010,  // original: "~:~"
};

/****************************************************************
** trade_route_type
*****************************************************************/
enum class trade_route_type : uint8_t {
  land = 0x00,
  sea  = 0x01,
};

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

/****************************************************************
** visible_to_dutch_1bit_type
*****************************************************************/
enum class visible_to_dutch_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  d     = 0b1,
};

/****************************************************************
** visible_to_english_1bit_type
*****************************************************************/
enum class visible_to_english_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  e     = 0b1,
};

/****************************************************************
** visible_to_french_1bit_type
*****************************************************************/
enum class visible_to_french_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  f     = 0b1,
};

/****************************************************************
** visible_to_spanish_1bit_type
*****************************************************************/
enum class visible_to_spanish_1bit_type : uint8_t {
  empty = 0b0,  // original: " "
  s     = 0b1,
};

/****************************************************************
** GameOptions
*****************************************************************/
struct GameOptions {
  uint8_t unused01 : 7;
  bool tutorial_hints : 1;
  bool water_color_cycling : 1;
  bool combat_analysis : 1;
  bool autosave : 1;
  bool end_of_turn : 1;
  bool fast_piece_slide : 1;
  bool cheats_enabled : 1;
  bool show_foreign_moves : 1;
  bool show_indian_moves : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, GameOptions& o );
bool write_binary( base::BinaryData& b, GameOptions const& o );

/****************************************************************
** ColonyReportOptions
*****************************************************************/
struct ColonyReportOptions {
  bool labels_on_cargo_and_terrain : 1;
  bool labels_on_buildings : 1;
  bool report_new_cargos_available : 1;
  bool report_inefficient_government : 1;
  bool report_tools_needed_for_production : 1;
  bool report_raw_materials_shortages : 1;
  bool report_food_shortages : 1;
  bool report_when_colonists_trained : 1;
  bool report_sons_of_liberty_membership : 1;
  bool report_rebel_majorities : 1;
  uint8_t unused03 : 6;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, ColonyReportOptions& o );
bool write_binary( base::BinaryData& b, ColonyReportOptions const& o );

/****************************************************************
** Event
*****************************************************************/
struct Event {
  bool discovery_of_the_new_world : 1;
  bool building_a_colony : 1;
  bool meeting_the_natives : 1;
  bool the_aztec_empire : 1;
  bool the_inca_nation : 1;
  bool discovery_of_the_pacific_ocean : 1;
  bool entering_indian_village : 1;
  bool the_fountain_of_youth : 1;
  bool cargo_from_the_new_world : 1;
  bool meeting_fellow_europeans : 1;
  bool colony_burning : 1;
  bool colony_destroyed : 1;
  bool indian_raid : 1;
  bool woodcut14 : 1;
  bool woodcut15 : 1;
  bool woodcut16 : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Event& o );
bool write_binary( base::BinaryData& b, Event const& o );

/****************************************************************
** Duration
*****************************************************************/
struct Duration {
  uint8_t dur_1 : 4;
  uint8_t dur_2 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Duration& o );
bool write_binary( base::BinaryData& b, Duration const& o );

/****************************************************************
** Buildings
*****************************************************************/
struct Buildings {
  level_3bit_type fortification : 3;
  level_3bit_type armory : 3;
  level_3bit_type docks : 3;
  level_3bit_type town_hall : 3;
  level_3bit_type schoolhouse : 3;
  bool warehouse : 1;
  bool unused05a : 1;
  bool stables : 1;
  bool custom_house : 1;
  level_2bit_type printing_press : 2;
  level_3bit_type weavers_house : 3;
  level_3bit_type tobacconists_house : 3;
  level_3bit_type rum_distillers_house : 3;
  level_2bit_type capitol_unused : 2;
  level_3bit_type fur_traders_house : 3;
  level_2bit_type carpenters_shop : 2;
  level_2bit_type church : 2;
  level_3bit_type blacksmiths_house : 3;
  uint8_t unused05b : 6;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Buildings& o );
bool write_binary( base::BinaryData& b, Buildings const& o );

/****************************************************************
** CustomHouseFlags
*****************************************************************/
struct CustomHouseFlags {
  bool food : 1;
  bool sugar : 1;
  bool tobacco : 1;
  bool cotton : 1;
  bool furs : 1;
  bool lumber : 1;
  bool ore : 1;
  bool silver : 1;
  bool horses : 1;
  bool rum : 1;
  bool cigars : 1;
  bool cloth : 1;
  bool coats : 1;
  bool trade_goods : 1;
  bool tools : 1;
  bool muskets : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, CustomHouseFlags& o );
bool write_binary( base::BinaryData& b, CustomHouseFlags const& o );

/****************************************************************
** NationInfo
*****************************************************************/
struct NationInfo {
  nation_4bit_type nation_id : 4;
  bool vis_to_english : 1;
  bool vis_to_french : 1;
  bool vis_to_spanish : 1;
  bool vis_to_dutch : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, NationInfo& o );
bool write_binary( base::BinaryData& b, NationInfo const& o );

/****************************************************************
** Unknown15
*****************************************************************/
struct Unknown15 {
  uint8_t unknown15a : 7;
  bool damaged : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Unknown15& o );
bool write_binary( base::BinaryData& b, Unknown15 const& o );

/****************************************************************
** CargoItems
*****************************************************************/
struct CargoItems {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, CargoItems& o );
bool write_binary( base::BinaryData& b, CargoItems const& o );

/****************************************************************
** BoycottBitmap
*****************************************************************/
struct BoycottBitmap {
  bool food : 1;
  bool sugar : 1;
  bool tobacco : 1;
  bool cotton : 1;
  bool furs : 1;
  bool lumber : 1;
  bool ore : 1;
  bool silver : 1;
  bool horses : 1;
  bool rum : 1;
  bool cigars : 1;
  bool cloth : 1;
  bool coats : 1;
  bool trade_goods : 1;
  bool tools : 1;
  bool muskets : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, BoycottBitmap& o );
bool write_binary( base::BinaryData& b, BoycottBitmap const& o );

/****************************************************************
** ALCS
*****************************************************************/
struct ALCS {
  bool artillery_near : 1;
  bool learned : 1;
  bool capital : 1;
  bool scouted : 1;
  uint8_t unused09 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, ALCS& o );
bool write_binary( base::BinaryData& b, ALCS const& o );

/****************************************************************
** TILE
*****************************************************************/
struct TILE {
  terrain_5bit_type tile : 5;
  hills_river_3bit_type hill_river : 3;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, TILE& o );
bool write_binary( base::BinaryData& b, TILE const& o );

/****************************************************************
** MASK
*****************************************************************/
struct MASK {
  has_unit_1bit_type has_unit : 1;
  has_city_1bit_type has_city : 1;
  suppress_1bit_type suppress : 1;
  road_1bit_type road : 1;
  purchased_1bit_type purchased : 1;
  pacific_1bit_type pacific : 1;
  plowed_1bit_type plowed : 1;
  suppress_1bit_type unused : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, MASK& o );
bool write_binary( base::BinaryData& b, MASK const& o );

/****************************************************************
** PATH
*****************************************************************/
struct PATH {
  region_id_4bit_type region_id : 4;
  nation_4bit_short_type visitor_nation : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, PATH& o );
bool write_binary( base::BinaryData& b, PATH const& o );

/****************************************************************
** SEEN
*****************************************************************/
struct SEEN {
  region_id_4bit_type score : 4;
  visible_to_english_1bit_type vis2en : 1;
  visible_to_french_1bit_type vis2fr : 1;
  visible_to_spanish_1bit_type vis2sp : 1;
  visible_to_dutch_1bit_type vis2du : 1;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, SEEN& o );
bool write_binary( base::BinaryData& b, SEEN const& o );

/****************************************************************
** Stop1LoadsAndUnloadsCount
*****************************************************************/
struct Stop1LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1LoadsAndUnloadsCount& o );
bool write_binary( base::BinaryData& b, Stop1LoadsAndUnloadsCount const& o );

/****************************************************************
** Stop1LoadsCargo
*****************************************************************/
struct Stop1LoadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1LoadsCargo& o );
bool write_binary( base::BinaryData& b, Stop1LoadsCargo const& o );

/****************************************************************
** Stop1UnloadsCargo
*****************************************************************/
struct Stop1UnloadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop1UnloadsCargo& o );
bool write_binary( base::BinaryData& b, Stop1UnloadsCargo const& o );

/****************************************************************
** Stop2LoadsAndUnloadsCount
*****************************************************************/
struct Stop2LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2LoadsAndUnloadsCount& o );
bool write_binary( base::BinaryData& b, Stop2LoadsAndUnloadsCount const& o );

/****************************************************************
** Stop2LoadsCargo
*****************************************************************/
struct Stop2LoadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2LoadsCargo& o );
bool write_binary( base::BinaryData& b, Stop2LoadsCargo const& o );

/****************************************************************
** Stop2UnloadsCargo
*****************************************************************/
struct Stop2UnloadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop2UnloadsCargo& o );
bool write_binary( base::BinaryData& b, Stop2UnloadsCargo const& o );

/****************************************************************
** Stop3LoadsAndUnloadsCount
*****************************************************************/
struct Stop3LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3LoadsAndUnloadsCount& o );
bool write_binary( base::BinaryData& b, Stop3LoadsAndUnloadsCount const& o );

/****************************************************************
** Stop3LoadsCargo
*****************************************************************/
struct Stop3LoadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3LoadsCargo& o );
bool write_binary( base::BinaryData& b, Stop3LoadsCargo const& o );

/****************************************************************
** Stop3UnloadsCargo
*****************************************************************/
struct Stop3UnloadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop3UnloadsCargo& o );
bool write_binary( base::BinaryData& b, Stop3UnloadsCargo const& o );

/****************************************************************
** Stop4LoadsAndUnloadsCount
*****************************************************************/
struct Stop4LoadsAndUnloadsCount {
  uint8_t unloads_count : 4;
  uint8_t loads_count : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4LoadsAndUnloadsCount& o );
bool write_binary( base::BinaryData& b, Stop4LoadsAndUnloadsCount const& o );

/****************************************************************
** Stop4LoadsCargo
*****************************************************************/
struct Stop4LoadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4LoadsCargo& o );
bool write_binary( base::BinaryData& b, Stop4LoadsCargo const& o );

/****************************************************************
** Stop4UnloadsCargo
*****************************************************************/
struct Stop4UnloadsCargo {
  cargo_4bit_type cargo_1 : 4;
  cargo_4bit_type cargo_2 : 4;
  cargo_4bit_type cargo_3 : 4;
  cargo_4bit_type cargo_4 : 4;
  cargo_4bit_type cargo_5 : 4;
  cargo_4bit_type cargo_6 : 4;
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Stop4UnloadsCargo& o );
bool write_binary( base::BinaryData& b, Stop4UnloadsCargo const& o );

/****************************************************************
** ExpeditionaryForce
*****************************************************************/
struct ExpeditionaryForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, ExpeditionaryForce& o );
bool write_binary( base::BinaryData& b, ExpeditionaryForce const& o );

/****************************************************************
** BackupForce
*****************************************************************/
struct BackupForce {
  uint16_t regulars = {};
  uint16_t dragoons = {};
  uint16_t man_o_wars = {};
  uint16_t artillery = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, BackupForce& o );
bool write_binary( base::BinaryData& b, BackupForce const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, PriceGroupState& o );
bool write_binary( base::BinaryData& b, PriceGroupState const& o );

/****************************************************************
** HEAD
*****************************************************************/
struct HEAD {
  std::array<uint8_t, 9> colonize = {};
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
  uint64_t nation_relation = {};
  std::array<uint8_t, 10> unknown45 = {};
  ExpeditionaryForce expeditionary_force = {};
  BackupForce backup_force = {};
  PriceGroupState price_group_state = {};
  Event event = {};
  uint16_t unknown05 = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, HEAD& o );
bool write_binary( base::BinaryData& b, HEAD const& o );

/****************************************************************
** PLAYER
*****************************************************************/
struct PLAYER {
  std::array<uint8_t, 24> name = {};
  std::array<uint8_t, 24> country_name = {};
  uint8_t unknown06 = {};
  control_type control = {};
  uint8_t founded_colonies = {};
  uint8_t diplomacy = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, PLAYER& o );
bool write_binary( base::BinaryData& b, PLAYER const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, Tiles& o );
bool write_binary( base::BinaryData& b, Tiles const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, Stock& o );
bool write_binary( base::BinaryData& b, Stock const& o );

/****************************************************************
** PopulationOnMap
*****************************************************************/
struct PopulationOnMap {
  uint8_t for_english = {};
  uint8_t for_french = {};
  uint8_t for_spanish = {};
  uint8_t for_dutch = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, PopulationOnMap& o );
bool write_binary( base::BinaryData& b, PopulationOnMap const& o );

/****************************************************************
** FortificationOnMap
*****************************************************************/
struct FortificationOnMap {
  fortification_level_type for_english = {};
  fortification_level_type for_french = {};
  fortification_level_type for_spanish = {};
  fortification_level_type for_dutch = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, FortificationOnMap& o );
bool write_binary( base::BinaryData& b, FortificationOnMap const& o );

/****************************************************************
** COLONY
*****************************************************************/
struct COLONY {
  std::array<uint8_t, 2> x_y = {};
  std::array<uint8_t, 24> name = {};
  nation_type nation_id = {};
  uint32_t unknown08 = {};
  uint8_t population = {};
  std::array<occupation_type, 32> occupation = {};
  std::array<profession_type, 32> profession = {};
  std::array<Duration, 16> duration = {};
  Tiles tiles = {};
  std::array<uint8_t, 12> unknown10 = {};
  Buildings buildings = {};
  CustomHouseFlags custom_house_flags = {};
  std::array<uint8_t, 6> unknown11 = {};
  uint16_t hammers = {};
  uint8_t building_in_production = {};
  uint8_t warehouse_level = {};
  uint32_t unknown12 = {};
  Stock stock = {};
  PopulationOnMap population_on_map = {};
  FortificationOnMap fortification_on_map = {};
  uint32_t rebel_dividend = {};
  uint32_t rebel_divisor = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, COLONY& o );
bool write_binary( base::BinaryData& b, COLONY const& o );

/****************************************************************
** TransportChain
*****************************************************************/
struct TransportChain {
  int16_t next_unit_idx = {};
  int16_t prev_unit_idx = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, TransportChain& o );
bool write_binary( base::BinaryData& b, TransportChain const& o );

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
  uint8_t unknown16b = {};
  orders_type orders = {};
  uint8_t goto_x = {};
  uint8_t goto_y = {};
  uint8_t unknown18 = {};
  uint8_t holds_occupied = {};
  std::array<CargoItems, 3> cargo_items = {};
  std::array<uint8_t, 6> cargo_hold = {};
  uint8_t turns_worked = {};
  profession_type profession_or_treasure_amount = {};
  TransportChain transport_chain = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, UNIT& o );
bool write_binary( base::BinaryData& b, UNIT const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, RelationByIndian& o );
bool write_binary( base::BinaryData& b, RelationByIndian const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, Trade& o );
bool write_binary( base::BinaryData& b, Trade const& o );

/****************************************************************
** NATION
*****************************************************************/
struct NATION {
  uint8_t unknown19 = {};
  uint8_t tax_rate = {};
  std::array<profession_type, 3> recruit = {};
  uint8_t unused07 = {};
  uint8_t recruit_count = {};
  uint32_t founding_fathers = {};
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
  uint32_t unknown24b = {};
  int32_t gold = {};
  uint16_t current_crosses = {};
  uint16_t needed_crosses = {};
  std::array<uint8_t, 2> point_return_from_europe = {};
  uint32_t unknown25b = {};
  RelationByIndian relation_by_indian = {};
  uint32_t unknown26a = {};
  uint16_t unknown26b = {};
  std::array<uint8_t, 6> unknown26c = {};
  Trade trade = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, NATION& o );
bool write_binary( base::BinaryData& b, NATION const& o );

/****************************************************************
** Alarm
*****************************************************************/
struct Alarm {
  uint8_t friction = {};
  uint8_t attacks = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, Alarm& o );
bool write_binary( base::BinaryData& b, Alarm const& o );

/****************************************************************
** TRIBE
*****************************************************************/
struct TRIBE {
  std::array<uint8_t, 2> x_y = {};
  nation_type nation_id = {};
  ALCS alcs = {};
  uint8_t population = {};
  uint8_t mission = {};
  uint16_t unknown28 = {};
  uint8_t last_bought = {};
  uint8_t last_sold = {};
  std::array<Alarm, 4> alarm = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, TRIBE& o );
bool write_binary( base::BinaryData& b, TRIBE const& o );

/****************************************************************
** RelationByNations
*****************************************************************/
struct RelationByNations {
  relation_type england = {};
  relation_type france = {};
  relation_type spain = {};
  relation_type netherlands = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, RelationByNations& o );
bool write_binary( base::BinaryData& b, RelationByNations const& o );

/****************************************************************
** INDIAN
*****************************************************************/
struct INDIAN {
  std::array<uint8_t, 2> capitol_x_y = {};
  tech_type tech = {};
  uint32_t unknown31a = {};
  uint8_t muskets = {};
  uint8_t horse_herds = {};
  std::array<uint8_t, 5> unknown31b = {};
  std::array<int16_t, 16> tons = {};
  std::array<uint8_t, 12> unknown32 = {};
  RelationByNations relation_by_nations = {};
  uint64_t unknown33 = {};
  std::array<uint16_t, 4> alarm_by_player = {};
};

// Binary conversion.
bool read_binary( base::BinaryData& b, INDIAN& o );
bool write_binary( base::BinaryData& b, INDIAN const& o );

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

// Binary conversion.
bool read_binary( base::BinaryData& b, STUFF& o );
bool write_binary( base::BinaryData& b, STUFF const& o );

/****************************************************************
** TRADEROUTE
*****************************************************************/
struct TRADEROUTE {
  std::array<uint8_t, 32> name = {};
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

// Binary conversion.
bool read_binary( base::BinaryData& b, TRADEROUTE& o );
bool write_binary( base::BinaryData& b, TRADEROUTE const& o );

/****************************************************************
** ColonySAV
*****************************************************************/
struct ColonySAV {
  HEAD head = {};
  std::array<PLAYER, 4> player = {};
  std::array<uint8_t, 24> other = {};
  std::vector<COLONY> colony = {};
  std::vector<UNIT> unit = {};
  std::array<NATION, 4> nation = {};
  std::vector<TRIBE> tribe = {};
  std::array<INDIAN, 8> indian = {};
  STUFF stuff = {};
  std::vector<TILE> tile = {};
  std::vector<MASK> mask = {};
  std::vector<PATH> path = {};
  std::vector<SEEN> seen = {};
  std::array<std::array<uint8_t, 18>, 14> unknown_map38a = {};
  std::array<std::array<uint8_t, 18>, 14> unknown_map38b = {};
  std::array<uint8_t, 104> unknown39a = {};
  std::array<uint8_t, 3> unknown39b = {};
  uint16_t prime_resource_seed = {};
  uint8_t unknown39d = {};
  std::array<TRADEROUTE, 12> trade_route = {};
};

}  // namespace sav
