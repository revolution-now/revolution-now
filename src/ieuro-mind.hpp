/****************************************************************
**ieuro-mind.hpp
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
#include "imind.hpp"
#include "meet-natives.rds.hpp"
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

// base
#include "base/heap-value.hpp"

namespace rn {

enum class e_nation;
enum class e_woodcut;

struct MeetTribe;
struct CapturableCargo;
struct CapturableCargoItems;

/****************************************************************
** IEuroMind
*****************************************************************/
struct IEuroMind : IMind {
  IEuroMind( e_nation nation );

  virtual ~IEuroMind() override = default;

  // For convenience.
  e_nation nation() const { return nation_; }

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

 private:
  e_nation nation_ = {};
};

/****************************************************************
** NoopEuroMind
*****************************************************************/
// Minimal implementation does not either nothing or the minimum
// necessary to fulfill the contract of each request.
struct NoopEuroMind final : IEuroMind {
  NoopEuroMind( e_nation nation );

 public: // IMind.
  wait<> message_box( std::string const& msg ) override;

 public: // IEuroMind.
  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe ) override;

  wait<> show_woodcut( e_woodcut woodcut ) override;

  wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst,
      CapturableCargo const& items ) override;
};

} // namespace rn
