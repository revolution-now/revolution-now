/****************************************************************
**flatbuffers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-03.
*
* Description: Unit tests for Flatbuffers integration.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "enum.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "io.hpp"
#include "logging.hpp"
#include "serial.hpp"
#include "unit.hpp"
#include "ustate.hpp"

// base-util
#include "base-util/io.hpp"

// Flatbuffers
#include "fb/testing_generated.h"
#include "flatbuffers/flatbuffers.h"

// C++ standard library
#include <fstream>
#include <map>

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::CargoSlot_t );
FMT_TO_CATCH( ::rn::UnitId );
FMT_TO_CATCH( ::rn::Commodity );

namespace rn {
adt_s_rn( MyAdt,                //
          ( none ),             //
          ( some,               //
            ( std::string, s ), //
            ( int, y ) ),       //
          ( more,               //
            ( double, d ) )     //
);
}

namespace testing {
namespace {

using namespace std;
using namespace rn;

using ::Catch::Contains;
using ::Catch::Equals;
using ::rn::serial::BinaryBlob;

enum class e_( color, Red, Green, Blue );
SERIALIZABLE_ENUM( e_color );

struct Weapon {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  bool operator==( Weapon const& rhs ) const {
    return name == rhs.name && damage == rhs.damage;
  }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Weapon,
  ( string, name    ),
  ( short,  damage  ));
  // clang-format on
};

struct Vec2 {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  bool operator<( Vec2 const& rhs ) const {
    if( x < rhs.x ) return true;
    if( x > rhs.x ) return false;
    return y < rhs.y;
  }

  bool operator==( Vec2 const& rhs ) const {
    return x == rhs.x && y == rhs.y;
  }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, Vec2 const& v ) {
    return H::combine( std::move( h ), v.x, v.y );
  }

  // clang-format off
  SERIALIZABLE_STRUCT_MEMBERS( Vec2,
  ( float, x ),
  ( float, y ));
  // clang-format on
};

struct Vec3 {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  bool operator==( Vec3 const& rhs ) const {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }

  // clang-format off
  SERIALIZABLE_STRUCT_MEMBERS( Vec3,
  ( float, x ),
  ( float, y ),
  ( float, z ));
  // clang-format on
};

struct Monster {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  using pair_s_i_t = pair<string, int>;
  using pair_v_i_t = pair<Vec2, int>;
  using map_vecs_t = map<Vec2, int>;
  using map_strs_t = map<string, int>;
  using map_wpns_t = map<int, Weapon>;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Monster,
  ( Vec3,            pos            ),
  ( short,           mana           ),
  ( short,           hp             ),
  ( string,          name           ),
  ( vector<string>,  names          ),
  ( vector<uint8_t>, inventory      ),
  ( e_color,         color          ),
  ( vector<Weapon>,  weapons        ),
  ( vector<Vec3>,    path           ),
  ( pair_s_i_t,      pair1          ),
  ( pair_v_i_t,      pair2          ),
  ( map_vecs_t,      map_vecs       ),
  ( map_strs_t,      map_strs       ),
  ( map_wpns_t,      map_wpns       ));
  // clang-format on
};

BinaryBlob create_monster_blob() {
  FBBuilder builder;

  auto  weapon_one_name   = builder.CreateString( "Sword" );
  short weapon_one_damage = 3;
  auto  weapon_two_name   = builder.CreateString( "Axe" );
  short weapon_two_damage = 5;

  auto sword = fb::CreateWeapon( builder, weapon_one_name,
                                 weapon_one_damage );
  auto axe   = fb::CreateWeapon( builder, weapon_two_name,
                               weapon_two_damage );

  auto name = builder.CreateString( "Orc" );

  vector<FBOffset<flatbuffers::String>> names_v;
  names_v.push_back( builder.CreateString( "hello1" ) );
  names_v.push_back( builder.CreateString( "hello2" ) );
  names_v.push_back( builder.CreateString( "hello3" ) );
  auto names = builder.CreateVector( names_v );

  unsigned char treasure[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto          inventory = builder.CreateVector( treasure, 10 );

  vector<FBOffset<fb::Weapon>> weapons_vector;
  weapons_vector.push_back( sword );
  weapons_vector.push_back( axe );
  auto weapons = builder.CreateVector( weapons_vector );

  fb::Vec3 points[] = {fb::Vec3( 1.0f, 2.0f, 3.0f ),
                       fb::Vec3( 4.0f, 5.0f, 6.0f )};
  auto     path     = builder.CreateVectorOfStructs( points, 2 );

  auto position = fb::Vec3( 1.0f, 2.0f, 3.0f );

  int hp   = 300;
  int mana = 150;

  auto pair1 = fb::CreatePair_string_int(
      builder, builder.CreateString( "hello" ), 42 );

  auto pair2 = fb::Pair_Vec2_int( fb::Vec2( 7.0, 8.0 ), 43 );

  Vec<fb::Pair_Vec2_int>             map_vecs;
  Vec<FBOffset<fb::Pair_string_int>> map_strs;
  Vec<FBOffset<fb::Pair_int_Weapon>> map_wpns;

  map_vecs.push_back(
      fb::Pair_Vec2_int( fb::Vec2( 8.0, 9.0 ), 10 ) );
  map_vecs.push_back(
      fb::Pair_Vec2_int( fb::Vec2( 9.0, 10.0 ), 11 ) );

  map_strs.push_back( fb::CreatePair_string_int(
      builder, builder.CreateString( "blue" ), 5 ) );
  map_strs.push_back( fb::CreatePair_string_int(
      builder, builder.CreateString( "red" ), 4 ) );

  map_wpns.push_back( fb::CreatePair_int_Weapon(
      builder, -1,
      fb::CreateWeapon( builder, builder.CreateString( "knife" ),
                        30 ) ) );
  map_wpns.push_back( fb::CreatePair_int_Weapon(
      builder, 0,
      fb::CreateWeapon( builder, builder.CreateString( "Gun" ),
                        3000 ) ) );
  auto fb_map_vecs = builder.CreateVectorOfStructs( map_vecs );
  auto fb_map_strs = builder.CreateVector( map_strs );
  auto fb_map_wpns = builder.CreateVector( map_wpns );

  auto orc = fb::CreateMonster(
      builder, &position, mana, hp, name, names, inventory,
      fb::e_color::Red, weapons, path, pair1, &pair2,
      fb_map_vecs, fb_map_strs, fb_map_wpns );

  builder.Finish( orc );

  return BinaryBlob::from_builder( std::move( builder ) );
}

TEST_CASE( "[flatbuffers] monster: serialize to blob" ) {
  auto tmp_file = fs::temp_directory_path() / "flatbuffers.out";
  constexpr uint64_t kExpectedBlobSize = 476;
  auto               json_file = data_dir() / "monster.json";

  SECTION( "create/serialize" ) {
    auto blob = create_monster_blob();
    REQUIRE( blob.size() == kExpectedBlobSize );

    auto json = rn::serial::blob_to_json<Monster>( blob );
    ASSIGN_CHECK_XP( json_golden,
                     rn::read_file_as_string( json_file ) );
    REQUIRE( json == json_golden );

    CHECK_XP( blob.write( tmp_file.c_str() ) );
    REQUIRE( fs::file_size( tmp_file ) == kExpectedBlobSize );
  }

  SECTION( "deserialize to flatbuffer" ) {
    ASSIGN_CHECK_XP( blob, BinaryBlob::read( tmp_file ) );
    REQUIRE( blob.size() == kExpectedBlobSize );

    auto json = blob.to_json<fb::Monster>();
    ASSIGN_CHECK_XP( json_golden,
                     rn::read_file_as_string( json_file ) );
    REQUIRE( json == json_golden );

    // Get a pointer to the root object inside the buffer.
    auto& monster =
        *flatbuffers::GetRoot<fb::Monster>( blob.get() );

    REQUIRE( monster.hp() == 300 );
    REQUIRE( monster.mana() == 150 );

    REQUIRE( monster.name() != nullptr );
    REQUIRE( monster.name()->str() == "Orc" );

    REQUIRE( monster.names() != nullptr );
    REQUIRE( monster.names()->Get( 0 )->str() == "hello1" );
    REQUIRE( monster.names()->Get( 1 )->str() == "hello2" );
    REQUIRE( monster.names()->Get( 2 )->str() == "hello3" );

    REQUIRE( monster.pos() != nullptr );
    REQUIRE( monster.pos()->x() == 1.0 );
    REQUIRE( monster.pos()->y() == 2.0 );
    REQUIRE( monster.pos()->z() == 3.0 );

    auto inv = monster.inventory();
    REQUIRE( inv != nullptr );
    REQUIRE( inv->size() == 10 );
    REQUIRE( inv->Get( 2 ) == 2 );

    auto pair1 = monster.pair1();
    REQUIRE( pair1 != nullptr );
    REQUIRE( pair1->fst() != nullptr );
    REQUIRE( pair1->fst()->str() == "hello" );
    REQUIRE( pair1->snd() == 42 );
    auto pair2 = monster.pair2();
    REQUIRE( pair2->fst().x() == 7.0 );
    REQUIRE( pair2->fst().y() == 8.0 );
    REQUIRE( pair2->snd() == 43 );
    auto weapons = monster.weapons();
    REQUIRE( weapons != nullptr );
    REQUIRE( weapons->size() == 2 );
    auto fst = weapons->Get( 0 );
    REQUIRE( fst->name() != nullptr );
    REQUIRE( fst->name()->str() == "Sword" );
    REQUIRE( fst->damage() == 3 );
    auto snd = weapons->Get( 1 );
    REQUIRE( snd->name() != nullptr );
    REQUIRE( snd->name()->str() == "Axe" );
    REQUIRE( snd->damage() == 5 );

    auto map_vecs = monster.map_vecs();
    REQUIRE( map_vecs != nullptr );
    auto map_strs = monster.map_strs();
    REQUIRE( map_strs != nullptr );
    auto map_wpns = monster.map_wpns();
    REQUIRE( map_wpns != nullptr );

    REQUIRE( map_vecs->size() == 2 );
    REQUIRE( map_strs->size() == 2 );
    REQUIRE( map_wpns->size() == 2 );

    REQUIRE( map_vecs->Get( 0 )->fst().x() == 8.0 );
    REQUIRE( map_vecs->Get( 0 )->fst().y() == 9.0 );
    REQUIRE( map_vecs->Get( 0 )->snd() == 10 );
    REQUIRE( map_vecs->Get( 1 )->fst().x() == 9.0 );
    REQUIRE( map_vecs->Get( 1 )->fst().y() == 10.0 );
    REQUIRE( map_vecs->Get( 1 )->snd() == 11 );

    REQUIRE( map_strs->Get( 0 )->fst()->str() == "blue" );
    REQUIRE( map_strs->Get( 0 )->snd() == 5 );
    REQUIRE( map_strs->Get( 1 )->fst()->str() == "red" );
    REQUIRE( map_strs->Get( 1 )->snd() == 4 );

    REQUIRE( map_wpns->Get( 0 )->fst() == -1 );
    REQUIRE( map_wpns->Get( 0 )->snd()->name()->str() ==
             "knife" );
    REQUIRE( map_wpns->Get( 0 )->snd()->damage() == 30 );
    REQUIRE( map_wpns->Get( 1 )->fst() == 0 );
    REQUIRE( map_wpns->Get( 1 )->snd()->name()->str() == "Gun" );
    REQUIRE( map_wpns->Get( 1 )->snd()->damage() == 3000 );
  }

  SECTION( "deserialize to native" ) {
    ASSIGN_CHECK_XP( blob, BinaryBlob::read( tmp_file ) );
    REQUIRE( blob.size() == kExpectedBlobSize );

    Monster monster;
    CHECK_XP(
        rn::serial::deserialize_from_blob( blob, &monster ) );

    REQUIRE( monster.hp == 300 );
    REQUIRE( monster.mana == 150 );

    REQUIRE( monster.name == "Orc" );

    REQUIRE( monster.names.size() == 3 );
    REQUIRE( monster.names[0] == "hello1" );
    REQUIRE( monster.names[1] == "hello2" );
    REQUIRE( monster.names[2] == "hello3" );

    REQUIRE( monster.pos.x == 1.0 );
    REQUIRE( monster.pos.y == 2.0 );
    REQUIRE( monster.pos.z == 3.0 );

    auto const& inv = monster.inventory;
    REQUIRE( inv.size() == 10 );
    REQUIRE( inv[2] == 2 );

    auto const& pair1 = monster.pair1;
    REQUIRE( pair1.first == "hello" );
    REQUIRE( pair1.second == 42 );

    auto const& pair2 = monster.pair2;
    REQUIRE( pair2.first.x == 7.0 );
    REQUIRE( pair2.first.y == 8.0 );
    REQUIRE( pair2.second == 43 );

    auto const& weapons = monster.weapons;
    REQUIRE( weapons.size() == 2 );
    auto fst = weapons[0];
    REQUIRE( fst.name == "Sword" );
    REQUIRE( fst.damage == 3 );
    auto snd = weapons[1];
    REQUIRE( snd.name == "Axe" );
    REQUIRE( snd.damage == 5 );

    auto& map_vecs = monster.map_vecs;
    auto& map_strs = monster.map_strs;
    auto& map_wpns = monster.map_wpns;

    REQUIRE( map_vecs.size() == 2 );
    REQUIRE( map_strs.size() == 2 );
    REQUIRE( map_wpns.size() == 2 );

    REQUIRE( map_vecs[Vec2{8.0, 9.0}] == 10 );
    REQUIRE( map_vecs[Vec2{9.0, 10.0}] == 11 );

    REQUIRE( map_strs["blue"] == 5 );
    REQUIRE( map_strs["red"] == 4 );

    REQUIRE( map_wpns[-1] == Weapon{"knife", 30} );
    REQUIRE( map_wpns[0] == Weapon{"Gun", 3000} );
  }

  SECTION( "native to native roundtrip" ) {
    Monster monster;
    monster.pos       = Vec3{2.25, 3.5, 4.5};
    monster.mana      = 9;
    monster.hp        = 200;
    monster.name      = "mon";
    monster.names     = {"A", "B"};
    monster.inventory = {7, 6, 5, 4};
    monster.color     = e_color::Red;
    monster.weapons   = Vec<Weapon>{
        Weapon{"rock", 2}, //
        Weapon{"stone", 3} //
    };
    monster.path  = {{3, 4.5, 5}, {4, 5.6, 5}, {7, 8.9, 5}};
    monster.pair1 = pair<string, int>( "primo", 2 );
    monster.pair2 = pair<Vec2, int>( Vec2{0.25, 0.5}, 3 );
    monster.map_vecs[Vec2{4.75, 8}] = 0;
    monster.map_vecs[Vec2{4.25, 7}] = 1;
    monster.map_strs["one"]         = -1;
    monster.map_strs["two"]         = -2;
    monster.map_wpns[3]             = Weapon{"rock", 2};
    monster.map_wpns[4]             = Weapon{"stone", 4};

    auto blob = rn::serial::serialize_to_blob( monster );
    constexpr uint64_t kExpectedBlobSize = 460;
    REQUIRE( blob.size() == kExpectedBlobSize );

    auto json = rn::serial::serialize_to_json( monster );
    ASSIGN_CHECK_XP(
        json_golden,
        rn::read_file_as_string( data_dir() /
                                 "monster-round-trip.json" ) );
    REQUIRE( json == json_golden );

    Monster monster_new;
    CHECK_XP( rn::serial::deserialize_from_blob(
        blob, &monster_new ) );

    REQUIRE( monster_new.pos.x == 2.25 );
    REQUIRE( monster_new.pos.y == 3.5 );
    REQUIRE( monster_new.pos.z == 4.5 );
    REQUIRE( monster_new.mana == 9 );
    REQUIRE( monster_new.hp == 200 );
    REQUIRE( monster_new.name == "mon" );
    REQUIRE( monster_new.names.size() == 2 );
    REQUIRE( monster_new.names[0] == "A" );
    REQUIRE( monster_new.names[1] == "B" );

    auto const& inv = monster_new.inventory;
    REQUIRE( inv.size() == 4 );
    REQUIRE( inv[0] == 7 );
    REQUIRE( inv[1] == 6 );
    REQUIRE( inv[2] == 5 );
    REQUIRE( inv[3] == 4 );

    REQUIRE( monster_new.color == e_color::Red );

    auto const& weapons = monster_new.weapons;
    REQUIRE( weapons.size() == 2 );
    REQUIRE( weapons[0].name == "rock" );
    REQUIRE( weapons[0].damage == 2 );
    REQUIRE( weapons[1].name == "stone" );
    REQUIRE( weapons[1].damage == 3 );

    auto const& p = monster_new.path;
    REQUIRE( p.size() == 3 );
    REQUIRE( p[0] == Vec3{3, 4.5, 5} );
    REQUIRE( p[1] == Vec3{4, 5.6, 5} );
    REQUIRE( p[2] == Vec3{7, 8.9, 5} );

    auto const& pair1 = monster_new.pair1;
    REQUIRE( pair1.first == "primo" );
    REQUIRE( pair1.second == 2 );

    auto const& pair2 = monster_new.pair2;
    REQUIRE( pair2.first.x == 0.25 );
    REQUIRE( pair2.first.y == 0.5 );
    REQUIRE( pair2.second == 3 );

    auto& map_vecs = monster_new.map_vecs;
    auto& map_strs = monster_new.map_strs;
    auto& map_wpns = monster_new.map_wpns;

    REQUIRE( map_vecs.size() == 2 );
    REQUIRE( map_strs.size() == 2 );
    REQUIRE( map_wpns.size() == 2 );

    REQUIRE( map_vecs[Vec2{4.75, 8}] == 0 );
    REQUIRE( map_vecs[Vec2{4.25, 7}] == 1 );

    REQUIRE( map_strs["one"] == -1 );
    REQUIRE( map_strs["two"] == -2 );

    REQUIRE( map_wpns[3] == Weapon{"rock", 2} );
    REQUIRE( map_wpns[4] == Weapon{"stone", 4} );
  }
}

TEST_CASE( "deserialize json" ) {
  testing_only::reset_unit_creation();

  (void)create_unit( e_nation::english,
                     e_unit_type::merchantman )
      .id();
  (void)create_unit( e_nation::english,
                     e_unit_type::free_colonist )
      .id();
  (void)create_unit( e_nation::english, e_unit_type::soldier )
      .id();

  ASSIGN_CHECK_XP( json, rn::read_file_as_string(
                             data_dir() / "unit.json" ) );
  Unit unit;
  CHECK_XP( rn::serial::deserialize_from_json(
      /*schema_name=*/"unit",
      /*json=*/json, /*out=*/&unit ) );

  REQUIRE( unit.id() == UnitId{1} );
  REQUIRE( unit.desc().type == rn::e_unit_type::merchantman );
  REQUIRE( unit.orders() == rn::e_unit_orders::none );
  REQUIRE( unit.nation() == rn::e_nation::english );
  REQUIRE( unit.worth() == nullopt );
  REQUIRE( unit.movement_points() == rn::MovementPoints( 5 ) );
  REQUIRE( unit.finished_turn() == false );

  auto& cargo = unit.cargo();
  REQUIRE( cargo.slots_total() == 4 );
  REQUIRE( cargo.slots()[0] ==
           rn::CargoSlot_t{rn::CargoSlot::empty{}} );
  REQUIRE( cargo.slots()[1] ==
           rn::CargoSlot_t{
               rn::CargoSlot::cargo{/*contents=*/UnitId{2}}} );
  REQUIRE( cargo.slots()[2] ==
           rn::CargoSlot_t{
               rn::CargoSlot::cargo{/*contents=*/UnitId{3}}} );
  REQUIRE(
      cargo.slots()[3] ==
      rn::CargoSlot_t{rn::CargoSlot::cargo{
          /*contents=*/Commodity{/*type=*/rn::e_commodity::food,
                                 /*quantity=*/100}}} );
}

TEST_CASE( "[flatbuffers] serialize Unit" ) {
  testing_only::reset_unit_creation();

  auto ship =
      create_unit( e_nation::english, e_unit_type::merchantman )
          .id();
  auto unit_id2 = create_unit( e_nation::english,
                               e_unit_type::free_colonist )
                      .id();
  auto unit_id3 =
      create_unit( e_nation::english, e_unit_type::soldier )
          .id();
  auto comm1 = Commodity{/*type=*/e_commodity::food,
                         /*quantity=*/100};

  add_commodity_to_cargo( comm1, ship, 3,
                          /*try_other_slots=*/false );
  rn::ustate_change_to_cargo( ship, unit_id2, 1 );
  rn::ustate_change_to_cargo( ship, unit_id3, 2 );

  auto tmp_file = fs::temp_directory_path() / "flatbuffers.out";
  constexpr uint64_t kExpectedBlobSize = 240;
  auto               json_file = data_dir() / "unit.json";

  SECTION( "create/serialize" ) {
    FBBuilder builder;
    auto      ship_offset =
        unit_from_id( ship ).serialize_table( builder );
    builder.Finish( ship_offset );
    auto blob = BinaryBlob::from_builder( std::move( builder ) );

    REQUIRE( blob.size() == kExpectedBlobSize );

    auto json = blob.to_json<fb::Unit>();
    ASSIGN_CHECK_XP( json_golden,
                     rn::read_file_as_string( json_file ) );
    INFO( json );
    REQUIRE( json == json_golden );

    CHECK_XP( blob.write( tmp_file.c_str() ) );
    REQUIRE( fs::file_size( tmp_file ) == kExpectedBlobSize );
  }

  SECTION( "read from blob" ) {
    ASSIGN_CHECK_XP( blob, BinaryBlob::read( tmp_file ) );
    REQUIRE( blob.size() == kExpectedBlobSize );

    // Get a pointer to the root object inside the buffer.
    auto& unit = *flatbuffers::GetRoot<fb::Unit>( blob.get() );

    auto const& ship_unit = rn::unit_from_id( ship );

    REQUIRE( unit.id_() == ship._ );
    REQUIRE( static_cast<int>( unit.type_() ) ==
             ship_unit.desc().type._value );
    REQUIRE( static_cast<int>( unit.orders_() ) ==
             ship_unit.orders()._value );
    REQUIRE( static_cast<int>( unit.nation_() ) ==
             ship_unit.nation()._value );
    REQUIRE( unit.worth_() != nullptr );
    REQUIRE( unit.worth_()->has_value() == false );
    // REQUIRE( unit.mv_pts() == ship_unit.movement_points() );
    REQUIRE( unit.finished_turn_() ==
             ship_unit.finished_turn() );

    REQUIRE( unit.cargo_() != nullptr );
    auto cargo = unit.cargo_();
    REQUIRE( cargo != nullptr );
    auto slots = cargo->slots_();
    REQUIRE( slots != nullptr );
    REQUIRE( slots->size() == 4 );
    REQUIRE( slots->Get( 0 ) != nullptr );
    REQUIRE( slots->Get( 1 ) != nullptr );
    REQUIRE( slots->Get( 2 ) != nullptr );
    REQUIRE( slots->Get( 3 ) != nullptr );

    REQUIRE( slots->Get( 0 )->empty() != nullptr );
    REQUIRE( slots->Get( 0 )->overflow() == nullptr );
    REQUIRE( slots->Get( 0 )->cargo() == nullptr );

    REQUIRE( slots->Get( 1 )->empty() == nullptr );
    REQUIRE( slots->Get( 1 )->overflow() == nullptr );
    REQUIRE( slots->Get( 1 )->cargo() != nullptr );

    auto c  = slots->Get( 1 )->cargo();
    auto cc = c->contents();
    REQUIRE( cc != nullptr );
    REQUIRE( cc->is_unit() );
    REQUIRE( cc->unit_id() == 2 );
    REQUIRE( cc->commodity() == nullptr );

    REQUIRE( slots->Get( 2 )->empty() == nullptr );
    REQUIRE( slots->Get( 2 )->overflow() == nullptr );
    REQUIRE( slots->Get( 2 )->cargo() != nullptr );

    c  = slots->Get( 2 )->cargo();
    cc = c->contents();
    REQUIRE( cc != nullptr );
    REQUIRE( cc->is_unit() );
    REQUIRE( cc->unit_id() == 3 );
    REQUIRE( cc->commodity() == nullptr );

    REQUIRE( slots->Get( 3 )->empty() == nullptr );
    REQUIRE( slots->Get( 3 )->overflow() == nullptr );
    REQUIRE( slots->Get( 3 )->cargo() != nullptr );

    c  = slots->Get( 3 )->cargo();
    cc = c->contents();
    REQUIRE( cc != nullptr );
    REQUIRE_FALSE( cc->is_unit() );
    REQUIRE( cc->unit_id() == 0 );
    REQUIRE( cc->commodity() != nullptr );

    auto comm = *cc->commodity();

    REQUIRE( comm.type() == fb::e_commodity::food );
    REQUIRE( comm.quantity() == 100 );
  }

  SECTION( "deserialize unit to json" ) {
    ASSIGN_CHECK_XP( blob, BinaryBlob::read( tmp_file ) );
    auto json = blob.to_json<fb::Unit>();
    ASSIGN_CHECK_XP( json_golden,
                     rn::read_file_as_string( json_file ) );
    INFO( json );
    REQUIRE( json == json_golden );
  }

  SECTION( "deserialize unit" ) {
    ASSIGN_CHECK_XP( blob, BinaryBlob::read( tmp_file ) );
    REQUIRE( blob.size() == kExpectedBlobSize );

    Unit unit;
    CHECK_XP( rn::serial::deserialize_from_blob( blob, &unit ) );

    auto const& orig = rn::unit_from_id( ship );

    REQUIRE( unit.id() == orig.id() );
    REQUIRE( unit.desc().type == orig.desc().type );
    REQUIRE( unit.orders() == orig.orders() );
    REQUIRE( unit.nation() == orig.nation() );
    REQUIRE( unit.worth() == orig.worth() );
    REQUIRE( unit.movement_points() == orig.movement_points() );
    REQUIRE( unit.finished_turn() == orig.finished_turn() );

    REQUIRE( unit.units_in_cargo().has_value() );
    REQUIRE( orig.units_in_cargo().has_value() );

    REQUIRE_THAT( *unit.units_in_cargo(),
                  Equals( *orig.units_in_cargo() ) );
    REQUIRE( unit.finished_turn() == orig.finished_turn() );
    REQUIRE( unit.moved_this_turn() == orig.moved_this_turn() );
    REQUIRE( unit.orders_mean_move_needed() ==
             orig.orders_mean_move_needed() );
    REQUIRE( unit.orders_mean_input_required() ==
             orig.orders_mean_input_required() );

    auto& cargo      = unit.cargo();
    auto& orig_cargo = orig.cargo();

    REQUIRE( cargo.slots_occupied() ==
             orig_cargo.slots_occupied() );
    REQUIRE( cargo.slots_remaining() ==
             orig_cargo.slots_remaining() );
    REQUIRE( cargo.slots_total() == orig_cargo.slots_total() );

    REQUIRE( cargo.count_items() == orig_cargo.count_items() );
    REQUIRE( cargo.count_items_of_type<UnitId>() ==
             orig_cargo.count_items_of_type<UnitId>() );
    REQUIRE( cargo.count_items_of_type<Commodity>() ==
             orig_cargo.count_items_of_type<Commodity>() );
    REQUIRE( cargo.items_of_type<UnitId>() ==
             orig_cargo.items_of_type<UnitId>() );
    REQUIRE( cargo.items_of_type<Commodity>() ==
             orig_cargo.items_of_type<Commodity>() );
    REQUIRE( cargo.max_commodity_per_cargo_slot() ==
             orig_cargo.max_commodity_per_cargo_slot() );
    REQUIRE( cargo.slots() == orig_cargo.slots() );
  }
}

struct MapTester1 {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }
  using map_t = FlatMap<string, int>;
  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, MapTester1,
  ( map_t,        map                ));
  // clang-format on
};

struct MapTester2 {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }
  using map_t = unordered_map<int, int>;
  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, MapTester2,
  ( map_t,        map                ));
  // clang-format on
};

TEST_CASE( "[flatbuffers] hash maps" ) {
  using namespace ::rn::serial;

  MapTester1 m1{{{"one", 1}, {"two", 2}}};
  MapTester2 m2{{{2, 1}, {3, 2}}};

  auto m1_blob = rn::serial::serialize_to_blob( m1 );
  auto m2_blob = rn::serial::serialize_to_blob( m2 );

  MapTester1 m1_new;
  CHECK_XP( deserialize_from_blob( m1_blob, &m1_new ) );
  MapTester2 m2_new;
  CHECK_XP( deserialize_from_blob( m2_blob, &m2_new ) );

  REQUIRE( m1.map == m1_new.map );
  REQUIRE( m2.map == m2_new.map );

  // Make sure that deserializing a map with duplicate keys re-
  // sults in an error.
  MapTester1 m_dup;
  ASSIGN_CHECK_XP( json, rn::read_file_as_string(
                             data_dir() / "map-dup-key.json" ) );
  auto xp = rn::serial::deserialize_from_json(
      /*schema_name=*/"testing",
      /*json=*/json, /*out=*/&m_dup );

  REQUIRE( !xp.has_value() );
  REQUIRE_THAT( xp.error().what, Contains( "duplicate key" ) );
  REQUIRE_THAT( xp.error().what, Contains( "string" ) );
}

TEST_CASE( "[flatbuffers] ADTs" ) {
  using namespace ::rn::serial;
  MyAdt::none n;
  MyAdt::some s{"hello", 7};
  MyAdt::more m{5.5};

  MyAdt_t v;

  SECTION( "none" ) { v = n; }
  SECTION( "some" ) { v = s; }
  SECTION( "more" ) { v = m; }

  FBBuilder fbb;

  auto v_offset = serialize<::fb::MyAdt_t>(
      fbb, v, ::rn::serial::rn_adl_tag{} );

  fbb.Finish( v_offset.get() );
  auto blob = BinaryBlob::from_builder( std::move( fbb ) );

  auto* root = flatbuffers::GetRoot<::fb::MyAdt_t>( blob.get() );

  MyAdt_t d_v;
  REQUIRE(
      deserialize( root, &d_v, ::rn::serial::rn_adl_tag{} ) ==
      rn::xp_success_t{} );

  REQUIRE( v == d_v );
}

} // namespace
} // namespace testing
