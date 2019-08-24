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
#include "adt.hpp"
#include "aliases.hpp"
#include "coord.hpp"
#include "gfx.hpp"
#include "input.hpp"
#include "plane.hpp"
#include "text.hpp"
#include "tx.hpp"

// base-util
#include "base-util/variant.hpp"

// C++ standard library
#include <variant>

ADT_T( rn,                                       //
       TEMPLATE( DragSrcT, DragDstT, DragArcT ), //
       DragState,                                //
       ( none ),                                 //
       ( in_progress,                            //
         ( DragSrcT, src ),                      //
         ( Opt<DragDstT>, dst ),                 //
         ( Opt<Texture>, tx ) ),                 //
       ( complete,                               //
         ( DragArcT, arc ) ),                    //
       ( rubber_band,                            //
         ( Coord, current ),                     //
         ( Coord, dest ),                        //
         ( double, percent ),                    //
         ( Opt<Texture>, tx ) )                  //
);

namespace rn {

// To be used as a base class in the CRTP.
template<typename Child, typename DragSrcT, typename DragDstT,
         typename DragArcT>
class DragAndDrop {
protected:
public:
  DragAndDrop() : state_{NoneT{}} {}

  Child const& child() const {
    return *static_cast<Child const*>( this );
  }
  Child& child() { return *static_cast<Child*>( this ); }

  void handle_on_frame_start() {
    if_v( state_, CompleteT, p_arc ) {
      child().perform_drag( p_arc->arc );
      state_ = NoneT{};
      return;
    }
    if_v( state_, RubberBandT, p_band ) {
      p_band->percent += 0.15;
      if( p_band->percent > 1.0 ) state_ = NoneT{};
    }
  }

  void handle_draw( Texture& tx ) const {
    switch_( state_ ) {
      case_( InProgressT ) {
        if( val.tx ) {
          auto mouse_pos = input::current_mouse_position();
          copy_texture( *val.tx, tx,
                        mouse_pos - val.tx->size() / Scale{2} );
          // Now draw the indicator.
          auto indicator = drag_status_indicator( val );
          switch( indicator ) {
            case e_drag_status_indicator::none: break;
            case e_drag_status_indicator::never: {
              auto const& status_tx =
                  render_text( "X", Color::yellow() );
              auto indicator_pos =
                  mouse_pos - status_tx.size() / Scale{1};
              copy_texture( status_tx, tx, indicator_pos );
              break;
            }
            case e_drag_status_indicator::bad: {
              auto const& status_tx =
                  render_text( "X", Color::red() );
              auto indicator_pos =
                  mouse_pos - status_tx.size() / Scale{1};
              copy_texture( status_tx, tx, indicator_pos );
              break;
            }
            case e_drag_status_indicator::good: {
              auto const& status_tx =
                  render_text( "+", Color::green() );
              auto indicator_pos =
                  mouse_pos - status_tx.size() / Scale{1};
              copy_texture( status_tx, tx, indicator_pos );
              break;
            }
          }
        }
      }
      case_( RubberBandT ) {
        CHECK( val.tx.has_value() );
        auto  mouse_pos = input::current_mouse_position();
        auto  delta     = val.dest - val.current;
        Coord pos;
        pos.x._ = mouse_pos.x._ + int( delta.w._ * val.percent );
        pos.y._ = mouse_pos.y._ + int( delta.h._ * val.percent );
        copy_texture( *val.tx, tx,
                      pos - val.tx->size() / Scale{2} );
      }
      switch_non_exhaustive;
    }
  }

  Plane::DragInfo handle_can_drag( Coord origin ) {
    auto maybe_drag_src = child().drag_src( origin );
    if( !maybe_drag_src ) return Plane::e_accept_drag::no;

    state_ = InProgressT{
        /*src=*/*maybe_drag_src,
        /*dst=*/std::nullopt,
        /*tx=*/child().draw_dragged_item( *maybe_drag_src ),
    };
    return Plane::e_accept_drag::yes;
  }

  void handle_on_drag( Coord current ) {
    if_v( state_, InProgressT, in_progress ) {
      in_progress->dst = child().drag_dst( current );
    }
  }

  bool handle_on_drag_finished( Coord const& drag_start,
                                Coord const& drag_end ) {
    if_v( state_, InProgressT, in_progress ) {
      if( in_progress->dst ) {
        auto maybe_drag_arc = child().drag_arc(
            in_progress->src, *in_progress->dst );
        if( maybe_drag_arc &&
            child().can_perform_drag( *maybe_drag_arc ) ) {
          state_ = CompleteT{
              /*arc=*/*maybe_drag_arc //
          };
          return true;
        }
      }

      if( in_progress->tx.has_value() ) {
        state_ = RubberBandT{
            /*current=*/drag_end,
            /*dest=*/drag_start,
            /*percent=*/0.0,
            /*<Texture> tx=*/std::move( in_progress->tx ),
        };
      } else {
        state_ = NoneT{};
      }
      return true;
    }
    return false;
  }

private:
  using NoneT = DragState::none<DragSrcT, DragDstT, DragArcT>;
  using InProgressT =
      DragState::in_progress<DragSrcT, DragDstT, DragArcT>;
  using CompleteT =
      DragState::complete<DragSrcT, DragDstT, DragArcT>;
  using RubberBandT =
      DragState::rubber_band<DragSrcT, DragDstT, DragArcT>;

private:
  enum class e_drag_status_indicator { none, never, bad, good };

  e_drag_status_indicator drag_status_indicator(
      InProgressT const& in_progress ) const {
    if( !in_progress.dst ) return e_drag_status_indicator::none;
    auto maybe_arc =
        child().drag_arc( in_progress.src, *in_progress.dst );
    if( !maybe_arc ) return e_drag_status_indicator::never;
    if( !child().can_perform_drag( *maybe_arc ) )
      return e_drag_status_indicator::bad;
    return e_drag_status_indicator::good;
  }

private:
  DragState_t<DragSrcT, DragDstT, DragArcT> state_;
};

} // namespace rn
