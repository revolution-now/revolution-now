/****************************************************************
**serial.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-28.
*
* Description: Serialization.
*
*****************************************************************/
#include "serial.hpp"

// Revolution Now
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "logging.hpp"

// Flatbuffers
#include "fb/testing_generated.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

using namespace std;

namespace rn {

namespace {

template<int N>
struct disambiguate;

struct UclArchiver {
  // All types that have a `serialize` member function.
  template<typename ObjectType, disambiguate<0>* = nullptr>
  auto save( string_view field_name, ObjectType const& o )
      -> void_t<decltype( o.serialize( *this ) )> {
    print_line_prefix( field_name );
    result += fmt::format( "{{\n" );
    level++;
    o.serialize( *this );
    level--;
    result += fmt::format( "{}}}\n", spaces(), field_name );
  }

  void save( string_view field_name, char n ) {
    print_line_prefix( field_name );
    result += fmt::format( "'{}'\n", n );
  }

  void save( string_view field_name, int n ) {
    print_line_prefix( field_name );
    result += fmt::format( "{}\n", n );
  }

  void save( string_view field_name, double d ) {
    print_line_prefix( field_name );
    result += fmt::format( "{}\n", d );
  }

  void save( string_view field_name, string const& s ) {
    print_line_prefix( field_name );
    result += fmt::format( "\"{}\"\n", s );
  }

  // Types for which there is a `serialize` free function found
  // by ADL.
  template<typename ADLserializable, disambiguate<1>* = nullptr>
  auto save( string_view            field_name,
             ADLserializable const& serializable )
      -> void_t<decltype( serialize( *this, serializable ) )> {
    print_line_prefix( field_name );
    result += fmt::format( "{{\n" );
    level++;
    serialize( *this, serializable );
    level--;
    result += fmt::format( "{}}}\n", spaces() );
  }

  // Vectors of serializable types.
  template<typename T>
  auto save( string_view field_name, Vec<T> const& v )
      -> void_t<decltype( save( field_name, v[0] ) )> {
    print_line_prefix( field_name );
    result +=
        fmt::format( "[\n", spaces(), field_name, v.size() );
    level++;
    for( auto const& e : v ) save( "", e );
    level--;
    result += fmt::format( "{}]\n", spaces() );
  }

  // Should not serialize pointers.
  template<typename T>
  void save( string_view field_name, T* ) = delete;

  // Better-enums.
  template<typename ReflectedEnum>
  auto save( string_view field_name, ReflectedEnum const& e )
      -> void_t<typename ReflectedEnum::_enumerated> {
    print_line_prefix( field_name );
    result += fmt::format( "\"{}\"\n", e );
  }

  void print_line_prefix( string_view field_name ) {
    string if_not_empty =
        field_name.empty() ? ""
                           : fmt::format( "{}: ", field_name );
    result += fmt::format( "{}{}", spaces(), if_not_empty );
  }

  string spaces() const { return string( level * 2, ' ' ); }

  int    level{0};
  string result;
};

} // namespace

/****************************************************************
** Testing
*****************************************************************/
void write_monster( FILE* fp ) {
  flatbuffers::FlatBufferBuilder builder;

  auto  weapon_one_name   = builder.CreateString( "Sword" );
  short weapon_one_damage = 3;
  auto  weapon_two_name   = builder.CreateString( "Axe" );
  short weapon_two_damage = 5;
  // Use the `CreateWeapon` shortcut to create Weapons with all
  // the fields set.
  auto sword = MyGame::CreateWeapon( builder, weapon_one_name,
                                     weapon_one_damage );
  auto axe   = MyGame::CreateWeapon( builder, weapon_two_name,
                                   weapon_two_damage );

  // Serialize a name for our monster, called "Orc".
  auto name = builder.CreateString( "Orc" );
  // Create a `vector` representing the inventory of the Orc.
  // Each number could correspond to an item that can be claimed
  // after he is slain.
  unsigned char treasure[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  auto          inventory = builder.CreateVector( treasure, 10 );

  // Place the weapons into a `std::vector`, then convert that
  // into a FlatBuffer `vector`.
  vector<flatbuffers::Offset<MyGame::Weapon>> weapons_vector;
  weapons_vector.push_back( sword );
  weapons_vector.push_back( axe );
  auto weapons = builder.CreateVector( weapons_vector );

  MyGame::Vec3 points[] = {MyGame::Vec3( 1.0f, 2.0f, 3.0f ),
                           MyGame::Vec3( 4.0f, 5.0f, 6.0f )};
  auto         path = builder.CreateVectorOfStructs( points, 2 );

  // Create the position struct
  auto position = MyGame::Vec3( 1.0f, 2.0f, 3.0f );
  // Set his hit points to 300 and his mana to 150.
  int hp   = 300;
  int mana = 150;
  // Finally, create the monster using the `CreateMonster` helper
  // function to set all fields.
  auto orc = MyGame::CreateMonster(
      builder, &position, mana, hp, name, inventory,
      MyGame::Color::Red, weapons, MyGame::Equipment::Weapon,
      axe.Union(), path );

  // You can use this code instead of `CreateMonster()`, to
  // create our orc manually.
  // MyGame::MonsterBuilder monster_builder( builder );
  // monster_builder.add_pos( &position );
  // monster_builder.add_hp( hp );
  // monster_builder.add_name( name );
  // monster_builder.add_inventory( inventory );
  // monster_builder.add_color( MyGame::Color::Red );
  // monster_builder.add_weapons( weapons );
  // monster_builder.add_equipped_type( MyGame::Equipment::Weapon
  // ); monster_builder.add_equipped( axe.Union() ); auto orc =
  // monster_builder.Finish();

  // Call `Finish()` to instruct the builder that this monster is
  // complete. Note: Regardless of how you created the `orc`, you
  // still need to call `Finish()` on the `FlatBufferBuilder`.
  builder.Finish( orc );

  // This must be called after `Finish()`.
  uint8_t* buf  = builder.GetBufferPointer();
  int      size = builder.GetSize();
  ::fwrite( buf, 1, size, fp );
}

void read_monster( string_view file ) {
  CHECK( fs::exists( file ) );
  auto size = fs::file_size( file );

  auto fp = ::fopen( string( file ).c_str(), "rb" );
  CHECK( fp );

  auto buffer = unique_ptr<uint8_t>( new uint8_t[size] );
  CHECK_EQ( ::fread( buffer.get(), 1, size, fp ),
            size_t( size ) );

  // Get a pointer to the root object inside the buffer.
  auto& monster = *MyGame::GetMonster( buffer.get() );

  lg.info( "monster.hp():    {}", monster.hp() );
  lg.info( "monster.mana():  {}", monster.mana() );
  lg.info( "monster.name():  {}", monster.name()->c_str() );
  CHECK( monster.pos() != nullptr );
  lg.info( "monster.pos.x(): {}", monster.pos()->x() );
  lg.info( "monster.pos.y(): {}", monster.pos()->y() );
  lg.info( "monster.pos.z(): {}", monster.pos()->z() );

  auto inv = monster.inventory();
  lg.info( "inv->size(): {}", inv->size() );
  lg.info( "inv->Get(2): {}", inv->Get( 2 ) );

  auto weapons = monster.weapons();
  lg.info( "weapon_len:           {}", weapons->size() );
  lg.info( "second_weapon_name:   {}",
           weapons->Get( 1 )->name()->str() );
  lg.info( "second_weapon_damage: {}",
           weapons->Get( 1 )->damage() );

  flatbuffers::Parser parser;

  char const* include_dirs[] = {"src/fb", nullptr};
  string      root           = R"(
    include "monster.fbs";
    root_type MyGame.Monster;
  )";
  CHECK( parser.Parse( root.c_str(), include_dirs ) );

  string output;
  CHECK( flatbuffers::GenerateText( parser, buffer.get(),
                                    &output ) );

  lg.info( "Flatbuffers text output:\n{}", output );
}

void test_serial() {
  // == Direct ==================================================
  // SaveableOmni omni;
  // UclArchiver  ar;
  // ar.save( "Parent", omni );
  // lg.info( "result:\n{}", ar.result );

  // == Flatbuffers =============================================
  auto fp = ::fopen( "fb.out", "wb" );
  CHECK( fp );
  write_monster( fp );
  ::fclose( fp );
  read_monster( "fb.out" );
}

} // namespace rn
