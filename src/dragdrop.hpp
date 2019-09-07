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
#include "fsm.hpp"
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

namespace rn {

adt_template_rn( template( DragSrcT, DragDstT, DragArcT ), //
                 DragState,                                //
                 ( none ),                                 //
                 ( in_progress,                            //
                   ( DragSrcT, src ),                      //
                   ( Opt<DragDstT>, dst ),                 //
                   ( Texture, tx ) ),                      //
                 ( waiting_to_execute,                     //
                   ( DragArcT, arc ) ),                    //
                 ( rubber_banding,                         //
                   ( Coord, current ),                     //
                   ( Coord, dest ),                        //
                   ( DragSrcT, src ),                      //
                   ( double, percent ),                    //
                   ( Texture, tx ) )                       //
);

adt_template_rn( template( DragSrcT, DragDstT, DragArcT ), //
                 DragEvent,                                //
                 ( start,                                  //
                   ( DragSrcT, src ),                      //
                   ( Opt<DragDstT>, dst ),                 //
                   ( Texture, tx ) ),                      //
                 ( rubber_band,                            //
                   ( Coord, current ),                     //
                   ( Coord, dest ),                        //
                   ( DragSrcT, src ),                      //
                   ( double, percent ),                    //
                   ( Texture, tx ) ),                      //
                 ( complete,                               //
                   ( DragArcT, arc ) ),                    //
                 ( reset )                                 //
);

// clang-format off
fsm_transitions_T( template( DragSrcT, DragDstT, DragArcT ), Drag,
  ((none,               start      ), ->  ,in_progress       ),
  ((in_progress,        rubber_band), ->  ,rubber_banding    ),
  ((in_progress,        complete   ), ->  ,waiting_to_execute),
  ((rubber_banding,     reset      ), ->  ,none              ),
  ((waiting_to_execute, reset      ), ->  ,none              )
);
// clang-format on

fsm_class_template( template( DragSrcT, DragDstT, DragArcT ),
                    Drag ) {
  fsm_init_template(
      template( DragSrcT, DragDstT, DragArcT ), Drag,
      ( DragState::none<DragSrcT, DragDstT, DragArcT>{} ) );

  fsm_transition_template(
      template( DragSrcT, DragDstT, DragArcT ), Drag, //
      none, start, ->, in_progress ) {
    return {
        /*src=*/std::move( event.src ), //
        /*dst=*/std::move( event.dst ), //
        /*tx=*/std::move( event.tx ),   //
    };
  }

  fsm_transition_template(
      template( DragSrcT, DragDstT, DragArcT ), Drag, //
      in_progress, complete, ->, waiting_to_execute ) {
    return {
        /*arc=*/std::move( event.arc ) //
    };
  }

  fsm_transition_template(
      template( DragSrcT, DragDstT, DragArcT ), Drag, //
      in_progress, rubber_band, ->, rubber_banding ) {
    return {
        /*current=*/std::move( event.current ), //
        /*dest=*/std::move( event.dest ),       //
        /*src=*/std::move( event.src ),         //
        /*percent=*/std::move( event.percent ), //
        /*tx=*/std::move( event.tx ),           //
    };
  }
};

// To be used as a base class in the CRTP.
template<typename Child, typename DraggableObjectT,
         typename DragSrcT, typename DragDstT, typename DragArcT>
class DragAndDrop {
protected:
  DragAndDrop() {}

  Child const& child() const {
    return *static_cast<Child const*>( this );
  }
  Child& child() { return *static_cast<Child*>( this ); }

public:
  bool is_drag_in_progress() const {
    return fsm_.template holds<InProgress_t>().has_value();
  }

  void handle_on_frame_start() {
    if( auto arc = fsm_.template holds<WaitingToExecute_t>();
        arc ) {
      child().perform_drag( arc->get().arc );
      fsm_.send_event( Reset_t{} );
      fsm_.process_events();
      return;
    }
    if( auto rband = fsm_.template holds<RubberBanding_t>();
        rband ) {
      rband->get().percent += 0.15;
      if( rband->get().percent > 1.0 ) {
        fsm_.send_event( Reset_t{} );
        fsm_.process_events();
      }
    }
  }

  void handle_draw( Texture& tx ) const {
    switch_( fsm_.state().get() ) {
      case_( InProgress_t ) {
        auto mouse_pos = input::current_mouse_position();
        copy_texture( val.tx, tx,
                      mouse_pos - val.tx.size() / Scale{2} );
        // Now draw the indicator.
        auto indicator = drag_status_indicator( val );
        switch( indicator ) {
          case e_drag_status_indicator::none: break;
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
      case_( RubberBanding_t ) {
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
    if( !fsm_.template holds<None_t>() )
      // e.g. we're rubber-banding, or waiting to execute.
      return Plane::e_accept_drag::swallow;
    auto maybe_drag_src = child().drag_src( origin );
    if( !maybe_drag_src ) return Plane::e_accept_drag::no;
    auto draggable =
        child().draggable_from_src( *maybe_drag_src );
    fsm_.send_event( Start_t{
        /*src=*/*maybe_drag_src,
        /*dst=*/std::nullopt,
        /*tx=*/child().draw_dragged_item( draggable ),
    } );
    fsm_.process_events();
    return Plane::e_accept_drag::yes;
  }

  void handle_on_drag( Coord current ) {
    if( auto in_progress = fsm_.template holds<InProgress_t>();
        in_progress ) {
      in_progress->get().dst = child().drag_dst( current );
    }
  }

  bool handle_on_drag_finished( Coord const& drag_start,
                                Coord const& drag_end ) {
    if( auto in_progress = fsm_.template holds<InProgress_t>();
        in_progress ) {
      if( in_progress->get().dst ) {
        auto maybe_drag_arc = drag_arc(
            in_progress->get().src, *in_progress->get().dst );
        if( maybe_drag_arc &&
            child().can_perform_drag( *maybe_drag_arc ) ) {
          fsm_.send_event( Complete_t{
              /*arc=*/*maybe_drag_arc //
          } );
          fsm_.process_events();
          return true;
        }
      }

      fsm_.send_event( RubberBand_t{
          /*current=*/drag_end,
          /*dest=*/drag_start,
          /*src=*/in_progress->get().src,
          /*percent=*/0.0,
          /*tx=*/std::move( in_progress->get().tx ),
      } );
      fsm_.process_events();
      return true;
    }
    return false;
  }

  Opt<DraggableObjectT> obj_being_dragged() const {
    Opt<DraggableObjectT> res;
    switch_( fsm_.state().get() ) {
      case_( None_t ) break_;
      case_( RubberBanding_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( InProgress_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( WaitingToExecute_t ) {}
      switch_exhaustive;
    }
    return res;
  }

private:
  // These are to save typing above.
  using None_t  = DragState::none<DragSrcT, DragDstT, DragArcT>;
  using Reset_t = DragEvent::reset<DragSrcT, DragDstT, DragArcT>;
  using InProgress_t =
      DragState::in_progress<DragSrcT, DragDstT, DragArcT>;
  using Start_t = DragEvent::start<DragSrcT, DragDstT, DragArcT>;
  using WaitingToExecute_t =
      DragState::waiting_to_execute<DragSrcT, DragDstT,
                                    DragArcT>;
  using Complete_t =
      DragEvent::complete<DragSrcT, DragDstT, DragArcT>;
  using RubberBanding_t =
      DragState::rubber_banding<DragSrcT, DragDstT, DragArcT>;
  using RubberBand_t =
      DragEvent::rubber_band<DragSrcT, DragDstT, DragArcT>;
  using State_t = DragState_t<DragSrcT, DragDstT, DragArcT>;
  using Event_t = DragEvent_t<DragSrcT, DragDstT, DragArcT>;
  using fsm_t   = DragFsm<DragSrcT, DragDstT, DragArcT>;

  ASSERT_NOTHROW_MOVING(
      DragState_t<DragSrcT, DragDstT, DragArcT> );
  ASSERT_NOTHROW_MOVING(
      DragEvent_t<DragSrcT, DragDstT, DragArcT> );
  ASSERT_NOTHROW_MOVING( fsm_t );

private:
  template<size_t ArcTypeIndex>
  static void try_set_arc_impl( Opt<DragArcT>*  arc,
                                DragSrcT const& src,
                                DragDstT const& dst ) {
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
  static void try_set_arc( Opt<DragArcT>*  arc,
                           DragSrcT const& src,
                           DragDstT const& dst,
                           std::index_sequence<Indexes...> ) {
    ( try_set_arc_impl<Indexes>( arc, src, dst ), ... );
  }

  static Opt<DragArcT> drag_arc( DragSrcT const& src,
                                 DragDstT const& dst ) {
    Opt<DragArcT> res;
    try_set_arc( &res, src, dst,
                 std::make_index_sequence<
                     std::variant_size_v<DragArcT>>() );
    return res;
  }

  enum class e_drag_status_indicator { none, bad, good };

  e_drag_status_indicator drag_status_indicator(
      InProgress_t const& in_progress ) const {
    if( !in_progress.dst ) return e_drag_status_indicator::none;
    auto maybe_arc =
        drag_arc( in_progress.src, *in_progress.dst );
    if( !maybe_arc ) return e_drag_status_indicator::none;
    if( !child().can_perform_drag( *maybe_arc ) )
      return e_drag_status_indicator::bad;
    return e_drag_status_indicator::good;
  }

private:
  fsm_t fsm_{};
};

} // namespace rn
