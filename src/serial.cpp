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
#include "fmt-helper.hpp"
#include "logging.hpp"

using namespace std;

namespace rn {

namespace {

template<int N>
struct disambiguate;

struct Archiver {
  // All types that have a `serialize` member function.
  template<typename ObjectType, disambiguate<0>* = nullptr>
  auto save( string_view field_name, ObjectType const& o )
      -> void_t<decltype( o.serialize( *this ) )> {
    result +=
        fmt::format( "{}{}: object\n", spaces(), field_name );
    level++;
    o.serialize( *this );
    level--;
  }

  // All types that are formattable by {fmt} and/or one of our
  // {fmt} formatting customizations that is visible in this
  // translation unit.
  // template<typename Formattable,
  //         typename = void_t<
  //             decltype( ::fmt::formatter<Formattable>{} )>*>
  // void save( string_view        field_name,
  //           Formattable const& fmtable ) {
  //  result += fmt::format( "{}{}: {}\n", spaces(), field_name,
  //                         fmtable );
  //}

  void save( string_view field_name, int n ) {
    result +=
        fmt::format( "{}{}: {}\n", spaces(), field_name, n );
  }

  void save( string_view field_name, double d ) {
    result +=
        fmt::format( "{}{}: {}\n", spaces(), field_name, d );
  }

  void save( string_view field_name, string const& s ) {
    result +=
        fmt::format( "{}{}: {}\n", spaces(), field_name, s );
  }

  // Types for which there is a `serialize` free function found
  // by ADL.
  template<typename ADLserializable, disambiguate<1>* = nullptr>
  auto save( string_view            field_name,
             ADLserializable const& serializable )
      -> void_t<decltype( serialize( *this, serializable ) )> {
    result += fmt::format( "{}{}:\n", spaces(), field_name );
    level++;
    serialize( *this, serializable );
    level--;
  }

  // Vectors of serializable types.
  template<typename T>
  auto save( string_view field_name, Vec<T> const& v )
      -> void_t<decltype( save( field_name, v[0] ) )> {
    result += fmt::format( "{}{} begin (size {}):\n", spaces(),
                           field_name, v.size() );
    int i = 0;
    level++;
    for( auto const& e : v ) save( std::to_string( i++ ), e );
    level--;
    result += fmt::format( "{}{} end\n", spaces(), field_name );
  }

  // Should not serialize pointers.
  template<typename T>
  void save( string_view field_name, T* ) = delete;

  // Better-enums.
  template<typename ReflectedEnum>
  auto save( string_view field_name, ReflectedEnum const& e )
      -> void_t<typename ReflectedEnum::_enumerated> {
    result +=
        fmt::format( "{}{}: {}\n", spaces(), field_name, e );
  }

  string spaces() const { return string( level * 2, ' ' ); }

  int    level{0};
  string result;
};

} // namespace

/****************************************************************
** Testing
*****************************************************************/
void test_serial() {
  SaveableOmni omni;
  Archiver     ar;

  ar.save( "Parent", omni );
  lg.info( "result:\n{}", ar.result );
}

} // namespace rn
