/****************************************************************
**binary-data.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-03.
*
* Description: For ease of working with binary buffers.
*
*****************************************************************/
#include "binary-data.hpp"

using namespace std;

namespace base {

/****************************************************************
** BinaryData
*****************************************************************/
bool BinaryData::eof() const { return !good( 1 ); }

bool BinaryData::good( int nbytes ) const {
  int const remaining = buffer_.size() - ( idx_ + nbytes );
  return ( remaining >= 0 );
}

bool BinaryData::read_bytes( int n, unsigned char* dst ) {
  if( !good( n ) ) return false;
  std::memcpy( dst, &buffer_[idx_], n );
  idx_ += n;
  return true;
}

bool BinaryData::write_bytes( int n, unsigned char const* src ) {
  if( !good( n ) ) return false;
  std::memcpy( &buffer_[idx_], src, n );
  idx_ += n;
  return true;
}

} // namespace base
