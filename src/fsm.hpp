/****************************************************************
**fsm.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-25.
*
* Description: Finite State Machine.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "cc-specific.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "flat-queue.hpp"
#include "fmt-helper.hpp"
#include "macros.hpp"

#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// C++ standard library
#include <variant>

namespace rn {

namespace internal {

void log_state( std::string const& child_name,
                std::string const& logged_state );

void log_event( std::string const& child_name,
                std::string const& logged_event );

} // namespace internal

/****************************************************************
** Finite State Machine
*****************************************************************/
// Example:
//
//   adt_rn_( ColorState,
//            ( red ),
//            ( light_red ),
//            ( dark_red )
//   );
//
//   adt_rn_( ColorEvent,
//            ( light ),
//            ( dark )
//   );
//
//   fsm_transitions( Color
//    ,(   (red,       light),  ->   ,light_red
//   ),(   (red,       dark ),  ->   ,dark_red
//   ),(   (light_red, dark ),  ->   ,red
//   ),(   (dark_red,  light),  ->   ,red
//   ));
//
//   fsm_class( Color ) {
//     fsm_init( ColorState::red{} );
//
//     // Optionally can override these to perform actions
//     // during a transition.  Overriding these are mandatory
//     // if either the event or target state has data members.
//     fsm_transition( Color, blue, rotate, ->, yellow ) {
//       cout << "Going from blue to yellow via rotate.\n";
//       return {};
//     }
//   };
//
//   ColorFsm color;
//   lg.info( "color state: {}", color.state() );
//
//   color.send_event( ColorEvent::light{} );
//   color.process_events();
//   lg.info( "color state: {}", color.state() );
//
//   color.send_event( ColorEvent::dark{} );
//   color.process_events();
//   lg.info( "color state: {}", color.state() );
//
template<typename ChildT, typename StateT, typename EventT,
         typename TransitionMap>
class fsm {
  NOTHROW_MOVE( StateT );
  NOTHROW_MOVE( EventT );

protected:
  using Parent = fsm<ChildT, StateT, EventT, TransitionMap>;

public:
  using IamFsm_t = void;
  using state_t  = StateT;

  fsm() : state_{}, events_{} {
    state_ = child().initial_state();
  }

  fsm( StateT&& st ) : state_( std::move( st ) ), events_{} {}

  // Queue an event, but do not process it immediately.
  void send_event( EventT const& event,
                   CALLER_LOCATION( loc ) ) {
    events_.push( { event, loc } );
  }

  // Queue an event, but do not process it immediately.
  void send_event( EventT&& event, CALLER_LOCATION( loc ) ) {
    EventWithSource event_with_src{
        /*event=*/std::move( event ), //
        /*location=*/loc              //
    };
    events_.push_emplace( std::move( event_with_src ) );
  }

  // Process all pending events. NOTE: that, in processing an
  // event, additional events may be added to the queue; this
  // will keep processing until all such events have been han-
  // dled.  Returns true if any events were processed.
  bool process_events() {
    bool processed_events = events_.size() > 0;
    while( auto maybe_event_ref = events_.front() ) {
      internal::log_event(
          demangled_typename<ChildT>(),
          fmt::format( "{}", maybe_event_ref->get().event ) );
      process_event( maybe_event_ref->get() );
      internal::log_state( demangled_typename<ChildT>(),
                           fmt::format( "{}", state_ ) );
      events_.pop();
    }
    return processed_events;
  }

  bool has_pending_events() const { return !events_.empty(); }

  StateT const& state() const { return state_; }

  // !! No pointer stability here; state could change after
  //    calling another non-const method.
  template<typename T>
  Opt<CRef<T>> holds() const {
    Opt<CRef<T>> res;
    if( auto* s = std::get_if<T>( &state_ ); s != nullptr )
      res = *s;
    return res;
  }

  // !! No pointer stability here; state could change after
  //    calling another non-const method.
  template<typename T>
  Opt<Ref<T>> holds() {
    Opt<Ref<T>> res;
    if( auto* s = std::get_if<T>( &state_ ); s != nullptr )
      res = *s;
    return res;
  }

  // NOTE: we're not comparing the event queue here.
  bool operator==( Parent const& rhs ) const {
    return ( state_ == rhs.state_ );
  }

  bool operator!=( Parent const& rhs ) const {
    return !( *this == rhs );
  }

protected:
  struct NoTransition {};

  template<typename T>
  struct FsmTag {
    using type = T;
  };

  // Default transition function.
  template<typename S1, typename E, typename S2>
  static S2 transition( S1 const&, E&, FsmTag<S2> const& ) {
    // Unfortunately it seems that empty structs still report
    // having a size of one, so we'll just assume that if the
    // size is one that it is empty.
    static_assert(
        sizeof( S2 ) <= 1,
        "should not default-construct a state with data "
        "members.  Please provide a custom override of "
        "`transition` to handle this case." );
    return S2{};
  }

private:
  struct EventWithSource {
    EventT    event;
    SourceLoc location;
  };
  NOTHROW_MOVE( EventWithSource );

  ChildT const& child() const {
    return *static_cast<ChildT const*>( this );
  }
  ChildT& child() { return *static_cast<ChildT*>( this ); }

  void process_event( EventWithSource& event_with_src ) {
    auto& event        = event_with_src.event;
    auto& src_location = event_with_src.location;
    auto  visitor      = [this, &src_location](
                       auto const& state,
                       auto&       event ) -> StateT {
      using state1_t = std::decay_t<decltype( state )>;
      using event_t  = std::decay_t<decltype( event )>;
      using key_t    = std::pair<state1_t, event_t>;

      using state2_t = Get<TransitionMap, key_t, NoTransition>;

      if constexpr( std::is_same_v<state2_t, NoTransition> ) {
        // Maybe in the future, at least for release builds, we
        // will want to avoid throwing here and just emit an
        // error and return the current state (i.e., leave state
        // unchanged).
        FATAL(
            "state {} cannot receive the event {} (sent from "
            "{})",
            FmtRemoveTemplateArgs{ state },
            FmtRemoveTemplateArgs{ event }, src_location );
      } else {
        if constexpr(
            !std::is_same_v<
                std::decay_t<decltype( child().transition(
                    state, event, FsmTag<state2_t>{} ) )>,
                state2_t> ) {
          struct
              one_of_your_transition_functions_returns_the_wrong_type {
          };
          // If you get an error on the line below, then one of
          // your transition functions does not return the right
          // type. A transition function must return the type of
          // the new state (not the state variant).
          return one_of_your_transition_functions_returns_the_wrong_type{};
        } else {
          // !! NOTE: event is passed as a non-const reference so
          // that the child may move things out of; i.e., do not
          // use event after this call!
          return child().transition( state, event,
                                     FsmTag<state2_t>{} );
          // !! event's elements can be moved-from here.
        }
      }
    };
    state_ = std::visit( visitor, state_, event );
  }

  StateT                      state_;
  flat_queue<EventWithSource> events_;
  NOTHROW_MOVE( flat_queue<EventWithSource> );
};

/****************************************************************
** Serialization
*****************************************************************/
namespace serial {

template<typename Hint,                  //
         typename T,                     //
         typename T::IamFsm_t* = nullptr //
         >
auto serialize( FBBuilder& builder, T const& o, serial::ADL ) {
  CHECK( !o.has_pending_events(),
         "cannot serialize a finite state machine with pending "
         "events." );
  auto s_state = serialize<serial::fb_serialize_hint_t<decltype(
      std::declval<Hint>().state() )>>( builder, o.state(),
                                        serial::ADL{} );
  return serial::ReturnValue{
      Hint::Create( builder, s_state.get() ) };
}

template<typename SrcT,                     //
         typename DstT,                     //
         typename DstT::IamFsm_t* = nullptr //
         >
expect<> deserialize( SrcT const* src, DstT* dst, serial::ADL ) {
  if( src == nullptr ) return xp_success_t{};
  UNXP_CHECK( src->state() != nullptr );
  typename std::decay_t<DstT>::state_t state;
  XP_OR_RETURN_(
      deserialize( src->state(), &state, serial::ADL{} ) );
  *dst = DstT( std::move( state ) );
  return xp_success_t{};
}

} // namespace serial

/****************************************************************
** Macros For Standard FSM
*****************************************************************/
#define fsm_transitions( ... ) \
  EVAL( FSM_TRANSITION_MAP_IMPL( __VA_ARGS__ ) )

#define FSM_TRANSITION_MAP_IMPL( name, ... )             \
  using name##FsmTransitions = FSM_TRANSITION_MAP_IMPL2( \
      name##State, name##Event, __VA_ARGS__ )

#define FSM_TRANSITION_MAP_IMPL2( state_t_name, event_t_name, \
                                  ... )                       \
  TypeMap<PP_MAP_TUPLE_COMMAS(                                \
      FSM_TO_KV_PAIR,                                         \
      PP_MAP_PREPEND2_TUPLE( state_t_name, event_t_name,      \
                             __VA_ARGS__ ) )>

#define FSM_TO_KV_PAIR( state_t_name, event_t_name,          \
                        state1_event, dummy, state2 )        \
  KV<FSM_TO_PAIR PREPEND_TUPLE2( state_t_name, event_t_name, \
                                 state1_event ),             \
     state_t_name::state2>

#define FSM_TO_PAIR( state_t_name, event_t_name, a, b ) \
  std::pair<state_t_name::a, event_t_name::b>

#define fsm_class( name )                                 \
  struct name##Fsm                                        \
    : public fsm<name##Fsm, name##State_t, name##Event_t, \
                 name##FsmTransitions>

#define fsm_init( a )       \
  using Parent::transition; \
  using Parent::Parent;     \
  auto initial_state() const { return a; }

#define fsm_transition( fsm_name, start, e, dummy, end )   \
  static_assert(                                           \
      std::is_same_v<fsm_name##State::end,                 \
                     Get<fsm_name##FsmTransitions,         \
                         std::pair<fsm_name##State::start, \
                                   fsm_name##Event::e>,    \
                         Parent::NoTransition>>,           \
      "this transition is not in the transitions map" );   \
  fsm_name##State::end transition(                         \
      fsm_name##State::start const& cur,                   \
      fsm_name##Event::e& event, FsmTag<fsm_name##State::end> )

// This version is for when the arguments are not needed.
#define fsm_transition_( fsm_name, start, e, dummy, end )  \
  static_assert(                                           \
      std::is_same_v<fsm_name##State::end,                 \
                     Get<fsm_name##FsmTransitions,         \
                         std::pair<fsm_name##State::start, \
                                   fsm_name##Event::e>,    \
                         Parent::NoTransition>>,           \
      "this transition is not in the transitions map" );   \
  fsm_name##State::end transition(                         \
      fsm_name##State::start const&, fsm_name##Event::e&,  \
      FsmTag<fsm_name##State::end> )

/****************************************************************
** Macros For Templated FSM
*****************************************************************/
#define fsm_transitions_T( ts, ... ) \
  EVAL( FSM_TRANSITION_MAP_T_IMPL( EAT_##ts, __VA_ARGS__ ) )

#define FSM_TRANSITION_MAP_T_IMPL( ts, name, ... )         \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND ts )>    \
  using name##FsmTransitions = FSM_TRANSITION_MAP_T_IMPL2( \
      ts, name##State, name##Event, __VA_ARGS__ )

#define FSM_TRANSITION_MAP_T_IMPL2( ts, state_t_name,        \
                                    event_t_name, ... )      \
  TypeMap<PP_MAP_TUPLE_COMMAS(                               \
      FSM_TO_KV_PAIR_T,                                      \
      PP_MAP_PREPEND3_TUPLE( ts, state_t_name, event_t_name, \
                             __VA_ARGS__ ) )>

#define FSM_TO_KV_PAIR_T( ts, state_t_name, event_t_name, \
                          state1_event, dummy, state2 )   \
  KV<FSM_TO_PAIR_T PREPEND_TUPLE3(                        \
         ts, state_t_name, event_t_name, state1_event ),  \
     state_t_name::state2<EXPAND ts>>

#define FSM_TO_PAIR_T( ts, state_t_name, event_t_name, a, b ) \
  std::pair<state_t_name::a<EXPAND ts>,                       \
            event_t_name::b<EXPAND ts>>

#define fsm_class_T( ts, ... ) \
  EVAL( fsm_class_T_impl( EAT_##ts, __VA_ARGS__ ) )

#define fsm_class_T_impl( ts, name )                    \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND ts )> \
  struct name##Fsm                                      \
    : public fsm<name##Fsm<EXPAND ts>,                  \
                 name##State_t<EXPAND ts>,              \
                 name##Event_t<EXPAND ts>,              \
                 name##FsmTransitions<EXPAND ts>>

#define fsm_init_T( ts, ... ) \
  EVAL( fsm_init_T_impl( EAT_##ts, __VA_ARGS__ ) )

#define fsm_init_T_impl( ts, name, a )                    \
  using Parent =                                          \
      fsm<name##Fsm<EXPAND ts>, name##State_t<EXPAND ts>, \
          name##Event_t<EXPAND ts>,                       \
          name##FsmTransitions<EXPAND ts>>;               \
  using Parent::transition;                               \
  auto initial_state() const { return a; }

#define fsm_transition_T( ts, ... ) \
  EVAL( fsm_transition_T_impl( ( EAT_##ts ), __VA_ARGS__ ) )

#define fsm_transition_T_impl( ts, fsm_name, start, e, dummy, \
                               end )                          \
  static_assert(                                              \
      std::is_same_v<                                         \
          fsm_name##State::end<EXPAND EXPAND ts>,             \
          Get<fsm_name##FsmTransitions<EXPAND EXPAND ts>,     \
              std::pair<                                      \
                  fsm_name##State::start<EXPAND EXPAND ts>,   \
                  fsm_name##Event::e<EXPAND EXPAND ts>>,      \
              typename Parent::NoTransition>>,                \
      "this transition is not in the transitions map" );      \
  fsm_name##State::end<EXPAND EXPAND ts> transition(          \
      fsm_name##State::start<EXPAND EXPAND ts> const&,        \
      fsm_name##Event::e<EXPAND EXPAND ts>& event,            \
      typename Parent::template FsmTag<                       \
          fsm_name##State::end<EXPAND EXPAND ts>> )

/****************************************************************
** Formatting
*****************************************************************/
#define FSM_DEFINE_FORMAT_IMPL( name )                       \
  namespace fmt {                                            \
  template<>                                                 \
  struct formatter<::rn::name##Fsm> : formatter_base {       \
    template<typename FormatContext>                         \
    auto format( ::rn::name##Fsm const& o,                   \
                 FormatContext&         ctx ) {                      \
      return formatter_base::format(                         \
          fmt::format( #name "Fsm{{state={}}}", o.state() ), \
          ctx );                                             \
    }                                                        \
  };                                                         \
  } /* namespace fmt */

#define FSM_DEFINE_FORMAT_T_IMPL( ts, name )                  \
  namespace fmt {                                             \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND EAT_##ts )> \
  struct formatter<::rn::name##Fsm<EXPAND EAT_##ts>>          \
    : formatter_base {                                        \
    template<typename FormatContext>                          \
    auto format( ::rn::name##Fsm<EXPAND EAT_##ts> const& o,   \
                 FormatContext&                          ctx ) {                       \
      return formatter_base::format(                          \
          fmt::format( #name "Fsm{{state={}}}", o.state() ),  \
          ctx );                                              \
    }                                                         \
  };                                                          \
  } /* namespace fmt */

#define FSM_DEFINE_FORMAT_RN( name ) \
  } /* close namespace rn */         \
  FSM_DEFINE_FORMAT_IMPL( name )     \
  /* reopen namespace rn */          \
  namespace rn {

#define FSM_DEFINE_FORMAT_RN_( name ) \
  } /* close namespace (anonymous) */ \
  } /* close namespace rn */          \
  FSM_DEFINE_FORMAT_IMPL( name )      \
  /* reopen namespace rn */           \
  namespace rn {                      \
  /* reopen namespace (anonymous) */  \
  namespace {

#define FSM_DEFINE_FORMAT_T_RN_NO_EVAL( ts, name ) \
  } /* close namespace rn */                       \
  FSM_DEFINE_FORMAT_T_IMPL( ts, name )             \
  /* reopen namespace rn */                        \
  namespace rn {

#define FSM_DEFINE_FORMAT_T_RN( ts, name ) \
  EVAL( FSM_DEFINE_FORMAT_T_RN_NO_EVAL( ts, name ) )

#define FSM_DEFINE_FORMAT_T_RN__NO_EVAL( ts, name ) \
  } /* close namespace (anonymous) */               \
  } /* close namespace rn */                        \
  FSM_DEFINE_FORMAT_T_IMPL( ts, name )              \
  /* reopen namespace rn */                         \
  namespace rn {                                    \
  /* reopen namespace (anonymous) */                \
  namespace {

#define FSM_DEFINE_FORMAT_T_RN_( ts, name ) \
  EVAL( FSM_DEFINE_FORMAT_T_RN__NO_EVAL( ts, name ) )

/****************************************************************
** Testing
*****************************************************************/
void test_fsm();

} // namespace rn
