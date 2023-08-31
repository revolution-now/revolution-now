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

namespace rn {

enum class e_nation;

struct MeetTribe;

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

  // Implement IMind.
  wait<> message_box( std::string const& msg ) override;

  // Implement IEuroMind.
  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe ) override;
};

} // namespace rn
