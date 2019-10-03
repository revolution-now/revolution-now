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

// Proto
#include <google/protobuf/text_format.h>
#include "testing.pb.h"

// CapnProto
#include <capnp/compat/json.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize.h>
#include "kj/filesystem.h"
#include "testing.capnp.h"

// Flatbuffers
#include "fb/testing_generated.h"
#include "flatbuffers/flatbuffers.h"

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
// void writeAddressBook( int fd ) {
void writeAddressBook( FILE* fp ) {
  capnp::MallocMessageBuilder message;

  AddressBook::Builder addressBook =
      message.initRoot<AddressBook>();
  capnp::List<Person>::Builder people =
      addressBook.initPeople( 2 );

  Person::Builder alice = people[0];
  alice.setId( 123 );
  alice.setName( "Alice" );
  alice.setEmail( "alice@example.com" );
  // Type shown for explanation purposes; normally you'd use
  // auto.
  capnp::List<Person::PhoneNumber>::Builder alicePhones =
      alice.initPhones( 1 );
  alicePhones[0].setNumber( "555-1212" );
  alicePhones[0].setType( Person::PhoneNumber::Type::MOBILE );
  alice.getEmployment().setSchool( "MIT" );

  Person::Builder bob = people[1];
  bob.setId( 456 );
  bob.setName( "Bob" );
  bob.setEmail( "bob@example.com" );
  auto bobPhones = bob.initPhones( 2 );
  bobPhones[0].setNumber( "555-4567" );
  bobPhones[0].setType( Person::PhoneNumber::Type::HOME );
  bobPhones[1].setNumber( "555-7654" );
  bobPhones[1].setType( Person::PhoneNumber::Type::WORK );
  bob.getEmployment().setUnemployed();

  // writePackedMessageToFd( fd, message );
  // writeMessageToFd( fd, message );

  capnp::JsonCodec tcodec;
  tcodec.setPrettyPrint( true );

  auto str = tcodec.encode( addressBook );
  ::fwrite( str.cStr(), 1, str.size(), fp );
}

void printAddressBook( int fd ) {
  // capnp::PackedFdMessageReader message( fd );
  // capnp::StreamFdMessageReader message( fd );

  capnp::MallocMessageBuilder message;
  capnp::JsonCodec            json;

  auto readable =
      kj::newDiskReadableFile( kj::AutoCloseFd( fd ) );
  auto orphan = json.decode<AddressBook>(
      readable->readAllText(), message.getOrphanage() );

  message.adoptRoot( std::move( orphan ) );

  // ============================================================

  AddressBook::Reader addressBook =
      message.getRoot<AddressBook>();

  for( Person::Reader person : addressBook.getPeople() ) {
    std::cout << person.getName().cStr() << ": "
              << person.getEmail().cStr() << std::endl;
    for( Person::PhoneNumber::Reader phone :
         person.getPhones() ) {
      const char* typeName = "UNKNOWN";
      switch( phone.getType() ) {
        case Person::PhoneNumber::Type::MOBILE:
          typeName = "mobile";
          break;
        case Person::PhoneNumber::Type::HOME:
          typeName = "home";
          break;
        case Person::PhoneNumber::Type::WORK:
          typeName = "work";
          break;
      }
      std::cout << "  " << typeName
                << " phone: " << phone.getNumber().cStr()
                << std::endl;
    }
    Person::Employment::Reader employment =
        person.getEmployment();
    switch( employment.which() ) {
      case Person::Employment::UNEMPLOYED:
        std::cout << "  unemployed" << std::endl;
        break;
      case Person::Employment::EMPLOYER:
        std::cout << "  employer: "
                  << employment.getEmployer().cStr()
                  << std::endl;
        break;
      case Person::Employment::SCHOOL:
        std::cout << "  student at: "
                  << employment.getSchool().cStr() << std::endl;
        break;
      case Person::Employment::SELF_EMPLOYED:
        std::cout << "  self-employed" << std::endl;
        break;
    }
  }
}

void writeMonster( FILE* fp ) {
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

void test_serial() {
  // == Direct ==================================================
  // SaveableOmni omni;
  // UclArchiver  ar;
  // ar.save( "Parent", omni );
  // lg.info( "result:\n{}", ar.result );

  // == Proto ===================================================
  pb_test::AddressBook book;

  auto& person = *book.mutable_people();
  person.set_name( "David" );
  person.set_id( 5 );
  auto& phone_number = *person.mutable_phones();
  phone_number.set_number( "000-0000" );
  phone_number.set_type( pb_test::Person_PhoneType_MOBILE );

  string pb;
  google::protobuf::TextFormat::PrintToString( book, &pb );

  lg.info( "proto result:\n{}", pb );

  pb_test::AddressBook book2;
  google::protobuf::TextFormat::ParseFromString( pb, &book2 );

  CHECK( book.people().name() == book2.people().name() );
  CHECK( book.people().id() == book2.people().id() );
  CHECK( book.people().email() == book2.people().email() );
  CHECK( book.people().phones().number() ==
         book2.people().phones().number() );
  CHECK( book.people().phones().type() ==
         book2.people().phones().type() );

  // == CapnProto ===============================================
  FILE* fp;
  fp = ::fopen( "capn.out", "w" );
  CHECK( fp );
  // writeAddressBook( fileno( fp ) );
  writeAddressBook( fp );
  ::fclose( fp );
  fp = ::fopen( "capn.out", "r" );
  CHECK( fp );
  printAddressBook( fileno( fp ) );
  //::fclose( fp );

  // == Flatbuffers =============================================
  fp = ::fopen( "fb.out", "wb" );
  CHECK( fp );
  writeMonster( fp );
}

} // namespace rn
