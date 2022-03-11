/****************************************************************
**emitter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Helper for emitting vertices into a vector.
*
*****************************************************************/
#include "emitter.hpp"

using namespace std;

#ifndef NDEBUG
#  define TRACK_CAPACITY
#endif

namespace rr {

/****************************************************************
** Emitter
*****************************************************************/
void Emitter::emit( GenericVertex const& vert ) {
  if( pos_ < int( buffer_->size() ) )
    ( *buffer_ )[pos_] = vert;
  else
    buffer_->push_back( vert );
  ++pos_;
}

void Emitter::emit( span<GenericVertex const> vertices ) {
  if( vertices.empty() ) return;
#ifdef TRACK_CAPACITY
  int capacity_before = buffer_->capacity();
#endif
  int       current_size = buffer_->size();
  int const needed_size  = pos_ + vertices.size();
  if( current_size <= needed_size )
    buffer_->resize( needed_size );
#ifdef TRACK_CAPACITY
  int capacity_after = buffer_->capacity();
  DCHECK( capacity_after >= capacity_before );
  if( log_capacity_changes_ &&
      capacity_after != capacity_before )
    fmt::print(
        "{}:{}:log: vertex buffer emitter capacity change: {} "
        "-> {} (factor of {} increase).\n",
        __FILE__, __LINE__, capacity_before, capacity_after,
        double( capacity_after ) / double( capacity_before ) );
#endif
  DCHECK( int( buffer_->size() ) >= needed_size );
  copy( vertices.begin(), vertices.end(),
        buffer_->begin() + pos_ );
  pos_ += vertices.size();
}

} // namespace rr
