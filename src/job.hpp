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

// C++ standard library
#include <variant>

namespace rn {

enum class ND e_unit_job_good { fortify, sentry };

enum class ND e_unit_job_error { ship_cannot_fortify };

using v_unit_job_desc =
    std::variant<e_unit_job_good, e_unit_job_error>;

struct JobAnalysis : public OrdersAnalysis<JobAnalysis> {
  JobAnalysis( UnitId id_, Orders orders_ )
    : parent_t( id_, orders_ ) {}

  // ------------------------ Data -----------------------------

  v_unit_job_desc desc{};

  // ---------------- "Virtual" Methods ------------------------

  bool allowed_() const;
  bool confirm_explain_() const;
  void affect_orders_() const;

  static Opt<JobAnalysis> analyze_( UnitId id, Orders orders );
};

} // namespace rn
