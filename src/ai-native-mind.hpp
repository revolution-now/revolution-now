/****************************************************************
**ai-native-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: AI for natives.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "inative-mind.hpp"

namespace rn {

struct IRand;
struct SS;

/****************************************************************
** AiNativeMind
*****************************************************************/
struct AiNativeMind : INativeMind {
  // We don't take TS here because would create a circular depen-
  // dency.
  AiNativeMind( SS& ss, IRand& rand );

  // Implement INativeMind.
  NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) override;

  // Implement INativeMind.
  NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) override;

 private:
  SS&    ss_; // can this be SSConst?
  IRand& rand_;
};

} // namespace rn
