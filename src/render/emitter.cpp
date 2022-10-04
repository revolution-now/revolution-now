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

namespace {

#ifdef TRACK_CAPACITY
void log_capacity_change( bool on, long capacity_before,
                          long capacity_after ) {
  DCHECK( capacity_after >= capacity_before );
  if( on && capacity_after != capacity_before )
    fmt::print(
        "{}:{}:log: vertex buffer emitter capacity change: {} "
        "-> {} (factor of {} increase).\n",
        __FILE__, __LINE__, capacity_before, capacity_after,
        double( capacity_after ) / double( capacity_before ) );
}
#else
void log_capacity_change( bool /*on*/, long /*capacity_before*/,
                          long /*capacity_after*/ ) {}
#endif

} // namespace

/****************************************************************
** Emitter
*****************************************************************/
void Emitter::emit( GenericVertex const& vert ) {
  long capacity_before = buffer_->capacity();
  DCHECK( pos_ <= long( buffer_->size() ) );
  if( pos_ < long( buffer_->size() ) )
    ( *buffer_ )[pos_] = vert;
  else
    buffer_->push_back( vert );
  ++pos_;
  long capacity_after = buffer_->capacity();
  log_capacity_change( log_capacity_changes_, capacity_before,
                       capacity_after );
}

void Emitter::set_position( long new_pos ) {
  pos_ = new_pos;
  DCHECK( pos_ <= long( buffer_->size() ) );
}

void Emitter::emit( span<GenericVertex const> vertices ) {
  if( vertices.empty() ) return;
  long       capacity_before = buffer_->capacity();
  long       current_size    = buffer_->size();
  long const needed_size     = pos_ + vertices.size();
  if( current_size <= needed_size )
    buffer_->resize( needed_size );
  long capacity_after = buffer_->capacity();
  log_capacity_change( log_capacity_changes_, capacity_before,
                       capacity_after );
  DCHECK( long( buffer_->size() ) >= needed_size );
  copy( vertices.begin(), vertices.end(),
        buffer_->begin() + pos_ );
  pos_ += vertices.size();
}

} // namespace rr
