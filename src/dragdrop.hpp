/****************************************************************
**dragdrop.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-20.
*
* Description: A framework for drag & drop of entities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "coord.hpp"
#include "input.hpp"
#include "plane.hpp"
#include "tx.hpp"

// base-util
#include "base-util/variant.hpp"

// C++ standard library
#include <variant>

namespace rn {

// To be used as a base class in the CRTP.
template<typename Child, typename DragSrcT, typename DragDstT,
         typename DragArcT>
class DragAndDrop {
protected:
  // FIXME: make this an ADT outside class, but need templates.
  struct DragState {
    struct none {};
    struct in_progress {
      DragSrcT      src;
      Opt<DragDstT> dst;
      Opt<Texture>  tx;
    };
    struct complete {
      DragArcT arc;
    };
    struct rubber_band {
      Coord        current;
      Coord        dest;
      Opt<Texture> tx;
    };
  };
  // The `none` state should be first.
  using DragState_t = std::variant<    //
      typename DragState::none,        //
      typename DragState::in_progress, //
      typename DragState::complete,    //
      typename DragState::rubber_band  //
      >;

public:
  DragAndDrop() : state_{typename DragState::none{}} {}

  Child const& child() const {
    return *static_cast<Child const*>( this );
  }
  Child& child() { return *static_cast<Child*>( this ); }

  void handle_draw( Texture& tx ) const {
    if_v( state_, typename DragState::in_progress,
          in_progress ) {
      child().draw_drag_cursor( tx, *in_progress );
    }
  }

  Plane::DragInfo handle_can_drag( Coord origin ) {
    CHECK( util::holds<typename DragState::none>( state_ ) );
    auto maybe_drag_in_progress = try_drag_start( origin );
    if( maybe_drag_in_progress ) {
      state_ = std::move( *maybe_drag_in_progress );
      // lg.info( "drag state: {}", state_ );
      return Plane::e_accept_drag::yes;
    }
    return Plane::e_accept_drag::no;
  }

  void handle_on_drag( Coord current ) {
    if_v( state_, typename DragState::in_progress,
          in_progress ) {
      in_progress->dst = child().drag_dst( current );
    }
  }

  bool handle_on_drag_finished() {
    if_v( state_, typename DragState::in_progress,
          in_progress ) {
      if( in_progress->dst ) {
        auto maybe_drag_arc = child().drag_arc(
            in_progress->src, *in_progress->dst );
        if( maybe_drag_arc ) {
          state_ = typename DragState::complete{
              /*arc=*/*maybe_drag_arc};
          // lg.info( "drag state: {}", state_ );
          return true;
        }
      }
      state_ = typename DragState::none{};
      return true;
    }
    return false;
  }

  void handle_on_frame_start() {
    if_v( state_, typename DragState::complete, p_arc ) {
      child().perform_drag( p_arc->arc );
      state_ = typename DragState::none{};
      // lg.info( "drag state: {}", state_ );
    }
  }

private:
  void draw_drag_cursor(
      Texture&                               target,
      typename DragState::in_progress const& info ) const {
    if( !info.tx ) return;
    auto mouse_pos = input::current_mouse_position();
    copy_texture( *info.tx, target,
                  mouse_pos - info.tx->size() / Scale{2} );
  }

  Opt<typename DragState::in_progress> try_drag_start(
      Coord const& origin ) const {
    Opt<typename DragState::in_progress> res;
    auto maybe_being_dragged = child().drag_src( origin );
    // lg.info( "drag src: {}", maybe_being_dragged );
    if( maybe_being_dragged ) {
      auto const& drag_src = *maybe_being_dragged;

      res = typename DragState::in_progress{
          /*src=*/drag_src,
          /*dst=*/std::nullopt,
          /*tx=*/child().draw_dragged_item( drag_src ),
      };
    }
    return res;
  }

private:
  DragState_t state_;
};

} // namespace rn
