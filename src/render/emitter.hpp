/****************************************************************
**emitter.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Helper for emitting vertices into a vector.
*
*****************************************************************/
#pragma once

// render
#include "vertex.hpp"

// C++ standard library
#include <span>
#include <vector>

namespace rr {

/****************************************************************
** Emitter
*****************************************************************/
// This is a kind of "cursor" that will abstract away the process
// of emitting vertices into a vertex buffer. It has a current
// position, and will either overwrite or append (depending on
// the position relative to the size of the underlying buffer) in
// a way that is invisible to the client.
struct Emitter {
  Emitter( std::vector<GenericVertex>& buffer )
    : Emitter( buffer, 0 ) {}

  Emitter( std::vector<GenericVertex>& buffer, int pos )
    : buffer_( &buffer ),
      pos_( pos ),
      log_capacity_changes_( false ) {}

  template<VertexType V>
  void emit( V const& vert ) {
    emit( vert.generic() );
  }

  // This one is preferred if emitting a number of vertices at
  // once since it will guarantee at most one resizing of the un-
  // derlying vector.
  template<VertexType V>
  void emit( std::span<V const> vert ) {
    if( vert.empty() ) return;
    emit( std::span<GenericVertex const>(
        static_cast<GenericVertex const*>(
            &( vert.data()->generic() ) ),
        vert.size() ) );
  }

  void log_capacity_changes( bool enable ) {
    log_capacity_changes_ = enable;
  }

 private:
  void emit( GenericVertex const& vert );

  void emit( std::span<GenericVertex const> vertices );

  std::vector<GenericVertex>* buffer_;
  int                         pos_;
  bool                        log_capacity_changes_;
};

} // namespace rr
