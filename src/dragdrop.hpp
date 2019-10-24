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
#include "variant.hpp"

// base-util
#include "base-util/variant.hpp"

// C++ standard library
#include <variant>

namespace rn {

adt_T_rn( template( DragSrcT, DragDstT, DragArcT ), //
          DragState,                                //
          ( none ),                                 //
          ( in_progress,                            //
            ( DragSrcT, src ),                      //
            ( Opt<DragDstT>, dst ),                 //
            ( Texture, tx ),                        //
            ( input::mod_keys, mod_keys ),          //
            ( Delta, click_offset ) ),              //
          ( waiting_to_execute,                     //
            ( DragArcT, arc ),                      //
            ( Coord, mouse_released ),              //
            ( Texture, tx ),                        //
            ( Delta, click_offset ) ),              //
          ( finalizing,                             //
            ( DragArcT, arc ),                      //
            ( Coord, drag_start ),                  //
            ( Coord, mouse_released ),              //
            ( Texture, tx ),                        //
            ( input::mod_keys, mod_keys ),          //
            ( Delta, click_offset ) ),              //
          ( rubber_banding,                         //
            ( Coord, current ),                     //
            ( Coord, dest ),                        //
            ( DragSrcT, src ),                      //
            ( double, percent ),                    //
            ( Texture, tx ) )                       //
);

adt_T_rn( template( DragSrcT, DragDstT, DragArcT ), //
          DragEvent,                                //
          ( start,                                  //
            ( DragSrcT, src ),                      //
            ( Opt<DragDstT>, dst ),                 //
            ( Texture, tx ),                        //
            ( Delta, click_offset ) ),              //
          ( rubber_band,                            //
            ( Coord, current ),                     //
            ( Coord, dest ),                        //
            ( DragSrcT, src ),                      //
            ( double, percent ),                    //
            ( Texture, tx ) ),                      //
          ( finalize,                               //
            ( DragArcT, arc ),                      //
            ( Coord, drag_start ),                  //
            ( Coord, mouse_released ),              //
            ( Texture, tx ),                        //
            ( input::mod_keys, mod_keys ),          //
            ( Delta, click_offset ) ),              //
          ( complete,                               //
            ( DragArcT, arc ),                      //
            ( Coord, mouse_released ),              //
            ( Texture, tx ),                        //
            ( Delta, click_offset ) ),              //
          ( reset )                                 //
);

// clang-format off
fsm_transitions_T( template( DragSrcT, DragDstT, DragArcT ), Drag,
  ((none,               start      ), ->  ,in_progress       ),
  ((in_progress,        rubber_band), ->  ,rubber_banding    ),
  ((in_progress,        finalize   ), ->  ,finalizing        ),
  ((finalizing,         rubber_band), ->  ,rubber_banding    ),
  ((finalizing,         complete   ), ->  ,waiting_to_execute),
  ((rubber_banding,     reset      ), ->  ,none              ),
  ((waiting_to_execute, reset      ), ->  ,none              )
);
// clang-format on

fsm_class_T( template( DragSrcT, DragDstT, DragArcT ), Drag ) {
  fsm_init_T(
      template( DragSrcT, DragDstT, DragArcT ), Drag,
      ( DragState::none<DragSrcT, DragDstT, DragArcT>{} ) );

  fsm_transition_T( template( DragSrcT, DragDstT, DragArcT ),
                    Drag, //
                    none, start, ->, in_progress ) {
    return {
        /*src=*/std::move( event.src ),      //
        /*dst=*/std::move( event.dst ),      //
        /*tx=*/std::move( event.tx ),        //
        /*mod_keys=*/{},                     //
        /*click_offset=*/event.click_offset, //
    };
  }

  fsm_transition_T( template( DragSrcT, DragDstT, DragArcT ),
                    Drag, //
                    in_progress, finalize, ->, finalizing ) {
    return {
        /*arc=*/std::move( event.arc ),          //
        /*drag_start=*/event.drag_start,         //
        /*mouse_released=*/event.mouse_released, //
        /*tx=*/std::move( event.tx ),            //
        /*mod_keys=*/event.mod_keys,             //
        /*click_offset=*/event.click_offset,     //
    };
  }

  fsm_transition_T(
      template( DragSrcT, DragDstT, DragArcT ), Drag, //
      finalizing, complete, ->, waiting_to_execute ) {
    return {
        /*arc=*/std::move( event.arc ),          //
        /*mouse_released=*/event.mouse_released, //
        /*tx=*/std::move( event.tx ),            //
        /*click_offset=*/event.click_offset,     //
    };
  }

  fsm_transition_T(
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

  fsm_transition_T(
      template( DragSrcT, DragDstT, DragArcT ), Drag, //
      finalizing, rubber_band, ->, rubber_banding ) {
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
  using State_t = DragState_t<DragSrcT, DragDstT, DragArcT>;
  using Event_t = DragEvent_t<DragSrcT, DragDstT, DragArcT>;
  using fsm_t   = DragFsm<DragSrcT, DragDstT, DragArcT>;

protected:
  DragAndDrop() {}

  Child const& child() const {
    return *static_cast<Child const*>( this );
  }
  Child& child() { return *static_cast<Child*>( this ); }

public:
  // For logging/debugging.
  fsm_t const& state() const { return fsm_; }

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

  bool handle_input( input::event_t const& event ) {
    if( auto in_progress = fsm_.template holds<InProgress_t>();
        in_progress ) {
      auto* base =
          variant_base_ptr<input::event_base_t>( event );
      in_progress->get().mod_keys = base->mod;
    }
    // Currently, us handling some input doesn't require us
    // blocking anyone else from handling it.
    return false;
  }

  void handle_draw( Texture& tx ) const {
    switch_( fsm_.state() ) {
      case_( InProgress_t ) {
        auto mouse_pos = input::current_mouse_position();
        copy_texture( val.tx, tx,
                      mouse_pos - val.tx.size() / Scale{ 2 } -
                          val.click_offset );
        // Now draw the indicator.
        auto indicator = drag_status_indicator( val );
        switch( indicator ) {
          case e_drag_status_indicator::none: break;
          case e_drag_status_indicator::bad: {
            auto const& status_tx =
                render_text( "X", Color::red() );
            auto indicator_pos =
                mouse_pos - status_tx.size() / Scale{ 1 };
            copy_texture( status_tx, tx,
                          indicator_pos - val.click_offset );
            break;
          }
          case e_drag_status_indicator::good: {
            auto const& status_tx =
                render_text( "+", Color::green() );
            auto indicator_pos =
                mouse_pos - status_tx.size() / Scale{ 1 };
            copy_texture( status_tx, tx,
                          indicator_pos - val.click_offset );
            if( val.mod_keys.shf_down || val.mod_keys.alt_down ||
                val.mod_keys.ctrl_down ) {
              auto const& mod_tx =
                  render_text( "?", Color::green() );
              auto mod_pos = mouse_pos;
              mod_pos.y -= mod_tx.size().h;
              copy_texture( mod_tx, tx,
                            mod_pos - val.click_offset );
            }
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
                      pos - val.tx.size() / Scale{ 2 } );
      }
      case_( None_t ) break_;
      case_( WaitingToExecute_t ) {
        copy_texture( val.tx, tx,
                      val.mouse_released -
                          val.tx.size() / Scale{ 2 } -
                          val.click_offset );
      }
      case_( Finalizing_t ) {
        copy_texture( val.tx, tx,
                      val.mouse_released -
                          val.tx.size() / Scale{ 2 } -
                          val.click_offset );
      }
      switch_exhaustive;
    }
  }

  struct DragSrcInfo {
    DragSrcT src;
    Rect     rect;
  };

  Plane::DragInfo handle_can_drag( Coord origin ) {
    if( !fsm_.template holds<None_t>() )
      // e.g. we're rubber-banding, or waiting to execute.
      return Plane::e_accept_drag::swallow;
    auto maybe_drag_src_info = child().drag_src( origin );
    if( !maybe_drag_src_info ) return Plane::e_accept_drag::no;
    auto draggable =
        child().draggable_from_src( maybe_drag_src_info->src );
    fsm_.send_event( Start_t{
        /*src=*/maybe_drag_src_info->src,
        /*dst=*/std::nullopt,
        /*tx=*/child().draw_dragged_item( draggable ),
        /*click_offset=*/origin -
            maybe_drag_src_info->rect.center(),
    } );
    fsm_.process_events();
    return Plane::e_accept_drag::yes;
  }

  void handle_on_drag( input::mod_keys const& mod,
                       Coord                  current ) {
    if( auto in_progress = fsm_.template holds<InProgress_t>();
        in_progress ) {
      in_progress->get().dst      = child().drag_dst( current );
      in_progress->get().mod_keys = mod;
    }
  }

  bool handle_on_drag_finished(
      input::mod_keys const& /*unused*/, Coord const& drag_start,
      Coord const& drag_end ) {
    if( auto in_progress_ref =
            fsm_.template holds<InProgress_t>();
        in_progress_ref ) {
      auto& in_progress = in_progress_ref->get();
      if( in_progress.dst ) {
        // Must save these before we change state.
        auto mod_keys = in_progress.mod_keys;
        auto maybe_drag_arc =
            drag_arc( in_progress.src, *in_progress.dst );
        if( maybe_drag_arc &&
            child().can_perform_drag( *maybe_drag_arc ) ) {
          fsm_.send_event( Finalize_t{
              /*arc=*/*maybe_drag_arc,                   //
              /*drag_start=*/drag_start,                 //
              /*mouse_released=*/drag_end,               //
              /*tx=*/std::move( in_progress.tx ),        //
              /*mod_keys=*/mod_keys,                     //
              /*click_offset=*/in_progress.click_offset, //
          } );
          fsm_.process_events();
          child().finalize_drag( mod_keys, *maybe_drag_arc );
          return true;
        }
      }

      fsm_.send_event( RubberBand_t{
          /*current=*/drag_end - in_progress.click_offset,
          /*dest=*/drag_start - in_progress.click_offset,
          /*src=*/in_progress.src,
          /*percent=*/0.0,
          /*tx=*/std::move( in_progress.tx ),
      } );
      fsm_.process_events();
      return true;
    }
    return false;
  }

  void accept_finalized_drag(
      Opt<CRef<DragArcT>> maybe_drag_arc ) {
    ASSIGN_CHECK_OPT( finalized_ref,
                      fsm_.template holds<Finalizing_t>() );
    auto& finalized = finalized_ref.get();
    if( !maybe_drag_arc ) {
      // Caller has told us to cancel the drag.
      fsm_.send_event( RubberBand_t{
          /*current=*/finalized.mouse_released -
              finalized.click_offset,
          /*dest=*/finalized.drag_start - finalized.click_offset,
          /*src=*/drag_src_from_arc( finalized.arc ),
          /*percent=*/0.0,
          /*tx=*/std::move( finalized.tx ),
      } );
      fsm_.process_events();
      return;
    }
    CHECK( child().can_perform_drag( *maybe_drag_arc ) );
    fsm_.send_event( Complete_t{
        /*arc=*/*maybe_drag_arc,                     //
        /*mouse_released=*/finalized.mouse_released, //
        /*tx=*/std::move( finalized.tx ),            //
        /*click_offset=*/finalized.click_offset,     //
    } );
    fsm_.process_events();
  }

  Opt<DraggableObjectT> obj_being_dragged() const {
    Opt<DraggableObjectT> res;
    switch_( fsm_.state() ) {
      case_( None_t ) break_;
      case_( RubberBanding_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( InProgress_t ) {
        res = child().draggable_from_src( val.src );
      }
      case_( WaitingToExecute_t ) { //
        res = draggable_from_arc( val.arc );
      }
      case_( Finalizing_t ) { //
        res = draggable_from_arc( val.arc );
      }
      switch_exhaustive;
    }
    return res;
  }

  // Default implementation; accepts all drags unchanged.
  void finalize_drag( input::mod_keys const& /*unused*/,
                      DragArcT const& drag_arc ) {
    accept_finalized_drag( drag_arc );
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
  using Finalizing_t =
      DragState::finalizing<DragSrcT, DragDstT, DragArcT>;
  using Finalize_t =
      DragEvent::finalize<DragSrcT, DragDstT, DragArcT>;
  using Complete_t =
      DragEvent::complete<DragSrcT, DragDstT, DragArcT>;
  using RubberBanding_t =
      DragState::rubber_banding<DragSrcT, DragDstT, DragArcT>;
  using RubberBand_t =
      DragEvent::rubber_band<DragSrcT, DragDstT, DragArcT>;

  NOTHROW_MOVE( DragState_t<DragSrcT, DragDstT, DragArcT> );
  NOTHROW_MOVE( DragEvent_t<DragSrcT, DragDstT, DragArcT> );
  NOTHROW_MOVE( fsm_t );

private:
  static DragSrcT drag_src_from_arc( DragArcT const& drag_arc ) {
    return std::visit( L( DragSrcT{ _.src } ), drag_arc );
  }

  DraggableObjectT draggable_from_arc(
      DragArcT const& drag_arc ) const {
    auto visitor = [this]( auto const& some_arc ) {
      return child().draggable_from_src( some_arc.src );
    };
    return std::visit( visitor, drag_arc );
  }

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
      *arc = ArcSubType{ *p_src, *p_dst };
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
  NOTHROW_MOVE( fsm_t );
};

} // namespace rn
