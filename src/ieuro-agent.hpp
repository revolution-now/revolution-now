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
#include "command.rds.hpp"
#include "iagent.hpp"
#include "isignal.hpp"
#include "meet-natives.rds.hpp"
#include "native-owned.rds.hpp"
#include "ui-enums.rds.hpp"
#include "wait.hpp"

// ss
#include "ss/native-enums.rds.hpp"
#include "ss/unit-id.hpp"

// refl
#include "refl/enum-map.hpp"

// base
#include "base/heap-value.hpp"

namespace rn {

enum class e_player;
enum class e_woodcut;

struct AnimationSequence;
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

  // For convenience.  Will only return a value for humans.
  virtual bool human() const = 0;

  // This is the interactive part of the sequence of events that
  // happens when first encountering a given native tribe. In
  // particular, it will ask if you want to accept peace. The
  // tile passed in is the tile that triggered the event; it may
  // be the tile of the native unit or the european unit.
  virtual wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe, gfx::point tile ) = 0;

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

  // Upon discovering the new world.
  virtual wait<std::string> name_new_world() = 0;

  virtual wait<ui::e_confirm> should_king_transport_treasure(
      std::string const& msg ) = 0;

  virtual wait<ui::e_confirm>
  should_explore_ancient_burial_mounds() = 0;

  virtual wait<std::chrono::microseconds> wait_for(
      std::chrono::milliseconds us ) = 0;

  virtual wait<> pan_tile( gfx::point tile ) = 0;

  virtual wait<> pan_unit( UnitId unit_id ) = 0;

  virtual command ask_orders( UnitId unit_id ) = 0;

  virtual wait<ui::e_confirm> kiss_pinky_ring(
      std::string const& msg, ColonyId colony_id,
      e_commodity type, int tax_increase ) = 0;

  virtual wait<ui::e_confirm>
  attack_with_partial_movement_points( UnitId unit_id ) = 0;

  virtual wait<ui::e_confirm> should_attack_natives(
      e_tribe tribe ) = 0;

  virtual wait<maybe<int>> pick_dump_cargo(
      std::map<int /*slot*/, Commodity> const& options ) = 0;

  virtual wait<e_native_land_grab_result>
  should_take_native_land(
      std::string const& msg,
      refl::enum_map<e_native_land_grab_result,
                     std::string> const& names,
      refl::enum_map<e_native_land_grab_result, bool> const&
          disabled ) = 0;

 public: // Signals.
  // Non-waitable signal, no message.
  auto signal( NonWaitableSignalContext auto const& ctx ) {
    return handle( ctx );
  }

  // Non-waitable signal, with message.
  auto signal( NonWaitableSignalContext auto const& ctx,
               std::string const& msg )
      -> wait<std::conditional_t<
          std::is_same_v<decltype( this->handle( ctx ) ), void>,
          std::monostate, decltype( this->handle( ctx ) )>> {
    co_await this->message_box( "{}", msg );
    // This one is not waitable.
    co_return handle( ctx );
  }

  // Waitable signal, no message.
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

  // Waitable signal, with message.
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

  bool human() const override;

  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe, gfx::point tile ) override;

  wait<> show_woodcut( e_woodcut woodcut ) override;

  wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst,
      CapturableCargo const& items ) override;

  wait<> notify_captured_cargo(
      Player const& src_player, Player const& dst_player,
      Unit const& dst_unit, Commodity const& stolen ) override;

  wait<std::string> name_new_world() override;

  wait<ui::e_confirm> should_king_transport_treasure(
      std::string const& msg ) override;

  wait<ui::e_confirm> should_explore_ancient_burial_mounds()
      override;

  wait<std::chrono::microseconds> wait_for(
      std::chrono::milliseconds us ) override;

  wait<> pan_tile( gfx::point tile ) override;

  wait<> pan_unit( UnitId unit_id ) override;

  command ask_orders( UnitId unit_id ) override;

  wait<ui::e_confirm> kiss_pinky_ring(
      std::string const& msg, ColonyId colony_id,
      e_commodity type, int tax_increase ) override;

  wait<ui::e_confirm> attack_with_partial_movement_points(
      UnitId unit_id ) override;

  wait<ui::e_confirm> should_attack_natives(
      e_tribe tribe ) override;

  wait<maybe<int>> pick_dump_cargo(
      std::map<int /*slot*/, Commodity> const& options )
      override;

  wait<e_native_land_grab_result> should_take_native_land(
      std::string const& msg,
      refl::enum_map<e_native_land_grab_result,
                     std::string> const& names,
      refl::enum_map<e_native_land_grab_result, bool> const&
          disabled ) override;

 public: // ISignalHandler
  wait<maybe<int>> handle(
      signal::ChooseImmigrant const& ) override;

 private:
  SSConst const& ss_;
};

} // namespace rn
