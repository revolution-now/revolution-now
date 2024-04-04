/****************************************************************
**colony-evolve.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colony-evolve.rds.hpp"

namespace rn {

struct Colony;
struct Player;
struct SS;
struct TS;

/****************************************************************
** IColonyEvolver
*****************************************************************/
// Interface for evolving a colony one turn and obtaining both
// internal and displayable results.
struct IColonyEvolver {
  virtual ~IColonyEvolver() = default;

  static std::unique_ptr<IColonyEvolver> create( SS& ss,
                                                 TS& ts );

  // Evolve the colony by one turn. This is not a coroutine for a
  // few reasons: 1) ease of testability, 2) we want it to also
  // be used for the AI players, 3) we want to be able to have a
  // way to evolve a colony (e.g. for cheat mode) where we can
  // control what is shown to the user.
  virtual ColonyEvolution evolve_one_turn(
      Colony& colony ) const = 0;

  virtual ColonyNotificationMessage
  generate_notification_message(
      Colony const&             colony,
      ColonyNotification const& notification ) const = 0;
};

#define DEFINE_MOCK_IColonyEvolver()                          \
  struct MockIColonyEvolver : public IColonyEvolver {         \
    MOCK_METHOD( ColonyEvolution, evolve_one_turn, (Colony&), \
                 ( const ) );                                 \
                                                              \
    MOCK_METHOD( ColonyNotificationMessage,                   \
                 generate_notification_message,               \
                 (Colony const&, ColonyNotification const&),  \
                 ( const ) );                                 \
  }

} // namespace rn
