/****************************************************************
**proto.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-30.
*
* Description: Unit tests for proto integration.
*
*****************************************************************/
#include "testing.hpp"

// Proto
#include <google/protobuf/text_format.h>
#include "testing.pb.h"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[proto] round trip proto to text." ) {
  pb_test::AddressBook book;

  auto& person = *book.mutable_people();
  person.set_name( "David" );
  person.set_id( 5 );
  auto& phone_number = *person.mutable_phones();
  phone_number.set_number( "000-0000" );
  phone_number.set_type( pb_test::Person_PhoneType_HOME );

  string pb;
  google::protobuf::TextFormat::PrintToString( book, &pb );

  pb_test::AddressBook book2;
  google::protobuf::TextFormat::ParseFromString( pb, &book2 );

  REQUIRE( book2.IsInitialized() );

  REQUIRE( book2.people().name() == "David" );
  REQUIRE( book2.people().id() == 5 );
  // REQUIRE( !book2.people().has_email() ); // ?!
  REQUIRE( book2.people().email() == "" );
  REQUIRE( book2.people().phones().number() == "000-0000" );
  REQUIRE( book2.people().phones().type() ==
           pb_test::Person_PhoneType_HOME );
}

} // namespace
