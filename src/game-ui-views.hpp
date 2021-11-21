/****************************************************************
**game-ui-views.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-13.
*
* Description: Contains high-level game-specific UI Views.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "id.hpp"
#include "macros.hpp"
#include "unit.hpp"
#include "views.hpp"

// base
#include "base/fmt.hpp"
#include "base/to-str.hpp"

// C++ standard library
#include <unordered_map>

namespace rn::ui {

/****************************************************************
** UnitActivationInfo
*****************************************************************/
// Holds the state of each unit in the window as the player is
// selecting them and cycling them through the states.
struct UnitActivationInfo {
  e_unit_orders original_orders;
  e_unit_orders current_orders;
  bool          is_activated;

  friend void to_str( UnitActivationInfo const& o,
                      std::string&              out );
};

/****************************************************************
** UnitActivationView
*****************************************************************/
class UnitActivationView final : public CompositeSingleView {
 public:
  using map_t = std::unordered_map<UnitId, UnitActivationInfo>;

  // Preferred way to create.
  static std::unique_ptr<UnitActivationView> Create(
      std::vector<UnitId> const& ids_, bool allow_activation );

  UnitActivationView( bool allow_activation );

  // Implement CompositeView
  void notify_children_updated() override {}

  map_t& info_map() { return info_map_; }

 private:
  void on_click_unit( UnitId id );

  bool  allow_activation_{};
  map_t info_map_;
};

} // namespace rn::ui

TOSTR_TO_FMT( ::rn::ui::UnitActivationInfo );
