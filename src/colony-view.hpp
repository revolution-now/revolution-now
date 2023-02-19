/****************************************************************
**colony-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-05.
*
* Description: The view that appears when clicking on a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "colony-view.rds.hpp"

// ss
#include "ss/colony-id.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Colony;
struct SS;
struct TS;

/****************************************************************
** IColonyViewer
*****************************************************************/
struct IColonyViewer {
  virtual ~IColonyViewer() = default;

  // Returns whether the colony was abandoned by the player while
  // open. The caller needs to know about that so that it can
  // take the appropriate measures to not access it after that,
  // since it will no longer exist.
  virtual wait<e_colony_abandoned> show(
      TS& ts, ColonyId colony_id ) = 0;
};

/****************************************************************
** ColonyViewer
*****************************************************************/
struct ColonyViewer : IColonyViewer {
  // We don't take TS here because that would be a circular de-
  // pendency, since the IColonyViewer is part of TS. Instead, we
  // take it explicitly in each method invidually where it is
  // needed.
  ColonyViewer( SS& ss );

  // Implement IColonyViewer.
  wait<e_colony_abandoned> show( TS&      ts,
                                 ColonyId colony_id ) override;

 private:
  wait<> show_impl( TS& ts, Colony& colony );

  SS& ss_;
};

} // namespace rn
