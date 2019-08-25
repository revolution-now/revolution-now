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
#include "macros.hpp"
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
         ( Texture, tx ) ),                      //
       ( complete,                               //
         ( DragArcT, arc ) ),                    //
       ( rubber_band,                            //
         ( Coord, current ),                     //
         ( Coord, dest ),                        //
         ( DragSrcT, src ),                      //
         ( double, percent ),                    //
         ( Texture, tx ) )                       //
);

namespace rn {

// To be used as a base class in the CRTP.
template<typename Child, typename DraggableObjectT,
         typename DragSrcT, typename DragDstT, typename DragArcT>
class DragAndDrop {
public:
  DragAndDrop() : state_{None_t{}} {}

  Child const& child() const {
    return *static_cast<Child const*>( this );
  }
  Child& child() { return *static_cast<Child*>( this ); }

  void handle_on_frame_start() {
    if_v( state_, Complete_t, p_arc ) {
      child().perform_drag( p_arc->arc );
      state_ = None_t{};
      return;
    }
    if_v( state_, RubberBand_t, p_band ) {
      p_band->percent += 0.15;
      if( p_band->percent > 1.0 ) state_ = None_t{};
    }
  }

  void handle_draw( Texture& tx ) const {
    switch_( state_ ) {
      case_( InProgress_t ) {
        auto mouse_pos = input::current_mouse_position();
        copy_texture( val.tx, tx,
                      mouse_pos - val.tx.size() / Scale{2} );
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
      case_( RubberBand_t ) {
        auto  delta = val.dest - val.current;
        Coord pos;
        pos.x._ =
            val.current.x._ + int( delta.w._ * val.percent );
        pos.y._ =
            val.current.y._ + int( delta.h._ * val.percent );
        copy_texture( val.tx, tx,
                      pos - val.tx.size() / Scale{2} );
      }
      switch_non_exhaustive;
    }
  }

  Plane::DragInfo handle_can_drag( Coord origin ) {
    if( !util::holds<None_t>( state_ ) )
      // e.g. we're rubber-banding.
      return Plane::e_accept_drag::swallow;
    auto maybe_drag_src = child().drag_src( origin );
    if( !maybe_drag_src ) return Plane::e_accept_drag::no;
    auto draggable =
        child().draggable_from_src( *maybe_drag_src );
    state_ = InProgress_t{
        /*src=*/*maybe_drag_src,
        /*dst=*/std::nullopt,
        /*tx=*/child().draw_dragged_item( draggable ),
    };
    return Plane::e_accept_drag::yes;
  }

  void handle_on_drag( Coord current ) {
    if_v( state_, InProgress_t, in_progress ) {
      in_progress->dst = child().drag_dst( current );
    }
  }

  bool handle_on_drag_finished( Coord const& drag_start,
                                Coord const& drag_end ) {
    if_v( state_, InProgress_t, in_progress ) {
      if( in_progress->dst ) {
        auto maybe_drag_arc =
            drag_arc( in_progress->src, *in_progress->dst );
        if( maybe_drag_arc &&
            child().can_perform_drag( *maybe_drag_arc ) ) {
          state_ = Complete_t{
              /*arc=*/*maybe_drag_arc //
          };
          return true;
        }
      }

      state_ = RubberBand_t{
          /*current=*/drag_end,
          /*dest=*/drag_start,
          /*src=*/in_progress->src,
          /*percent=*/0.0,
          /*<Texture> tx=*/std::move( in_progress->tx ),
      };
      return true;
    }
    return false;
  }

  Opt<DraggableObjectT> obj_being_dragged() const {
    Opt<DraggableObjectT> res;
    switch_( state_ ) {
      case_( None_t ) break_;
      case_( RubberBand_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( InProgress_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( Complete_t ) {}
      switch_exhaustive;
    }
    return res;
  }

private:
  using None_t = DragState::none<DragSrcT, DragDstT, DragArcT>;
  using InProgress_t =
      DragState::in_progress<DragSrcT, DragDstT, DragArcT>;
  using Complete_t =
      DragState::complete<DragSrcT, DragDstT, DragArcT>;
  using RubberBand_t =
      DragState::rubber_band<DragSrcT, DragDstT, DragArcT>;
  using State_t = DragState_t<DragSrcT, DragDstT, DragArcT>;

private:
  template<size_t ArcTypeIndex>
  void try_set_arc_impl( Opt<DragArcT>* arc, DragSrcT const& src,
                         DragDstT const& dst ) const {
    CHECK( arc );
    using ArcSubType =
        std::variant_alternative_t<ArcTypeIndex, DragArcT>;
    using ArcSrcType =
        std::decay_t<decltype( std::declval<ArcSubType>().src )>;
    using ArcDstType =
        std::decay_t<decltype( std::declval<ArcSubType>().dst )>;
    auto const* p_src = std::get_if<ArcSrcType>( &src );
    auto const* p_dst = std::get_if<ArcDstType>( &dst );
    if( p_src && p_dst ) {
      CHECK( !arc->has_value(),
             "There are two DragArc subtypes that have the same "
             "src & dst types but this is not allowed." );
      *arc = ArcSubType{*p_src, *p_dst};
    }
  }

  // Here we will iterate through all the types that DragArcT can
  // have (DragArcT is assumed to be a variant) and we will find
  // one whose `src` and `dst` field types match the types of the
  // arguments. If found we will copy src/dst into the arc. We
  // expect that at most one DragArcT subtype will be found com-
  // patible; the above helper function will check-fail if more
  // than one is found.
  template<size_t... Indexes>
  void try_set_arc( Opt<DragArcT>* arc, DragSrcT const& src,
                    DragDstT const& dst,
                    std::index_sequence<Indexes...> ) const {
    ( try_set_arc_impl<Indexes>( arc, src, dst ), ... );
  }

  Opt<DragArcT> drag_arc( DragSrcT const& src,
                          DragDstT const& dst ) const {
    Opt<DragArcT> res;
    try_set_arc( &res, src, dst,
                 std::make_index_sequence<
                     std::variant_size_v<DragArcT>>() );
    return res;
  }

  enum class e_drag_status_indicator { none, never, bad, good };

  e_drag_status_indicator drag_status_indicator(
      InProgress_t const& in_progress ) const {
    if( !in_progress.dst ) return e_drag_status_indicator::none;
    auto maybe_arc =
        drag_arc( in_progress.src, *in_progress.dst );
    if( !maybe_arc ) return e_drag_status_indicator::never;
    if( !child().can_perform_drag( *maybe_arc ) )
      return e_drag_status_indicator::bad;
    return e_drag_status_indicator::good;
  }

private:
  State_t state_;
  ASSERT_NOTHROW_MOVING( State_t );
};

} // namespace rn
