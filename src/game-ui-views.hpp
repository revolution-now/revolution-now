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

// rds
#include "game-ui-views.rds.hpp"

// Revolution Now
#include "macros.hpp"
#include "unit-id.hpp"
#include "views.hpp"

// ss
#include "ss/unit.hpp"

// C++ standard library
#include <memory>
#include <unordered_map>
#include <vector>

namespace rr {
struct ITextometer;
}

namespace rn {

struct SSConst;

/****************************************************************
** UnitActivationView
*****************************************************************/
class UnitActivationView final : public ui::CompositeSingleView {
 public:
  using map_t = std::unordered_map<UnitId, UnitActivationInfo>;

  // Preferred way to create.
  static std::unique_ptr<UnitActivationView> Create(
      rr::ITextometer const& textometer, SSConst const& ss,
      std::vector<UnitId> const& ids_ );

  UnitActivationView();

  // Implement CompositeView
  void notify_children_updated() override {}

  map_t& info_map() { return info_map_; }

 private:
  void on_click_unit( UnitId id );

  map_t info_map_;
};

} // namespace rn
