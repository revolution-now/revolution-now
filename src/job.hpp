/****************************************************************
**job.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles job orders on units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "analysis.hpp"
#include "macros.hpp"
#include "waitable.hpp"

// base
#include "base/variant.hpp"

// C++ standard library
#include <variant>

namespace rn {

enum class ND e_unit_job_good {
  fortify,
  sentry,
  disband,
  build
};

enum class ND e_unit_job_error {
  ship_cannot_fortify,
  cannot_fortify_on_ship,
  colony_exists_here,
  no_water_colony,
  ship_cannot_found_colony
};

using v_unit_job_desc =
    base::variant<e_unit_job_good, e_unit_job_error>;
NOTHROW_MOVE( v_unit_job_desc );

struct JobAnalysis : public OrdersAnalysis<JobAnalysis> {
  JobAnalysis( UnitId id_, orders_t orders_ )
    : parent_t( id_, orders_, /*units_to_prioritize_=*/{} ) {}

  // ------------------------ Data -----------------------------

  v_unit_job_desc desc{};
  std::string     colony_name;

  // ---------------- "Virtual" Methods ------------------------

  bool           allowed_() const;
  waitable<bool> confirm_explain_();
  void           affect_orders_() const;

  static maybe<JobAnalysis> analyze_( UnitId   id,
                                      orders_t orders );
};
NOTHROW_MOVE( JobAnalysis );

} // namespace rn
