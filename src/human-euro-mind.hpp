/****************************************************************
**human-euro-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IEuroMind for human players.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "ieuro-mind.hpp"

namespace rn {

struct IGui;
struct SS;

/****************************************************************
** HumanEuroMind
*****************************************************************/
// This is an implementation that will consult with a human user
// via GUI actions or input in order to fulfill the requests.
struct HumanEuroMind final : IEuroMind {
  HumanEuroMind( e_nation nation, SS& ss, IGui& gui );

  // Implement IEuroMind.
  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe ) override;

 private:
  SS&   ss_;
  IGui& gui_;
};

} // namespace rn
