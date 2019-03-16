/****************************************************************
**analysis.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-08.
*
* Description: Analyzes player orders on units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

/****************************************************************
**Orders Analyzers
*****************************************************************/
template<typename Child>
struct OrdersAnalysis {
  OrdersAnalysis( UnitId id_, orders_t orders_,
                  std::vector<UnitId> units_to_prioritize_ )
    : id( id_ ),
      orders( orders_ ),
      units_to_prioritize( std::move( units_to_prioritize_ ) ) {}

  using parent_t = OrdersAnalysis<Child>;
  Child const* child() const {
    return static_cast<Child const*>( this );
  }
  Child* child() { return static_cast<Child*>( this ); }

  // ---------------------- Methods ----------------------------

  // Is it possible to move at all. This just checks that the the
  // `desc` holds the right type. Note that this does not take
  // into account movement points. Hence 'allowed' here means
  // whether the move could ever be allowed in theory assuming
  // movement points were not a concern.
  bool allowed() const { return child()->allowed_(); }

  // Checks that the order is possible (if not, returns false)
  // and, if possible, will determine whether the player needs to
  // be asked for any kind of confirmation. In addition, if the
  // order is not allowed, the player may be given an
  // explantation as to why.
  bool confirm_explain() const {
    return child()->confirm_explain_();
  }

  // This will apply the orders described by the structure, i.e.,
  // it will change the state of the world!
  void affect_orders() const {
    CHECK( allowed() );
    child()->affect_orders_();
  }

  // Analyzes the move and returns nullopt if it is
  // non-applicable or returns this data structure (populated) if
  // it is applicable (though may still be disallowed).
  static Opt<Child> analyze( UnitId id, orders_t orders ) {
    return Child::analyze_( id, orders );
  }

  // ------------------------ Data -----------------------------

  // The unit being given the orders.
  UnitId id;

  // The orders that gave rise to this.
  orders_t orders;

  // Units that will be waiting for orders and which should be
  // prioritized in the "orders" loop after this move is made.
  // This field is only relevant for certain (valid) moves. NOTE:
  // units will be prioritized in reverse order of this vector,
  // i.e., the last unit will be up first.
  std::vector<UnitId> units_to_prioritize{};
};

// These are always allowed; a meta order is an order that
// concerns the units orders, such as `wait`, `forfeight`, etc.
struct MetaAnalysis : public OrdersAnalysis<MetaAnalysis> {
  MetaAnalysis( UnitId id_, orders_t orders_,
                bool mv_points_forfeighted_ )
    : parent_t( id_, orders_, /*units_to_prioritize_=*/{} ),
      mv_points_forfeighted( mv_points_forfeighted_ ) {}

  // ------------------------ Data -----------------------------

  // Will execution of this order require the unit to forfeight
  // its remaining movement points this turn (no matter how many
  // it may have).
  bool mv_points_forfeighted;

  // ---------------- "Virtual" Methods ------------------------

  bool allowed_() const { return true; }
  bool confirm_explain_() const { return true; }
  void affect_orders_() const;

  static Opt<MetaAnalysis> analyze_( UnitId   id,
                                     orders_t orders );
};

} // namespace rn
