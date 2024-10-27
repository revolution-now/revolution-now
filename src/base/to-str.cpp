/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-19.
*
* Description: to_str for builtin types.
*
*****************************************************************/
#include "to-str.hpp"

using namespace std;

namespace base {

void to_str( bool const o, std::string& out, tag<bool> ) {
  if( o )
    out += "true";
  else
    out += "false";
}

void to_str( char const o, string& out, tag<char> ) { out += o; }

void to_str( int8_t const o, string& out, tag<int8_t> ) {
  out += to_string( o );
}

void to_str( uint8_t const o, string& out, tag<uint8_t> ) {
  out += to_string( o );
}

void to_str( int16_t const o, string& out, tag<int16_t> ) {
  out += to_string( o );
}

void to_str( uint16_t const o, string& out, tag<uint16_t> ) {
  out += to_string( o );
}

void to_str( int32_t const o, string& out, tag<int32_t> ) {
  out += to_string( o );
}

void to_str( uint32_t const o, string& out, tag<uint32_t> ) {
  out += to_string( o );
}

void to_str( int64_t const o, string& out, tag<int64_t> ) {
  out += to_string( o );
}

void to_str( uint64_t const o, string& out, tag<uint64_t> ) {
  out += to_string( o );
}

void to_str( float const o, std::string& out, tag<float> ) {
  // Using the fmt one suppresses extra decimal places.
  out += fmt::to_string( o );
}

void to_str( double const o, std::string& out, tag<double> ) {
  // Using the fmt one suppresses extra decimal places.
  out += fmt::to_string( o );
}

void to_str( long double const o, std::string& out,
             tag<long double> ) {
  // Using the fmt one suppresses extra decimal places.
  out += fmt::to_string( o );
}

} // namespace base
