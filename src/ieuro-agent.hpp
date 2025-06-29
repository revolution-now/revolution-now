/****************************************************************
**ieuro-agent.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "iagent.hpp"
#include "isignal.hpp"
#include "meet-natives.rds.hpp"
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

// base
#include "base/heap-value.hpp"

namespace rn {

enum class e_player;
enum class e_woodcut;

struct CapturableCargo;
struct CapturableCargoItems;
struct Commodity;
struct MeetTribe;
struct Player;
struct SSConst;
struct Unit;

/****************************************************************
** Concepts
*****************************************************************/
template<typename Context>
concept WaitableSignalContext =
    requires( ISignalHandler& h, Context const& ctx ) {
      { h.handle( ctx ) } -> IsWait;
    };

template<typename Context>
concept NonWaitableSignalContext =
    !WaitableSignalContext<Context> &&
    requires( ISignalHandler& h, Context const& ctx ) {
      h.handle( ctx );
    };

template<typename Context>
using signal_context_result_t =
    decltype( std::declval<ISignalHandler>().handle(
        std::declval<Context>() ) );

/****************************************************************
** IEuroAgent
*****************************************************************/
struct IEuroAgent : IAgent, ISignalHandler {
  IEuroAgent( e_player player );

  virtual ~IEuroAgent() override = default;

  // For convenience.
  e_player player_type() const { return player_type_; }

  // For convenience.
  virtual Player const& player() = 0;

  // This is the interactive part of the sequence of events that
  // happens when first encountering a given native tribe. In
  // particular, it will ask if you want to accept peace.
  virtual wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe ) = 0;

  // Woodcut's are static "cut scenes" consisting of a single
  // image pixelated to mark a (good or bad) milestone in the
  // game. This will show it each time it is called.
  virtual wait<> show_woodcut( e_woodcut woodcut ) = 0;

  virtual wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst, CapturableCargo const& items ) = 0;

  // This is used to notify the player when the cargo in one of
  // their ships has been captured by a foreign ship.
  virtual wait<> notify_captured_cargo(
      Player const& src_player, Player const& dst_player,
      Unit const& dst_unit, Commodity const& stolen ) = 0;

 public: // Signals.
  // Non-waitable, no message.
  auto signal( NonWaitableSignalContext auto const& ctx ) {
    return handle( ctx );
  }

  // Waitable, no message.
  auto signal( WaitableSignalContext auto const& ctx )
      -> signal_context_result_t<decltype( ctx )> {
    if constexpr( std::is_same_v<
                      std::monostate,
                      typename signal_context_result_t<
                          decltype( ctx )>::value_type> )
      co_await handle( ctx );
    else
      co_return co_await handle( ctx );
  }

  // Waitable, with message.
  auto signal( WaitableSignalContext auto const& ctx,
               std::string const& msg )
      -> signal_context_result_t<decltype( ctx )> {
    co_await this->message_box( "{}", msg );
    if constexpr( std::is_same_v<
                      std::monostate,
                      typename signal_context_result_t<
                          decltype( ctx )>::value_type> )
      co_await signal( ctx );
    else
      co_return co_await signal( ctx );
  }

 private:
  e_player player_type_ = {};
};

/****************************************************************
** NoopEuroAgent
*****************************************************************/
// Minimal implementation does not either nothing or the minimum
// necessary to fulfill the contract of each request.
struct NoopEuroAgent final : IEuroAgent {
  NoopEuroAgent( SSConst const& ss, e_player player );

 public: // IAgent.
  wait<> message_box( std::string const& msg ) override;

 public: // IEuroAgent.
  Player const& player() override;

  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe ) override;

  wait<> show_woodcut( e_woodcut woodcut ) override;

  wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst,
      CapturableCargo const& items ) override;

  wait<> notify_captured_cargo(
      Player const& src_player, Player const& dst_player,
      Unit const& dst_unit, Commodity const& stolen ) override;

 public: // ISignalHandler
  bool handle( signal::Foo const& foo ) override;

  wait<int> handle( signal::Bar const& foo ) override;

 private:
  SSConst const& ss_;
};

} // namespace rn
